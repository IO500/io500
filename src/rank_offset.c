#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <io500-opt.h>
#include <io500-util.h>
#include <io500-rank-offset.h>

/* number of random draws before falling back to a scan of all values */
#define RANK_OFFSET_DRAWS 64
/* limit for the fallback scan, in rank->node comparisons */
#define RANK_OFFSET_SCAN_BUDGET 100000000LL

static u_node_table_t node_table;

u_node_table_t const * u_node_table(void){
  return & node_table;
}

void u_node_table_init(MPI_Comm comm){
  u_node_table_t * t = & node_table;
  MPI_Comm shm;
  int rank, size, shm_rank, leader;

  memset(t, 0, sizeof(*t));
  UMPI_CHECK(MPI_Comm_rank(comm, & rank));
  UMPI_CHECK(MPI_Comm_size(comm, & size));
  t->nranks = size;

  /* identify each node by the lowest rank in its shared-memory comm */
  UMPI_CHECK(MPI_Comm_split_type(comm, MPI_COMM_TYPE_SHARED, 0, MPI_INFO_NULL, & shm));
  UMPI_CHECK(MPI_Comm_rank(shm, & shm_rank));
  leader = rank;
  UMPI_CHECK(MPI_Bcast(& leader, 1, MPI_INT, 0, shm));
  UMPI_CHECK(MPI_Comm_free(& shm));

  int * leaders = NULL;
  if(rank == 0){
    leaders = u_malloc(sizeof(int) * size);
    t->node = u_malloc(sizeof(int) * size);
  }
  UMPI_CHECK(MPI_Gather(& leader, 1, MPI_INT, leaders, 1, MPI_INT, 0, comm));

  int info[4];
  if(rank == 0){
    /* map each leader rank to a dense node index, count ranks per node */
    int * idx = u_malloc(sizeof(int) * size);
    int * ppn = u_malloc(sizeof(int) * size);
    for(int r = 0; r < size; r++){
      idx[r] = -1;
    }
    for(int r = 0; r < size; r++){
      int l = leaders[r];
      if(idx[l] < 0){
        idx[l] = t->nnodes;
        ppn[t->nnodes] = 0;
        t->nnodes++;
      }
      t->node[r] = idx[l];
      ppn[idx[l]]++;
    }
    t->ppn_min = INT_MAX;
    t->ppn_max = 0;
    for(int n = 0; n < t->nnodes; n++){
      t->ppn_min = ppn[n] < t->ppn_min ? ppn[n] : t->ppn_min;
      t->ppn_max = ppn[n] > t->ppn_max ? ppn[n] : t->ppn_max;
    }
    info[0] = t->nnodes;
    info[1] = t->ppn_min;
    info[2] = t->ppn_max;
    info[3] = ppn[t->node[0]];
    free(idx);
    free(ppn);
    free(leaders);
  }
  UMPI_CHECK(MPI_Bcast(info, 4, MPI_INT, 0, comm));
  t->nnodes = info[0];
  t->ppn_min = info[1];
  t->ppn_max = info[2];

  /*
   * Model what IOR and mdtest compute: GetNumTasksOnNode0(), GetNumNodes()
   * and QueryNodeMapping(), which compares the host name of rank 1 with
   * that of rank 0, including the IOR_FAKE_* environment overrides.
   */
  t->ppn0 = info[3];
  t->tool_nnodes = t->nnodes;
  if(getenv("IOR_FAKE_TASK_PER_NODES")){
    t->ppn0 = atoi(getenv("IOR_FAKE_TASK_PER_NODES"));
  }
  if(getenv("IOR_FAKE_NODES")){
    t->tool_nnodes = atoi(getenv("IOR_FAKE_NODES"));
  }

  char localhost[1024], roothost[1024];
  if(gethostname(localhost, sizeof(localhost)) != 0){
    FATAL("gethostname() failed\n");
  }
  localhost[sizeof(localhost) - 1] = 0;
  strcpy(roothost, localhost);
  UMPI_CHECK(MPI_Bcast(roothost, sizeof(roothost), MPI_CHAR, 0, comm));
  t->block_mapping = 1;
  if(size > 1){
    t->block_mapping = strcmp(roothost, localhost) == 0;
    UMPI_CHECK(MPI_Bcast(& t->block_mapping, 1, MPI_INT, 1, comm));
  }
}

void u_node_table_print(void){
  u_node_table_t const * t = & node_table;

  if(opt.rank != 0){
    return;
  }
  PRINT_PAIR("nodes", "%d\n", t->nnodes);
  if(t->ppn_min == t->ppn_max){
    PRINT_PAIR("procs-per-node", "%d\n", t->ppn_min);
  }else{
    PRINT_PAIR("procs-per-node", "%d-%d\n", t->ppn_min, t->ppn_max);
  }
  PRINT_PAIR("rank-mapping", "%s\n", t->block_mapping ? "block" : "cyclic");
  if(getenv("IOR_FAKE_TASK_PER_NODES") || getenv("IOR_FAKE_NODES")){
    WARNING("IOR_FAKE_TASK_PER_NODES or IOR_FAKE_NODES is set, IOR/mdtest use %d nodes with %d procs on node 0\n", t->tool_nnodes, t->ppn0);
  }
}

int u_rank_offset_unit(u_node_table_t const * t, io500_offset_tool_t tool){
  if(! t->block_mapping){
    return 1;
  }
  if(tool == IO500_OFFSET_MDTEST && t->tool_nnodes <= 1){
    return 1;
  }
  return t->ppn0 > 0 ? t->ppn0 : 1;
}

int u_rank_offset_shift(u_node_table_t const * t, io500_offset_tool_t tool, int v, int k){
  int64_t unit = u_rank_offset_unit(t, tool);
  int64_t n = t->nranks;

  if(n <= 0 || v < 0){
    return -1;
  }
  if(tool == IO500_OFFSET_IOR){
    /* IOR: rankOffset = (taskPerNodeOffset * shift) % numTasks */
    if(v * unit > INT_MAX){
      return -1;
    }
    return (int)((v * unit) % n);
  }
  /* mdtest: nstride *= ppn0, names use rank + k * nstride, -u dirs up to 4 */
  if((n - 1) + 4 * v * unit > INT_MAX){
    return -1;
  }
  return (int)((k * v * unit) % n);
}

/* count each rank comparison against *budget, if set */
static int uncached(u_node_table_t const * t, int shift, int const * prev, int nprev, int64_t * budget){
  int n = t->nranks;

  if(shift < 0){
    return 0;
  }
  for(int i = 0; i < nprev; i++){
    /* the data of rank r + shift was accessed before from rank r + d */
    int d = (int)(((int64_t) shift - prev[i]) % n);
    if(d < 0){
      d += n;
    }
    if(d == 0){
      return 0;
    }
    for(int r = 0; r < n; r++){
      int peer = r + d;
      if(peer >= n){
        peer -= n;
      }
      if(budget){
        (*budget)--;
      }
      if(t->node[r] == t->node[peer]){
        return 0;
      }
    }
  }
  return 1;
}

int u_rank_offset_uncached(u_node_table_t const * t, int shift, int const * prev, int nprev){
  return uncached(t, shift, prev, nprev, NULL);
}

int u_rank_offset_cross_node(u_node_table_t const * t, io500_offset_tool_t tool, int v, int const * mult, int nmult){
  int writer = 0;

  for(int i = 0; i < nmult; i++){
    int shift = u_rank_offset_shift(t, tool, v, mult[i]);
    if(! uncached(t, shift, & writer, 1, NULL)){
      return 0;
    }
  }
  return 1;
}

static uint64_t splitmix64(uint64_t * state){
  uint64_t z = (*state += 0x9E3779B97F4A7C15ULL);
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
  return z ^ (z >> 31);
}

static int gcd(int a, int b){
  while(b != 0){
    int c = a % b;
    a = b;
    b = c;
  }
  return a;
}

typedef struct{
  u_node_table_t const * t;
  io500_offset_tool_t tool;
  int same_v;
  int period;       /* v * unit (mod n) repeats with this period */
  int64_t budget;   /* rank comparisons left */
  uint64_t * seed;
} choose_ctx_t;

static int search(choose_ctx_t * c, int const * prev, int nprev, int const * mult, int nmult, int lookahead);

/* <v> is valid for mult[0], and with lookahead also leaves the later phases a valid choice */
static int valid_seq(choose_ctx_t * c, int v, int const * prev, int nprev, int const * mult, int nmult, int lookahead){
  int shift = u_rank_offset_shift(c->t, c->tool, v, mult[0]);

  if(! uncached(c->t, shift, prev, nprev, & c->budget)){
    return 0;
  }
  if(nmult == 1 || ! lookahead || nprev >= IO500_RANK_OFFSET_MAX_SHIFTS){
    return 1;
  }
  int next[IO500_RANK_OFFSET_MAX_SHIFTS];
  memcpy(next, prev, sizeof(int) * nprev);
  next[nprev] = shift;
  if(c->same_v){
    return valid_seq(c, v, next, nprev + 1, mult + 1, nmult - 1, 1);
  }
  return search(c, next, nprev + 1, mult + 1, nmult - 1, 1) > 0;
}

static int search(choose_ctx_t * c, int const * prev, int nprev, int const * mult, int nmult, int lookahead){
  for(int i = 0; i < RANK_OFFSET_DRAWS && c->budget > 0; i++){
    int v = 1 + (int)(splitmix64(c->seed) % (uint64_t)(c->period - 1));
    if(valid_seq(c, v, prev, nprev, mult, nmult, lookahead)){
      return v;
    }
  }
  /* few values are valid, scan them from a random starting point */
  int start = (int)(splitmix64(c->seed) % (uint64_t)(c->period - 1));
  for(int i = 0; i < c->period - 1 && c->budget > 0; i++){
    int v = 1 + (start + i) % (c->period - 1);
    if(valid_seq(c, v, prev, nprev, mult, nmult, lookahead)){
      return v;
    }
  }
  return -1;
}

int u_rank_offset_choose(u_node_table_t const * t, io500_offset_tool_t tool, int const * prev, int nprev, int const * mult, int nmult, int same_v, uint64_t * seed){
  int n = t->nranks;
  int unit = u_rank_offset_unit(t, tool);

  if(t->nnodes < 2 || n < 2 || nmult < 1){
    return -1;
  }
  choose_ctx_t c = {.t = t, .tool = tool, .same_v = same_v, .seed = seed};
  c.period = n / gcd(n, unit % n == 0 ? n : unit % n);
  if(c.period < 2){
    return -1;
  }
  c.budget = RANK_OFFSET_SCAN_BUDGET;
  int v = search(& c, prev, nprev, mult, nmult, nmult > 1);
  if(v < 0 && nmult > 1){
    /* no value suits all later phases, at least make this one valid */
    c.budget = RANK_OFFSET_SCAN_BUDGET;
    v = search(& c, prev, nprev, mult, 1, 0);
  }
  return v;
}

/* rank shifts of the phases that accessed a data set, 0 for its writer */
typedef struct{
  char name[32];
  int nshift;
  int shift[IO500_RANK_OFFSET_MAX_SHIFTS];
} rank_shift_hist_t;

#define RANK_OFFSET_MAX_DATASETS 8
static rank_shift_hist_t hist[RANK_OFFSET_MAX_DATASETS];
static int nhist;

static void hist_filename(char * filename, char const * dataset){
  sprintf(filename, "%s/%s.rank-shift", opt.resdir, dataset);
}

/* rank 0: the accesses so far, from memory or from an earlier run in the result dir */
static rank_shift_hist_t * hist_get(char const * dataset){
  for(int i = 0; i < nhist; i++){
    if(strcmp(hist[i].name, dataset) == 0){
      return & hist[i];
    }
  }
  if(nhist >= RANK_OFFSET_MAX_DATASETS){
    FATAL("Too many data sets for rank offsets\n");
  }
  rank_shift_hist_t * h = & hist[nhist++];
  snprintf(h->name, sizeof(h->name), "%s", dataset);
  h->nshift = 0;
  char filename[PATH_MAX];
  hist_filename(filename, dataset);
  FILE * f = fopen(filename, "r");
  if(f){
    while(h->nshift < IO500_RANK_OFFSET_MAX_SHIFTS && fscanf(f, "%d", & h->shift[h->nshift]) == 1){
      h->nshift++;
    }
    fclose(f);
  }
  if(h->nshift == 0){
    h->shift[h->nshift++] = 0;
  }
  return h;
}

static void hist_save(rank_shift_hist_t const * h){
  if(opt.dry_run){
    return;
  }
  char filename[PATH_MAX];
  hist_filename(filename, h->name);
  FILE * f = fopen(filename, "w");
  if(! f){
    WARNING("Couldn't write %s\n", filename);
    return;
  }
  for(int i = 0; i < h->nshift; i++){
    fprintf(f, "%d\n", h->shift[i]);
  }
  fclose(f);
}

void u_rank_offset_written(char const * dataset){
  if(opt.rank != 0){
    return;
  }
  rank_shift_hist_t * h = hist_get(dataset);
  h->nshift = 1;
  h->shift[0] = 0;
  hist_save(h);
}

void u_rank_offset_cleanup(char const * dataset){
  if(opt.rank != 0){
    return;
  }
  char filename[PATH_MAX];
  hist_filename(filename, dataset);
  unlink(filename);
  rank_shift_hist_t * h = hist_get(dataset);
  h->nshift = 1;
  h->shift[0] = 0;
}

/*
 * Fresh entropy for each draw, so nothing in memory before the phase starts
 * determines the offset.  The seed cannot be configured.
 */
static uint64_t rank_offset_seed(void){
  uint64_t seed = 0;
  struct timespec ts;

  int fd = open("/dev/urandom", O_RDONLY);
  if(fd >= 0){
    if(read(fd, & seed, sizeof(seed)) != sizeof(seed)){
      seed = 0;
    }
    close(fd);
  }
  clock_gettime(CLOCK_REALTIME, & ts);
  seed ^= (uint64_t) ts.tv_sec * 1000000000ULL + ts.tv_nsec;
  clock_gettime(CLOCK_MONOTONIC, & ts);
  seed ^= ((uint64_t) ts.tv_nsec << 32) ^ ((uint64_t) getpid() << 16);
  return seed;
}

int u_rank_offset_draw(char const * dataset, io500_offset_tool_t tool, int const * mult, int nmult, int same_v, int dflt){
  int v = dflt;

  if(opt.rank == 0){
    if(opt.rank_offset != INI_UNSET_INT){
      v = opt.rank_offset;
    }else{
      rank_shift_hist_t * h = hist_get(dataset);
      uint64_t seed = rank_offset_seed();
      int c = u_rank_offset_choose(& node_table, tool, h->shift, h->nshift, mult, nmult, same_v, & seed);
      if(c < 0){
        /* nothing avoids every earlier access, at least avoid the writer */
        int writer = 0;
        c = u_rank_offset_choose(& node_table, tool, & writer, 1, mult, 1, 0, & seed);
      }
      if(c > 0){
        v = c;
      }
    }
  }
  UMPI_CHECK(MPI_Bcast(& v, 1, MPI_INT, 0, MPI_COMM_WORLD));
  return v;
}

void u_rank_offset_check(char const * dataset, io500_offset_tool_t tool, int v, int k){
  u_node_table_t const * t = & node_table;

  if(opt.rank != 0){
    return;
  }
  rank_shift_hist_t * h = hist_get(dataset);
  int shift = u_rank_offset_shift(t, tool, v, k);
  char earlier[IO500_RANK_OFFSET_MAX_SHIFTS * 12] = "";
  for(int i = 0; i < h->nshift; i++){
    sprintf(earlier + strlen(earlier), "%s%d", i ? " " : "", h->shift[i]);
  }
  PRINT_PAIR("rank-offset", "%d\n", v);
  PRINT_PAIR("rank-shift", "%d\n", shift);
  PRINT_PAIR("rank-shift-earlier", "%s\n", earlier);
  if(opt.rank_offset != INI_UNSET_INT){
    INVALID("rank-offset forced to %d by the [debug] section\n", opt.rank_offset);
  }else if(! u_rank_offset_uncached(t, shift, h->shift, h->nshift)){
    INVALID("No rank offset makes every rank access %s data that its node did not write or access in an earlier phase (%d nodes, %d-%d procs per node%s); rank-offset %d gives rank shift %d, earlier rank shifts %s\n",
            dataset, t->nnodes, t->ppn_min, t->ppn_max,
            t->nnodes < 4 ? ", a valid run needs at least 4 nodes" : "",
            v, shift, earlier);
  }
  if(h->nshift < IO500_RANK_OFFSET_MAX_SHIFTS){
    h->shift[h->nshift++] = shift;
    hist_save(h);
  }
}

int u_rank_offset_phase(char const * dataset, io500_offset_tool_t tool, int const * mult, int nmult, int dflt){
  int v = u_rank_offset_draw(dataset, tool, mult, nmult, 0, dflt);
  u_rank_offset_check(dataset, tool, v, mult[0]);
  return v;
}

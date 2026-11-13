/*
 * Test the read-back rank offset selection with fake rank->node tables.
 * Each valid/invalid decision is compared with a direct model of how IOR
 * (-C -Q) and mdtest (-N) shift ranks.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include <io500-util.h>
#include <io500-rank-offset.h>

FILE* file_out;
FILE* out_logfile;

static u_node_table_t * table_new(int nranks, int const * node, int block_mapping){
  u_node_table_t * t = calloc(1, sizeof(*t));
  t->nranks = nranks;
  t->node = malloc(sizeof(int) * nranks);
  memcpy(t->node, node, sizeof(int) * nranks);
  int max = 0;
  for(int r = 0; r < nranks; r++){
    max = node[r] > max ? node[r] : max;
  }
  int * ppn = calloc(max + 1, sizeof(int));
  for(int r = 0; r < nranks; r++){
    ppn[node[r]]++;
  }
  t->ppn_min = nranks;
  for(int n = 0; n <= max; n++){
    if(ppn[n] == 0) continue;
    t->nnodes++;
    t->ppn_min = ppn[n] < t->ppn_min ? ppn[n] : t->ppn_min;
    t->ppn_max = ppn[n] > t->ppn_max ? ppn[n] : t->ppn_max;
  }
  t->ppn0 = ppn[node[0]];
  t->tool_nnodes = t->nnodes;
  t->block_mapping = block_mapping;
  free(ppn);
  return t;
}

static void table_free(u_node_table_t * t){
  free(t->node);
  free(t);
}

/* ranks 0..ppn-1 on node 0, and so on (mpirun --map-by slot) */
static u_node_table_t * table_slot(int nnodes, int ppn){
  int n = nnodes * ppn;
  int * node = malloc(sizeof(int) * n);
  for(int r = 0; r < n; r++){
    node[r] = r / ppn;
  }
  u_node_table_t * t = table_new(n, node, ppn > 1);
  free(node);
  return t;
}

/* rank r on node (r / chunk) % nnodes (--map-by node is chunk 1) */
static u_node_table_t * table_cyclic(int nnodes, int ppn, int chunk){
  int n = nnodes * ppn;
  int * node = malloc(sizeof(int) * n);
  for(int r = 0; r < n; r++){
    node[r] = (r / chunk) % nnodes;
  }
  u_node_table_t * t = table_new(n, node, chunk > 1 || nnodes == 1);
  free(node);
  return t;
}

/* what IOR and mdtest do, written out independently of rank_offset.c */
static int reference_peer(u_node_table_t const * t, io500_offset_tool_t tool, int v, int k, int r){
  long long n = t->nranks;
  long long shift = 1;
  if(t->block_mapping && (tool == IO500_OFFSET_IOR || t->tool_nnodes > 1)){
    shift = t->ppn0;
  }
  if(tool == IO500_OFFSET_IOR){
    long long rank_offset = (v * shift) % n;  /* ior.c: rankOffset */
    return (int)((r + rank_offset) % n);      /* ior.c: pretendRank */
  }
  long long nstride = v * shift;             /* mdtest.c: o.nstride *= ppn0 */
  return (int)((r + k * nstride) % n);       /* mdtest.c: (rank + k * nstride) % i */
}

/*
 * Phase i of a data set uses value v[i] with multiplier k[i], and each rank
 * accesses the data of the rank reference_peer() names.  No rank may be on
 * a node that wrote that data or accessed it in an earlier phase.  Returns
 * the index of the first phase that breaks this, or nphase.
 */
static int reference_seq(u_node_table_t const * t, io500_offset_tool_t tool, int const * v, int const * k, int nphase){
  int n = t->nranks;
  int w = nphase + 1;
  int * seen = malloc(sizeof(int) * n * w);  /* nodes that touched each rank's data */
  int bad = nphase;

  for(int r = 0; r < n; r++){
    seen[r * w] = t->node[r];
  }
  for(int i = 0; i < nphase && bad == nphase; i++){
    for(int r = 0; r < n && bad == nphase; r++){
      int d = reference_peer(t, tool, v[i], k[i], r);
      for(int j = 0; j <= i; j++){
        if(seen[d * w + j] == t->node[r]){
          bad = i;
        }
      }
    }
    for(int r = 0; r < n; r++){
      seen[reference_peer(t, tool, v[i], k[i], r) * w + i + 1] = t->node[r];
    }
  }
  free(seen);
  return bad;
}

static int reference_valid(u_node_table_t const * t, io500_offset_tool_t tool, int v, int const * mult, int nmult){
  int vs[8];
  for(int i = 0; i < nmult; i++){
    vs[i] = v;
  }
  return reference_seq(t, tool, vs, mult, nmult) == nmult;
}

/* what rank_offset.c decides for the same value in all phases */
static int impl_valid(u_node_table_t const * t, io500_offset_tool_t tool, int v, int const * mult, int nmult){
  int prev[8] = {0};
  for(int i = 0; i < nmult; i++){
    int shift = u_rank_offset_shift(t, tool, v, mult[i]);
    if(! u_rank_offset_uncached(t, shift, prev, i + 1)){
      return 0;
    }
    prev[i + 1] = shift;
  }
  return 1;
}

static int writer[] = {0};

static int tests;

/*
 * Check every v against the reference, and that the random choice only
 * returns valid values and finds each of them.  Returns the number of
 * valid values.
 */
static int check(char const * name, u_node_table_t const * t, io500_offset_tool_t tool, int const * mult, int nmult, int expect_valid){
  int n = t->nranks;
  int valid = 0;
  char * is_valid = calloc(2 * n + 1, 1);
  char * seen = calloc(2 * n + 1, 1);

  for(int v = 1; v <= 2 * n; v++){
    int ref = reference_valid(t, tool, v, mult, nmult);
    int got = impl_valid(t, tool, v, mult, nmult);
    if(nmult == 1 && got != u_rank_offset_cross_node(t, tool, v, mult, nmult)){
      printf("FAIL %s: v=%d cross_node and uncached differ\n", name, v);
      exit(1);
    }
    if(ref != got){
      printf("FAIL %s: v=%d reference %d got %d\n", name, v, ref, got);
      exit(1);
    }
    is_valid[v] = ref;
  }
  /* count the distinct valid rank shifts, which is what the choice covers */
  int * shift_seen = calloc(n, sizeof(int));
  for(int v = 1; v <= 2 * n; v++){
    if(is_valid[v]){
      int s = u_rank_offset_shift(t, tool, v, 1);
      if(! shift_seen[s]){
        shift_seen[s] = 1;
        valid++;
      }
    }
  }
  uint64_t seed = 42;
  int found = 0;
  for(int i = 0; i < 2000; i++){
    int v = u_rank_offset_choose(t, tool, writer, 1, mult, nmult, 1, & seed);
    if(v == -1){
      break;
    }
    if(v < 1 || v > 2 * n || (! is_valid[v] && valid > 0)){
      printf("FAIL %s: chose invalid v=%d\n", name, v);
      exit(1);
    }
    /* with no valid value for all phases, it must still suit the first */
    if(! is_valid[v] && ! impl_valid(t, tool, v, mult, 1)){
      printf("FAIL %s: fallback v=%d is invalid for the first phase\n", name, v);
      exit(1);
    }
    if(is_valid[v] && ! seen[v]){
      seen[v] = 1;
      found++;
    }
  }
  if(valid > 0 && found != valid){
    printf("FAIL %s: random choice found %d of %d valid values\n", name, found, valid);
    exit(1);
  }
  if(expect_valid >= 0 && valid != expect_valid){
    printf("FAIL %s: %d valid values, expected %d\n", name, valid, expect_valid);
    exit(1);
  }
  printf("ok   %-48s %5d ranks %4d nodes ppn %d-%d: %d valid:", name, n, t->nnodes, t->ppn_min, t->ppn_max, valid);
  int shown = 0;
  for(int v = 1; v <= 2 * n && shown < 8; v++){
    if(seen[v]){
      printf(" %d", v);
      shown++;
    }
  }
  printf("%s\n", valid > shown ? " ..." : "");
  free(shift_seen);
  free(is_valid);
  free(seen);
  tests++;
  return valid;
}

/*
 * Run the phases of one data set as io500 does: a fresh choice at the
 * start of each phase, given the shifts of the earlier ones and the
 * multipliers of the later ones.  Every value chosen must be valid for its
 * phase according to the reference.  Returns 1 if all phases got one.
 */
static int run_seq(char const * name, u_node_table_t const * t, io500_offset_tool_t tool, int const * k, int nphase, uint64_t * seed){
  int prev[8] = {0};
  int v[8];

  for(int i = 0; i < nphase; i++){
    v[i] = u_rank_offset_choose(t, tool, prev, i + 1, k + i, nphase - i, 0, seed);
    if(v[i] < 0){
      return 0;
    }
    if(reference_seq(t, tool, v, k, i + 1) != i + 1){
      printf("FAIL %s: phase %d chose invalid v=%d\n", name, i, v[i]);
      exit(1);
    }
    prev[i + 1] = u_rank_offset_shift(t, tool, v[i], k[i]);
  }
  return 1;
}

/* every run of the phases gets valid values, or none does */
static void check_seq(char const * name, u_node_table_t const * t, io500_offset_tool_t tool, int const * k, int nphase, int expect){
  uint64_t seed = 42;
  int ok = 0;
  int runs = 200;

  for(int i = 0; i < runs; i++){
    ok += run_seq(name, t, tool, k, nphase, & seed);
  }
  if(ok != (expect ? runs : 0)){
    printf("FAIL %s: %d of %d runs got valid values for all phases\n", name, ok, runs);
    exit(1);
  }
  printf("ok   %-48s %5d ranks %4d nodes ppn %d-%d: %s\n", name, t->nranks, t->nnodes, t->ppn_min, t->ppn_max, expect ? "all phases valid" : "impossible");
  tests++;
}

int main(void){
  file_out = stdout;
  int k1[] = {1}, k2[] = {2}, k3[] = {3}, easy[] = {1, 3};
  int hard[] = {1, 2, 3}, ior_easy[] = {1, 1};
  u_node_table_t * t;

  /* --map-by slot: IOR and mdtest shift by whole nodes */
  t = table_slot(4, 4);
  check("slot 4x4 ior", t, IO500_OFFSET_IOR, k1, 1, 3);
  check("slot 4x4 mdtest stat", t, IO500_OFFSET_MDTEST, k1, 1, 3);
  check("slot 4x4 mdtest read (k=2)", t, IO500_OFFSET_MDTEST, k2, 1, 2);
  /* delete must also avoid the stat node: only odd node shifts */
  check("slot 4x4 mdtest-easy stat+delete", t, IO500_OFFSET_MDTEST, easy, 2, 2);
  check_seq("slot 4x4 mdtest-hard stat, read, delete", t, IO500_OFFSET_MDTEST, hard, 3, 1);
  check_seq("slot 4x4 ior-easy read, rnd4K read", t, IO500_OFFSET_IOR, ior_easy, 2, 1);
  table_free(t);

  /*
   * Odd ppn: mdtest-hard-read can only shift by 2 nodes, so stat must not
   * take that shift.  The look-ahead must avoid this dead end.
   */
  t = table_slot(4, 3);
  check_seq("slot 4x3 mdtest-hard stat, read, delete", t, IO500_OFFSET_MDTEST, hard, 3, 1);
  {
    int prev[] = {0, u_rank_offset_shift(t, IO500_OFFSET_MDTEST, 2, 1)};
    uint64_t seed = 1;
    assert(u_rank_offset_choose(t, IO500_OFFSET_MDTEST, prev, 2, hard + 1, 2, 0, & seed) == -1);
  }
  table_free(t);
  t = table_slot(5, 4);
  check("slot 5x4 mdtest-easy stat+delete", t, IO500_OFFSET_MDTEST, easy, 2, 4);
  check_seq("slot 5x4 mdtest-hard stat, read, delete", t, IO500_OFFSET_MDTEST, hard, 3, 1);
  table_free(t);
  t = table_cyclic(4, 4, 1);
  check_seq("cyclic 4x4 mdtest-hard stat, read, delete", t, IO500_OFFSET_MDTEST, hard, 3, 1);
  table_free(t);

  /* fewer nodes than the phases that access a data set */
  t = table_slot(3, 4);
  check_seq("slot 3x4 mdtest-hard (impossible)", t, IO500_OFFSET_MDTEST, hard, 3, 0);
  check_seq("slot 3x4 ior-easy read, rnd4K read", t, IO500_OFFSET_IOR, ior_easy, 2, 1);
  table_free(t);
  t = table_slot(2, 4);
  check_seq("slot 2x4 ior-easy read, rnd4K read (impossible)", t, IO500_OFFSET_IOR, ior_easy, 2, 0);
  check_seq("slot 2x4 ior-hard read", t, IO500_OFFSET_IOR, k1, 1, 1);
  table_free(t);

  /* --map-by node with nodes == ppn: v = ppn (the slot shift) is local */
  t = table_cyclic(4, 4, 1);
  check("cyclic 4x4 ior", t, IO500_OFFSET_IOR, k1, 1, 12);
  assert(! u_rank_offset_cross_node(t, IO500_OFFSET_IOR, 4, k1, 1));
  assert(! u_rank_offset_cross_node(t, IO500_OFFSET_IOR, 8, k1, 1));
  check("cyclic 4x4 mdtest read (k=2)", t, IO500_OFFSET_MDTEST, k2, 1, -1);
  assert(! u_rank_offset_cross_node(t, IO500_OFFSET_MDTEST, 2, k2, 1));
  table_free(t);

  /* --map-by ppr:2:node style: rank 1 is local to rank 0 */
  t = table_cyclic(4, 4, 2);
  check("2-chunk cyclic 4x4 ior", t, IO500_OFFSET_IOR, k1, 1, 2);
  assert(! u_rank_offset_cross_node(t, IO500_OFFSET_IOR, 2, k1, 1));
  table_free(t);

  /* uneven nodes: 3, 3 and 2 ranks */
  {
    int node[] = {0, 0, 0, 1, 1, 1, 2, 2};
    t = table_new(8, node, 1);
    check("uneven 3+3+2 ior", t, IO500_OFFSET_IOR, k1, 1, -1);
    check("uneven 3+3+2 mdtest delete (k=3)", t, IO500_OFFSET_MDTEST, k3, 1, -1);
    table_free(t);
  }
  /* uneven with no cross-node offset: 3 ranks cannot all read from 1 */
  {
    int node[] = {0, 0, 0, 1};
    t = table_new(4, node, 1);
    check("uneven 3+1 ior (impossible)", t, IO500_OFFSET_IOR, k1, 1, 0);
    table_free(t);
  }
  /* rank 0's node smaller than the others */
  {
    int node[] = {0, 0, 1, 1, 1, 2, 2, 2};
    t = table_new(8, node, 1);
    check("uneven 2+3+3 ior", t, IO500_OFFSET_IOR, k1, 1, -1);
    table_free(t);
  }

  /* single node: nothing is cross-node */
  t = table_slot(1, 8);
  check("single node ior", t, IO500_OFFSET_IOR, k1, 1, 0);
  check("single node mdtest", t, IO500_OFFSET_MDTEST, k1, 1, 0);
  check_seq("single node ior-hard read (impossible)", t, IO500_OFFSET_IOR, k1, 1, 0);
  table_free(t);
  t = table_slot(1, 1);
  check("single rank", t, IO500_OFFSET_IOR, k1, 1, 0);
  table_free(t);

  /* known limits of mdtest -N: k * stride cannot leave the node */
  t = table_slot(2, 4);
  check("slot 2x4 mdtest read (k=2, impossible)", t, IO500_OFFSET_MDTEST, k2, 1, 0);
  check("slot 2x4 ior", t, IO500_OFFSET_IOR, k1, 1, 1);
  table_free(t);
  t = table_cyclic(2, 4, 1);
  check("cyclic 2x4 mdtest read (k=2, impossible)", t, IO500_OFFSET_MDTEST, k2, 1, 0);
  table_free(t);
  t = table_slot(3, 4);
  check("slot 3x4 mdtest delete (k=3, impossible)", t, IO500_OFFSET_MDTEST, k3, 1, 0);
  check("slot 3x4 mdtest-easy (impossible)", t, IO500_OFFSET_MDTEST, easy, 2, 0);
  table_free(t);

  /* IOR_FAKE_TASK_PER_NODES changes the unit but not the real nodes */
  t = table_slot(4, 4);
  t->ppn0 = 2;
  check("slot 4x4, IOR thinks ppn 2", t, IO500_OFFSET_IOR, k1, 1, 5);
  table_free(t);

  /* overflow guard */
  t = table_slot(2, 4);
  assert(u_rank_offset_shift(t, IO500_OFFSET_IOR, 1 << 30, 1) == -1);
  table_free(t);

  /* large run: the choice stays fast */
  {
    t = table_slot(4096, 64);
    uint64_t seed = 7;
    clock_t start = clock();
    int v = 0;
    for(int i = 0; i < 10; i++){
      v = u_rank_offset_choose(t, IO500_OFFSET_IOR, writer, 1, k1, 1, 0, & seed);
      assert(v > 0 && u_rank_offset_cross_node(t, IO500_OFFSET_IOR, v, k1, 1));
      v = u_rank_offset_choose(t, IO500_OFFSET_MDTEST, writer, 1, k2, 1, 0, & seed);
      assert(v > 0 && u_rank_offset_cross_node(t, IO500_OFFSET_MDTEST, v, k2, 1));
      assert(run_seq("slot 4096x64 mdtest-hard", t, IO500_OFFSET_MDTEST, hard, 3, & seed));
    }
    printf("ok   slot 4096x64: 10 x (2 choices + mdtest-hard sequence) in %.3fs (last v=%d)\n", (double)(clock() - start) / CLOCKS_PER_SEC, v);
    table_free(t);
  }
  /* large impossible run: the fallback scan is bounded */
  {
    t = table_slot(2, 65536);
    uint64_t seed = 7;
    clock_t start = clock();
    assert(u_rank_offset_choose(t, IO500_OFFSET_MDTEST, writer, 1, k2, 1, 0, & seed) == -1);
    printf("ok   slot 2x65536 mdtest k=2: no value, %.3fs\n", (double)(clock() - start) / CLOCKS_PER_SEC);
    table_free(t);
  }
  /* large run that cannot be valid: the look-ahead is bounded too */
  {
    t = table_slot(3, 65536);
    uint64_t seed = 7;
    clock_t start = clock();
    assert(! run_seq("slot 3x65536 mdtest-hard", t, IO500_OFFSET_MDTEST, hard, 3, & seed));
    printf("ok   slot 3x65536 mdtest-hard: not all phases valid, %.3fs\n", (double)(clock() - start) / CLOCKS_PER_SEC);
    table_free(t);
  }

  printf("%d table checks passed\n", tests);
  return 0;
}

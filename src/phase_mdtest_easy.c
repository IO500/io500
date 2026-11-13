#include <sys/stat.h>
#include <unistd.h>

#include <mdtest.h>

#include <io500-phase.h>
#include <phase_mdtest.h>
#include <io500-rank-offset.h>

opt_mdtest_easy mdtest_easy_o;

static ini_option_t option[] = {
  {"API", "The API to be used", 0, INI_STRING, NULL, & mdtest_easy_o.g.api},
  {"n", "Files per proc", 0, INI_UINT64, "1000000", & mdtest_easy_o.g.files_per_proc},
  {"run", "Run this phase", 0, INI_BOOL, "TRUE", & mdtest_easy_o.g.run},
  {NULL} };

static void validate(void){

}

static void cleanup(void){
  if( ! opt.dry_run && opt.rank == 0){
    char filename[PATH_MAX];
    sprintf(filename, "%s/mdtest-easy.stonewall", opt.resdir);
    unlink(filename);
    sprintf(filename, "%s/mdtest-easy.rank-offset", opt.resdir);
    unlink(filename);
    u_rank_offset_cleanup("mdtest-easy");
    u_purge_datadir("mdtest-easy");
  }
}

/*
 * With -u, mdtest creates the files of rank r in the directory of rank
 * r + stride, and stat/delete look for them in the directory of rank
 * q + 2 * stride / q + 4 * stride, so the stride of the stat and delete
 * phases must equal the one used for the create.  It is drawn when the
 * create starts so that stat (rank + stride) and delete (rank + 3 * stride)
 * each access files that their node has not created or accessed before,
 * and kept for the later phases.
 */
static int easy_rank_offset = -1;

int mdtest_easy_rank_offset(int draw){
  char filename[PATH_MAX];
  sprintf(filename, "%s/mdtest-easy.rank-offset", opt.resdir);

  if(draw){
    int mult[] = {1, 3};
    u_rank_offset_written("mdtest-easy");
    easy_rank_offset = u_rank_offset_draw("mdtest-easy", IO500_OFFSET_MDTEST, mult, 2, 1, 1);
    PRINT_PAIR("rank-offset", "%d\n", easy_rank_offset);
    if(opt.rank == 0 && ! opt.dry_run){
      FILE * f = fopen(filename, "w");
      if(f){
        fprintf(f, "%d\n", easy_rank_offset);
        fclose(f);
      }else{
        WARNING("Couldn't write %s\n", filename);
      }
    }
    return easy_rank_offset;
  }
  if(easy_rank_offset < 0){
    /* the create ran in an earlier invocation using the same result dir */
    int v = 1;
    if(opt.rank == 0){
      FILE * f = fopen(filename, "r");
      if(! f || fscanf(f, "%d", & v) != 1 || v < 0){
        WARNING("Couldn't read %s, using rank-offset 1\n", filename);
        v = 1;
      }
      if(f){
        fclose(f);
      }
    }
    UMPI_CHECK(MPI_Bcast(& v, 1, MPI_INT, 0, MPI_COMM_WORLD));
    easy_rank_offset = v;
  }
  return easy_rank_offset;
}

void mdtest_easy_add_params(u_argv_t * argv, int rank_offset){
  opt_mdtest_easy d = mdtest_easy_o;

  u_argv_push(argv, "./mdtest");
  u_argv_push_printf(argv, "--dataPacketType=%s", opt.dataPacketType);
  if(opt.io_buffers_on_gpu){
    u_argv_push(argv, "--allocateBufferOnGPU");
  }
  u_argv_push(argv, "-n");	/* number of files per process */
  u_argv_push_printf(argv, "%"PRIu64, d.g.files_per_proc);
  u_argv_push(argv, "-u");	/* unique output directory per process */
  u_argv_push(argv, "-L");	/* create files only at leaf of tree */
  u_argv_push(argv, "-F");	/* create only files, not directories */
  u_argv_push(argv, "-P");	/* print both creation rate and elapsed time */
  u_argv_push(argv, "-G");
  int hash = u_phase_unique_random_number("mdtest-easy");
  u_argv_push_printf(argv, "%d", hash);
  u_argv_push(argv, "-N");	/* number of ranks between neighbours */
  u_argv_push_printf(argv, "%d", rank_offset);
  u_argv_push(argv, "-d");	/* output directory */
  u_argv_push_printf(argv, "%s/mdtest-easy", opt.datadir);
  u_argv_push(argv, "-x");	/* stonewall filename */
  u_argv_push_printf(argv, "%s/mdtest-easy.stonewall", opt.resdir);
}

u_phase_t p_mdtest_easy = {
  "mdtest-easy",
  IO500_PHASE_DUMMY,
  option,
  validate,
  NULL,
  .cleanup = cleanup,
};

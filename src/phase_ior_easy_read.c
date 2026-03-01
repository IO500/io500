#include <sys/stat.h>
#include <unistd.h>
#include <mpi.h>
#include <string.h>

#include <io500-phase.h>
#include <phase_ior.h>

typedef struct{
  int run;
  char * api;
  int direct;
  int iteration;

  char * command;
  IOR_point_t * res;
} opt_ior_easy_read;

static opt_ior_easy_read o;

static ini_option_t option[] = {
  {"API", "The API to be used", 0, INI_STRING, NULL, & o.api},
  {"run", "Run this phase", 0, INI_BOOL, "TRUE", & o.run},  
  {"direct", "Use direct IO (posix.odirect)", 0, INI_BOOL, NULL, & o.direct},
  {"iteration", "Number of iterations", 0, INI_INT, "1", & o.iteration},
  {NULL} };


static void validate(void){

}


static double run(void){
  opt_ior_easy d = ior_easy_o;

  u_argv_t * argv = u_argv_create();
  int direct = (o.direct != INI_UNSET_BOOL) ? o.direct : d.direct;
  ior_easy_add_params(argv, 1, direct);
  u_argv_push(argv, "-r");	/* read existing file */
  if (o.iteration != INI_UNSET_INT) {
    u_argv_push(argv, "-i");
    u_argv_push_printf(argv, "%d", o.iteration);
  }
  u_argv_push(argv, "-R");	/* verify data read */

  u_argv_push_default_if_set_api_options(argv, "-a", d.api, o.api);
  u_argv_push(argv, "-O");
  u_argv_push_printf(argv, "saveRankPerformanceDetailsCSV=%s/ior-easy-read.csv", opt.resdir);
//  u_argv_push_default_if_set(argv, "-U",		/* use hints file */
//			     d.hintsFileName, o.hintsFileName);

  o.command = u_flatten_argv(argv);

  PRINT_PAIR("exe", "%s\n", o.command);
  if(opt.dry_run || d.run == 0 || o.run == 0){
    u_argv_free(argv);
    return 0;
  }
  FILE * out = u_res_file_prep(p_ior_easy_read.name);
  return ior_process_read(argv, out, & o.res);
}


u_phase_t p_ior_easy_read = {
  "ior-easy-read",
  IO500_PHASE_READ,
  option,
  validate,
  run,
  0,
  .group = IO500_SCORE_BW,
};

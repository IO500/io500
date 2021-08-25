#include <sys/stat.h>
#include <unistd.h>
#include <mpi.h>

#include <io500-phase.h>
#include <phase_ior.h>

typedef struct{
  int run;
  char * api;

  char * command;
  IOR_point_t * res;
  int collective;
} opt_ior_hard_read;

static opt_ior_hard_read o;

static ini_option_t option[] = {
  {"API", "The API to be used", 0, INI_STRING, NULL, & o.api},
  {"collective", "Collective operation (for supported backends)", 0, INI_BOOL, NULL, & o.collective},
  {"run", "Run this phase", 0, INI_BOOL, "TRUE", & o.run},  
  {NULL} };


static void validate(void){

}


static double run(void){
  opt_ior_hard d = ior_hard_o;

  u_argv_t * argv = u_argv_create();
  ior_hard_add_params(argv);
  u_argv_push(argv, "-r");	/* read file */
  u_argv_push(argv, "-R");	/* verify data read */
  u_argv_push_default_if_set_bool(argv, "-c", d.collective, o.collective);
  u_argv_push_default_if_set_api_options(argv, "-a",	/* backend API */
					 d.api, o.api);
  u_argv_push(argv, "-O");
  u_argv_push_printf(argv, "saveRankPerformanceDetailsCSV=%s/ior-hard-read.csv", opt.resdir);
//  u_argv_push_default_if_set(argv, "-U",		/* use hints file */
//			     d.hintsFileName, o.hintsFileName);
//  u_argv_push_default_if_set_bool(argv, "--posix.odirect", d.odirect, o.odirect);

  o.command = u_flatten_argv(argv);

  PRINT_PAIR("exe", "%s\n", o.command);
  if(opt.dry_run || d.run == 0 || o.run == 0){
    u_argv_free(argv);
    return 0;
  }
  FILE * out = u_res_file_prep(p_ior_hard_read.name);
  return ior_process_read(argv, out, & o.res);
}


u_phase_t p_ior_hard_read = {
  "ior-hard-read",
  IO500_PHASE_READ,
  option,
  validate,
  run,
  0,
  .group = IO500_SCORE_BW,
};

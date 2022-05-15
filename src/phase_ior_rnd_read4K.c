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
} opt_ior_rnd_read;

static opt_ior_rnd_read o;

static ini_option_t option[] = {
  {"API", "The API to be used", 0, INI_STRING, NULL, & o.api},
  {"run", "Run this phase", 0, INI_BOOL, "TRUE", & o.run},  
  {NULL} };


static void validate(void){

}


static double run(void){
  opt_ior_rnd d = ior_rnd4K_o;

  u_argv_t * argv = u_argv_create();
  ior_rnd4K_add_params(argv);
  u_argv_push(argv, "-r");
  u_argv_push(argv, "-R");
  u_argv_push_default_if_set_api_options(argv, "-a", d.api, o.api);
  u_argv_push(argv, "-O");
  u_argv_push_printf(argv, "saveRankPerformanceDetailsCSV=%s/ior-rnd4K-read.csv", opt.resdir);
  
  o.command = u_flatten_argv(argv);

  PRINT_PAIR("exe", "%s\n", o.command);
  if(opt.dry_run || d.run == 0 || o.run == 0){
    u_argv_free(argv);
    return 0;
  }
  FILE * out = u_res_file_prep(p_ior_rnd4K_read.name, opt.rank);
  return ior_process_read(argv, out, & o.res, MPI_COMM_WORLD);
}


u_phase_t p_ior_rnd4K_read = {
  "ior-rnd4K-read",
  IO500_PHASE_READ | IO500_PHASE_FLAG_OPTIONAL,
  option,
  validate,
  run,
  0,
  .group = IO500_SCORE_BW,
};

#include <sys/stat.h>
#include <unistd.h>
#include <mpi.h>

#include <io500-phase.h>
#include <phase_mdworkbench.h>

typedef struct{
  int no_run;
  char * command;
  mdworkbench_results_t * res;
} opt_mdworkbench_delete;

static opt_mdworkbench_delete o;

static ini_option_t option[] = {
  {"noRun", "Disable running of this phase", 0, INI_BOOL, NULL, & o.no_run},
  {NULL} };

static double run(void){
  opt_mdworkbench d = mdworkbench_o;

  u_argv_t * argv = u_argv_create();
  mdworkbench_add_params(argv);
  u_argv_push(argv, "-3");

  o.command = u_flatten_argv(argv);
  PRINT_PAIR("exe", "%s\n", o.command);
  if(opt.dry_run || d.no_run == 1){
    u_argv_free(argv);
    return 0;
  }
  FILE * out = u_res_file_prep(p_mdworkbench_delete.name);
  mdworkbench_process(argv, out, & o.res);

  return 0;
}


u_phase_t p_mdworkbench_delete = {
  "mdworkbench-delete",
  IO500_PHASE_WRITE,
  option,
  NULL,
  run,
  0,
  .group = IO500_NO_SCORE,
};

#include <sys/stat.h>
#include <unistd.h>
#include <mpi.h>

#include <io500-phase.h>
#include <phase_mdworkbench.h>

typedef struct{
  int run;
  char * command;
  mdworkbench_results_t * res;
} opt_mdworkbench_create;

static opt_mdworkbench_create o;

static ini_option_t option[] = {
  {"run", "Run this phase", 0, INI_BOOL, "TRUE", & o.run},
  {NULL} };


static double run(void){
  opt_mdworkbench d = mdworkbench_o;

  u_argv_t * argv = u_argv_create();
  mdworkbench_add_params(argv, 1);
  u_argv_push(argv, "-1");

  o.command = u_flatten_argv(argv);
  PRINT_PAIR("exe", "%s\n", o.command);
  if(opt.dry_run || d.run == 0){
    u_argv_free(argv);
    return 0;
  }
  FILE * out = u_res_file_prep(p_mdworkbench_create.name);
  mdworkbench_process(argv, out, & o.res);
  PRINT_PAIR("maxOpTime", "%f\n", o.res->result[0].max_op_time);

  double rate = o.res->result[0].rate;
  return rate / 1000.0;
}


u_phase_t p_mdworkbench_create = {
  "mdworkbench-create",
  IO500_PHASE_WRITE | IO500_PHASE_FLAG_OPTIONAL ,
  option,
  NULL,
  run,
  0,
  .group = IO500_NO_SCORE,
};

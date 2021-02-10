#include <sys/stat.h>
#include <unistd.h>
#include <mpi.h>

#include <io500-phase.h>
#include <phase_mdworkbench.h>

typedef struct{
  int run;
  char * command;
  mdworkbench_results_t * res;
} opt_mdworkbench_bench;

static opt_mdworkbench_bench o;

static ini_option_t option[] = {
  {"run", "Run this phase", 0, INI_BOOL, "TRUE", & o.run},
  {NULL} };


static double run(void){
  opt_mdworkbench d = mdworkbench_o;

  u_argv_t * argv = u_argv_create();
  mdworkbench_add_params(argv, 0);
  u_argv_push(argv, "-2");
  u_argv_push(argv, "-R=2");
  u_argv_push(argv, "-X"); /* turn read verification on */

  o.command = u_flatten_argv(argv);
  PRINT_PAIR("exe", "%s\n", o.command);
  if(opt.dry_run || d.run == 0){
    u_argv_free(argv);
    return 0;
  }
  FILE * out = u_res_file_prep(p_mdworkbench_bench.name);
  mdworkbench_process(argv, out, & o.res);
  if(o.res->count != 2){
    INVALID("During the md-workbench phase not two iterations are performed but %d This invalidates your run.\n", o.res->count);
    return 0.0;
  }

  double rate = o.res->result[1].rate;
  if(   o.res->result[0].rate       < rate * 0.5
     || o.res->result[0].rate * 0.5 > rate){
    WARNING("The results of both md-workbench differs by 2x.\n");
  }
  PRINT_PAIR("maxOpTime", "%f\n", o.res->result[1].max_op_time);
  PRINT_PAIR("scoreIteration0", "%f\n", o.res->result[0].rate / 1000.0);
  PRINT_PAIR("maxOpTime0", "%f\n", o.res->result[0].max_op_time);
  return rate / 1000.0;
}


u_phase_t p_mdworkbench_bench = {
  "mdworkbench-bench",
  IO500_PHASE_WRITE | IO500_PHASE_FLAG_OPTIONAL,
  option,
  NULL,
  run,
  0,
  .group = IO500_SCORE_MD,
};

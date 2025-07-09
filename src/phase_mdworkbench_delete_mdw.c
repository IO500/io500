#include <sys/stat.h>
#include <unistd.h>
#include <mpi.h>

#include <io500-phase.h>
#include <phase_mdworkbench.h>

typedef struct{
  int run;
  char * command;
  mdworkbench_results_t * res;
} opt_mdworkbench_delete;

static opt_mdworkbench_delete o;

static ini_option_t option[] = {
  {"run", "Run this phase", 0, INI_BOOL, "TRUE", & o.run},
  {NULL} };

static void validate(void){
}

static double run(void){
  opt_mdworkbench d = mdworkbench_o;
  u_argv_t * argv = u_argv_create();
  mdworkbench_add_params(argv, 0);
  u_argv_push(argv, "-3");
  o.command = u_flatten_argv(argv);
  
  if(opt.dry_run || d.run == 0){
    return 0;
  }  
  FILE * out = u_res_file_prep(p_mdworkbench_delete.name, opt.rank);
  mdworkbench_process(argv, out, & o.res, MPI_COMM_WORLD);
  PRINT_PAIR("maxOpTime", "%f\n", o.res->result[0].max_op_time);
  
  double rate = o.res->result[0].rate;
  return rate / 1000.0;    
}


u_phase_t p_mdworkbench_delete = {
  "mdworkbench-delete",
  IO500_PHASE_REMOVE | IO500_PHASE_FLAG_OPTIONAL,
  option,
  validate,
  run,
  0,
  .group = IO500_NO_SCORE,
};

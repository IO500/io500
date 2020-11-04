#include <sys/stat.h>
#include <unistd.h>

#include <phase_mdworkbench.h>
#include <io500-phase.h>

opt_mdworkbench mdworkbench_o;

static ini_option_t option[] = {
  {"API", "The API to be used", 0, INI_STRING, "POSIX", & mdworkbench_o.api},
  {"waitingTime", "Waiting time of an IO operation relative to runtime (1.0 is 100%%)", 0, INI_FLOAT, "0.0", & mdworkbench_o.waiting_time},
  {"noRun", "Disable running of this phase", 0, INI_BOOL, NULL, & mdworkbench_o.no_run},
  {"verbosity", "The verbosity level", 0, INI_INT, 0, & mdworkbench_o.verbosity},
  {NULL} };


double mdworkbench_process(u_argv_t * argv, FILE * out, phase_stat_t ** res_out){
  phase_stat_t * res = md_workbench_run(argv->size, argv->vector, MPI_COMM_WORLD, out);
  u_res_file_close(out);
  u_argv_free(argv);

  if(res->t){
  //  INVALID("Errors (%d) occured during phase in IOR. This invalidates your run.\n", res->errors);
  }
  double tp = 1;
  return tp;
}

void mdworkbench_add_params(u_argv_t * argv){
  opt_mdworkbench d = mdworkbench_o;

  u_argv_push(argv, "./md-workbench");
  for(int i=0; i < mdworkbench_o.verbosity; i++){
    u_argv_push(argv, "-v");
  }
  u_argv_push(argv, "-a");
  u_argv_push(argv, d.api);
  u_argv_push_printf(argv, "-o=%s/mdworkbench", opt.datadir);
  u_argv_push_printf(argv, "-t=%f", d.waiting_time);
  u_argv_push(argv, "-R=1");
  u_argv_push(argv, "-O=1");
  u_argv_push(argv, "-D=10");
  u_argv_push_printf(argv, "--run-info-file=%s/mdworkbench.status", opt.resdir );
}

u_phase_t p_mdworkbench = {
  "mdworkbench",
  IO500_PHASE_DUMMY,
  option,
  NULL,
  NULL,
  NULL,
};

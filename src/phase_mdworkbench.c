#include <sys/stat.h>
#include <unistd.h>

#include <phase_mdworkbench.h>
#include <phase_mdtest.h>
#include <io500-phase.h>

/*
This phase adjusts the precreate based on the rate of mdtest.
It attempts to run pre-create for 60s.
*/

opt_mdworkbench mdworkbench_o;

static ini_option_t option[] = {
  {"API", "The API to be used", 0, INI_STRING, "POSIX", & mdworkbench_o.api},
  {"waitingTime", "Waiting time of an IO operation relative to runtime (1.0 is 100%%)", 0, INI_FLOAT, "0.0", & mdworkbench_o.waiting_time},
  {"precreatePerSet", "Files to precreate per set (always 10 sets), this is normally dynamically determined", 0, INI_UINT64, NULL, & mdworkbench_o.precreate_per_set},
  {"filesPerProc", "Files to run per iteration and set (always 10 sets), this is normally dynamically determined", 0, INI_UINT64, NULL, & mdworkbench_o.files_per_proc},
  {"noRun", "Disable running of this phase", 0, INI_BOOL, NULL, & mdworkbench_o.no_run},
  {"verbosity", "The verbosity level", 0, INI_INT, 0, & mdworkbench_o.verbosity},
  {NULL} };

static void validate(void){
  opt_mdworkbench d = mdworkbench_o;

  if(d.precreate_per_set != INI_UNSET_UINT64 || d.files_per_proc != INI_UNSET_UINT64){
    if(d.precreate_per_set == INI_UNSET_UINT64 || d.files_per_proc == INI_UNSET_UINT64){
      FATAL("MDWorkbench, must set either both options or none of: filesPerProc and precreatePerSet\n");
    }
  }
}


void mdworkbench_process(u_argv_t * argv, FILE * out, mdworkbench_results_t ** res_out){
  mdworkbench_results_t * res = md_workbench_run(argv->size, argv->vector, MPI_COMM_WORLD, out);
  u_res_file_close(out);
  u_argv_free(argv);

  if(res->errors != 0){
    INVALID("Errors (%d) occured during the md-workbench phase. This invalidates your run.\n", res->errors);
  }
  *res_out = res;
}



void mdworkbench_add_params(u_argv_t * argv){
  opt_mdworkbench d = mdworkbench_o;

  u_argv_push(argv, "./md-workbench");
  for(int i=0; i < mdworkbench_o.verbosity; i++){
    u_argv_push(argv, "-v");
  }
  u_argv_push(argv, "--process-reports");
  u_argv_push(argv, "-a");
  u_argv_push(argv, d.api);
  u_argv_push_printf(argv, "-o=%s/mdworkbench", opt.datadir);
  u_argv_push_printf(argv, "-t=%f", d.waiting_time);
  u_argv_push(argv, "-O=1");
  u_argv_push_printf(argv, "--run-info-file=%s/mdworkbench.status", opt.resdir );

  // determine the number of objects to precreate and the number of objects per iteration

  uint64_t precreate_per_set;
  uint64_t files_per_proc;

  if (d.precreate_per_set != INI_UNSET_UINT64 && d.files_per_proc != INI_UNSET_UINT64){
    precreate_per_set = d.precreate_per_set;
    files_per_proc = d.files_per_proc;
  }else{
    mdtest_generic_res* mdtest = mdtest_easy_write_get_result();
    if( mdtest->rate <= 0.0 ){
      WARNING("MDWorkbench uses the MDTest rates to determine suitable options but MDTest didn't run, will use a low (and sane) default instead\n");
      mdtest->rate = 1000.0;
    }
    // run for 60s
    int time = opt.stonewall < 60 ? opt.stonewall : 60;
    precreate_per_set = (uint64_t) (mdtest->rate * time * 1000) / 10;
    files_per_proc = precreate_per_set;
  }
  // we have 10 sets
  u_argv_push(argv, "-D=10");
  PRINT_PAIR("filesPerProc", "%"PRIu64"\n", files_per_proc);
  PRINT_PAIR("precreatePerSet", "%"PRIu64"\n", precreate_per_set);

  u_argv_push_printf(argv, "-P=%"PRIu64, precreate_per_set);
  u_argv_push_printf(argv, "-I=%"PRIu64, files_per_proc);
}

u_phase_t p_mdworkbench = {
  "mdworkbench",
  IO500_PHASE_DUMMY,
  option,
  validate,
  NULL,
  NULL,
};

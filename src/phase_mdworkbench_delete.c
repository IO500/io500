#include <sys/stat.h>
#include <unistd.h>
#include <mpi.h>

#include <io500-phase.h>
#include <phase_mdworkbench.h>
#include <pfind-options.h>
#include <phase_find.h>

typedef struct{
  int run;
  char * command;
  mdworkbench_results_t * res;
} opt_mdworkbench_delete;

static opt_mdworkbench_delete o;
static opt_find of;

static ini_option_t option[] = {
  {"run", "Run this phase", 0, INI_BOOL, "TRUE", & o.run},
  {"external-script", "Set to an external script to perform the find phase", 0, INI_STRING, NULL, & of.ext_find},
  {"external-mpi-args", "Startup arguments for external scripts, some MPI's may not support this!", 0, INI_STRING, "", & of.ext_mpi},
  {"external-extra-args", "Extra arguments for the external scripts", 0, INI_STRING, "", & of.ext_args},
  {"nproc", "Set the number of processes for pfind/the external script", 0, INI_UINT, NULL, & of.nproc},
  {NULL} };

static void validate(void){
}

static double run(void){
  opt_mdworkbench d = mdworkbench_o;
  u_argv_t * argv = u_argv_create();
  if(1){
    u_argv_push(argv, "./pfind");
    u_argv_push_printf(argv, "%s/mdworkbench", opt.datadir);  
    u_argv_push(argv, "-C");
    u_argv_push(argv, "-E");
    u_argv_push(argv, "-e");
    o.command = u_flatten_argv(argv);
    of.pfind_o = pfind_parse_args(argv->size, argv->vector, 0, MPI_COMM_WORLD);
  }else{
    mdworkbench_add_params(argv, 0);
    u_argv_push(argv, "-3");
    o.command = u_flatten_argv(argv);
  }  
  
  if(opt.dry_run || d.run == 0){
    return 0;
  }  
  if(1){
    if(of.ext_find){
      char args[PATH_MAX];
      sprintf(args, "%s/mdworkbench/ -E -e", opt.datadir);
      external_find_prepare_arguments(args, & of);
      return run_find(p_mdworkbench_delete.name, & of);
    }else{
      pfind_find_results_t * res = pfind_find(of.pfind_o);
      if(! res){
        WARNING("Pfind cleanup returned with an error.\n")
        return 0.0;
      }
      of.pfind_res = pfind_aggregrate_results(res);
      of.found_files = of.pfind_res->found_files;
      of.runtime = of.pfind_res->runtime;
      if(opt.rank == 0){
        PRINT_PAIR("deleted", "%"PRIu64"\n", of.found_files);
        return of.pfind_res->total_files / of.runtime / 1000;
      }
      return 0.0;
    }
  }else{
    FILE * out = u_res_file_prep(p_mdworkbench_delete.name, opt.rank);
    mdworkbench_process(argv, out, & o.res, MPI_COMM_WORLD);
    PRINT_PAIR("maxOpTime", "%f\n", o.res->result[0].max_op_time);
    
    double rate = o.res->result[0].rate;
    return rate / 1000.0;    
  }
  return 0.0;
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

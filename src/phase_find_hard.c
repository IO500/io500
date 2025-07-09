#include <sys/stat.h>
#include <unistd.h>
#include <mpi.h>
#include <string.h>
#include <errno.h>

#include <pfind-options.h>
#include <io500-phase.h>
#include "phase_find.h"

static opt_find of;

static double run(void){
  if(of.run == 0) return 0.0;
  return run_find("find-hard", & of);
}

static ini_option_t option[] = {
  {"external-script", "Set to an external script to perform the find phase", 0, INI_STRING, NULL, & of.ext_find},
  {"external-mpi-args", "Startup arguments for external scripts, some MPI's may not support this!", 0, INI_STRING, "", & of.ext_mpi},
  {"external-extra-args", "Extra arguments for the external scripts", 0, INI_STRING, "", & of.ext_args},
  {"nproc", "Set the number of processes for pfind/the external script", 0, INI_UINT, NULL, & of.nproc},
  {"run", "Run this phase", 0, INI_BOOL, "TRUE", & of.run},
  {"pfind-queue-length", "Pfind queue length", 0, INI_INT, "10000", & of.pfind_queue_length},
  {"pfind-steal-next", "Pfind Steal from next", 0, INI_BOOL, "FALSE", & of.pfind_steal_from_next},
  {"pfind-parallelize-single-dir-access-using-hashing", "Parallelize the readdir by using hashing. Your system must support this!", 0, INI_BOOL, "FALSE", &  of.pfind_par_single_dir_access_hash},
  {NULL} };

static void validate(void){
  if(of.run == 0) return;
  if(of.ext_find){ // runs with one process anyway
    char args[PATH_MAX];
    sprintf(args, "%s/mdtest-easy/ -newer %s/timestampfile -size 0c", opt.datadir, opt.resdir);
    external_find_prepare_arguments(args, & of);
  }else{
    u_argv_t * argv = u_argv_create();
    u_argv_push(argv, "./pfind");
    u_argv_push_printf(argv, "%s/mdtest-easy/", opt.datadir);
    u_argv_push(argv, "-size");
    u_argv_push(argv, "0c");
    u_argv_push(argv, "-r");
    u_argv_push_printf(argv, "%s", opt.datadir);

    pfind_prepare_arguments(argv, & of);
  }
}

u_phase_t p_find_hard = {
  "find-hard",
  IO500_PHASE_READ | IO500_PHASE_FLAG_OPTIONAL,
  option,
  validate,
  run,
  0,
  .group = IO500_SCORE_MD
};

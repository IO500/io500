#include <sys/stat.h>
#include <unistd.h>

#include <io500-phase.h>

typedef struct{
  char * ext_find;
  char * ext_args;
  char * ext_mpi;
  int nproc;
} opt_find;

static opt_find of;

static double run(void){
  int ret = 0;
  char arguments[1024];
  sprintf(arguments, "%s -newer %s/timestampfile -size 3901c -name \"*01*\"", opt.datadir, opt.datadir);

  char command[2048];
  char *p = command;

  double performance = 0;

  if(of.ext_find){
    p+= sprintf(p, "%s %s %s %s", of.ext_mpi, of.ext_find, of.ext_args, arguments);
    printf("exe=%s\n", command);
    if(! opt.dry_run){
      clock_t start = clock();

      FILE * fp = popen(command, "r");
      if (fp == NULL) {
        ERROR("Failed to run find command: \"%s\"\n", command);
        return -1;
      }
      char line[1024];
      uint64_t hits = 0;
      while (fgets(line, sizeof(line), fp) != NULL) {
        DEBUG_ALL("Found: %s", line);
        hits++;
      }
      ret = pclose(fp);
      double runtime = u_time_diff(start);
      performance = hits / runtime;

      printf("last-match=%s", line);

      if(ret != 0){
        opt.is_valid_run = 0;
        WARNING("Exit code != 0 from find command: \"%s\"\n", command);
      }
    }
    return performance;
  }
  if(of.nproc != 0){
    // pfind supports stonewalling timer -s, but ignore for now
  }
  return performance;
}

static ini_option_t option[] = {
  {"external-script", "Set to an external script to perform the find phase", 0, INI_STRING, NULL, & of.ext_find},
  {"external-extra-args", "Extra arguments for external scripts", 0, INI_STRING, "", & of.ext_args},
  {"external-mpi-args", "Startup arguments for external scripts", 0, INI_STRING, "", & of.ext_mpi},
  {"nproc", "Set the number of processes for pfind", 0, INI_UINT, NULL, & of.nproc},
  {NULL} };

static void validate(void){
  if(of.ext_find){
    struct stat sb;
    int ret = stat(of.ext_find, & sb);
    if(ret != 0){
      FATAL("Cannot check external-script %s\n", of.ext_find);
    }
    if(! (sb.st_mode & S_IXUSR) ){
      FATAL("The external-script must be a executable file %s\n", of.ext_find);
    }
  }
}

u_phase_t p_find = {
  "find",
  option,
  validate,
  run
};

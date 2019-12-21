#include <sys/stat.h>
#include <unistd.h>
#include <mpi.h>
#include <string.h>


#include <pfind-options.h>
#include <io500-phase.h>

typedef struct{
  char * ext_find;
  char * ext_args;
  char * ext_mpi;
  int nproc;

  char * command;

  pfind_options_t * pfind_o;
  pfind_find_results_t * pfind_res;

  uint64_t found_files;
  double runtime;
} opt_find;

static opt_find of;

static double run(void){
  int ret = 0;

  r0printf("exe=%s\n", of.command);
  r0printf("nproc=%d\n", of.nproc);

  if(opt.dry_run){
    return 0;
  }
  {
    char timestamp_file[2048];
    sprintf(timestamp_file, "%s/timestampfile", opt.datadir);
    FILE * f = fopen(timestamp_file, "r");
    if(! f){
      FATAL("Couldn't open timestampfile: %s\n", timestamp_file);
    }
    fclose(f);
  }

  if(! of.ext_find){
    // pfind supports stonewalling timer -s, but ignore for now
    of.pfind_res = pfind_find(of.pfind_o);
    of.found_files = of.pfind_res->found_files;
    of.runtime = of.pfind_res->runtime;

    r0printf("found=%"PRIu64"\n", of.found_files);
    r0printf("total-files=%"PRIu64"\n", of.pfind_res->total_files);
    return of.found_files / of.runtime / 1000;
  }
  // only one process runs the external find
  if(opt.rank != 0){
    MPI_Barrier(MPI_COMM_WORLD);
    return 0;
  }

  double performance;
  double start = GetTimeStamp();

  FILE * fp = popen(of.command, "r");
  if (fp == NULL) {
    ERROR("Failed to run find command: \"%s\"\n", of.command);
    return -1;
  }
  char line[1024];
  uint64_t hits = 0;
  *line = '\0';
  while (fgets(line, sizeof(line), fp) != NULL) {
    DEBUG_ALL("Found: %s", line);
    hits++;
  }
  ret = pclose(fp);
  double runtime = GetTimeStamp() - start;

  // support two semantics, the count only semantics
  if(*line != '\0'){
    line[strlen(line) - 1] = 0;
  }
  printf("last-output=\"%s\"\n", line);
  if(strstr(line, "MATCHED ") == line){
    // the script is supposed to output the number of files in this line
    char * ptr   = line + 8;
    char * left  = strtok(ptr, "/");
    char * right = strtok(NULL, "/");
    if(left == NULL || right == NULL || *left == 0 || * right == 0){
      FATAL("Invalid output from the external script, expected: MATCHED <NUM>/<NUM>\n");
    }
    hits = atoll(left);
    printf("total-files=%lld\n", atoll(right));
  }

  performance = hits / runtime / 1000;

  of.found_files = hits;
  of.runtime = runtime;

  printf("found=%"PRIu64"\n", of.found_files);

  if(ret != 0){
    opt.is_valid_run = 0;
    WARNING("Exit code != 0 from find command: \"%s\"\n", of.command);
  }
  MPI_Barrier(MPI_COMM_WORLD);
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
    char arguments[1024];
    sprintf(arguments, "%s -newer %s/timestampfile -size 3901c -name \"*01*\"", opt.datadir, opt.datadir);

    char command[2048];
    sprintf(command, "%s %s %s %s", of.ext_mpi, of.ext_find, of.ext_args, arguments);
    of.command = strdup(command);
    of.nproc = 1;
  }else{
    u_argv_t * argv = u_argv_create();
    u_argv_push(argv, "./pfind");
    u_argv_push(argv, opt.datadir);
    u_argv_push(argv, "-newer");
    u_argv_push_printf(argv, "%s/timestampfile", opt.datadir);
    u_argv_push(argv, "-size");
    u_argv_push(argv, "3901c");
    u_argv_push(argv, "-name");
    u_argv_push(argv, "*01*");
    u_argv_push(argv, "-C");

    of.command = u_flatten_argv(argv);

    MPI_Comm com = MPI_COMM_WORLD;
    if(of.nproc != 0){
      int color = opt.rank < of.nproc;
      int ret = MPI_Comm_split(MPI_COMM_WORLD, color, opt.rank, & com);
      MPI_Comm_size(com, & ret);
      DEBUG_INFO("Configuring pfind to run with %d procs\n", ret);
      if(color == 1 && of.nproc != ret){
        FATAL("Couldn't split rank for find into %d procs (got %d procs)\n", of.nproc, ret);
      }
    }

    of.pfind_o = pfind_parse_args(argv->size, argv->vector, 0, com);

    u_argv_free(argv);

    if(of.nproc != 0){
      MPI_Comm_free(& com);
    }
  }
}

u_phase_t p_find = {
  "find",
  option,
  validate,
  run,
  0
};

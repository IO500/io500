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
  int pfind_queue_length;
  int pfind_steal_from_next;

  uint64_t found_files;
  double runtime;
} opt_find;

static opt_find of;

static double run(void){
  int ret = 0;

  PRINT_PAIR("exe", "%s\n", of.command);
  if(of.nproc != INI_UNSET_UINT){
    PRINT_PAIR("nproc", "%d\n", of.nproc);
  }

  if(opt.dry_run){
    return 0;
  }
  {
    char timestamp_file[2048];
    sprintf(timestamp_file, "%s/timestampfile", opt.resdir);
    FILE * f = fopen(timestamp_file, "r");
    if(! f){
      FATAL("Couldn't open timestampfile: %s\n", timestamp_file);
    }
    fclose(f);
  }

  if(! of.ext_find){
    // pfind supports stonewalling timer -s, but ignore for now
    pfind_find_results_t * res = pfind_find(of.pfind_o);
    of.pfind_res = pfind_aggregrate_results(res);
    free(res);
    of.found_files = of.pfind_res->found_files;
    of.runtime = of.pfind_res->runtime;

    if( of.found_files == 0 ){
      WARNING("Find didn't find anything, this is likely invalid.\n")
      opt.is_valid_run = 0;
    }
    PRINT_PAIR("found", "%"PRIu64"\n", of.found_files);
    PRINT_PAIR("total-files", "%"PRIu64"\n", of.pfind_res->total_files);
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
  PRINT_PAIR("last-output", "\"%s\"\n", line);
  if(strstr(line, "MATCHED ") == line){
    // the script is supposed to output the number of files in this line
    char * ptr   = line + 8;
    char * left  = strtok(ptr, "/");
    char * right = strtok(NULL, "/");
    if(left == NULL || right == NULL || *left == 0 || * right == 0){
      FATAL("Invalid output from the external script, expected: MATCHED <NUM>/<NUM>\n");
    }
    hits = atoll(left);
    PRINT_PAIR("total-files", "%lld\n", atoll(right));
  }

  performance = hits / runtime / 1000;

  of.found_files = hits;
  of.runtime = runtime;

  if( of.found_files == 0 ){
    WARNING("Find didn't find anything, this is likely invalid.\n")
    opt.is_valid_run = 0;
  }

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
  {"pfind-queue-length", "Pfind queue length", 0, INI_INT, "10000", & of.pfind_queue_length},
  {"pfind-steal-next", "Pfind Steal from next", 0, INI_BOOL, "FALSE", & of.pfind_steal_from_next},
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
    sprintf(arguments, "%s -newer %s/timestampfile -size 3901c -name \"*01*\"", opt.datadir, opt.resdir);

    char command[2048];
    sprintf(command, "%s %s %s %s", of.ext_mpi, of.ext_find, of.ext_args, arguments);
    of.command = strdup(command);
    of.nproc = 1;
  }else{
    u_argv_t * argv = u_argv_create();
    u_argv_push(argv, "./pfind");
    u_argv_push(argv, opt.datadir);
    u_argv_push(argv, "-newer");
    u_argv_push_printf(argv, "%s/timestampfile", opt.resdir);
    u_argv_push(argv, "-size");
    u_argv_push(argv, "3901c");
    u_argv_push(argv, "-name");
    u_argv_push(argv, "*01*");
    u_argv_push(argv, "-C");
    if(of.pfind_steal_from_next){
      u_argv_push(argv, "-N");
    }
    u_argv_push(argv, "-q");
    u_argv_push_printf(argv, "%d", of.pfind_queue_length);

    of.command = u_flatten_argv(argv);

    MPI_Comm com = MPI_COMM_WORLD;
    if(of.nproc != INI_UNSET_UINT){
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

    if(of.nproc != INI_UNSET_UINT){
      MPI_Comm_free(& com);
    }
  }
}

u_phase_t p_find = {
  "find",
  IO500_PHASE_READ,
  option,
  validate,
  run,
  0,
  .group = IO500_SCORE_MD
};

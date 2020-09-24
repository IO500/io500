#include <sys/stat.h>
#include <unistd.h>

#include <io500-phase.h>
#include <phase_ior.h>

opt_ior_easy ior_easy_o;

static ini_option_t option[] = {
  {"API", "The API to be used", 0, INI_STRING, NULL, & ior_easy_o.api},
  {"posix.odirect", "Use ODirect", 0, INI_BOOL, NULL, & ior_easy_o.odirect},
  {"transferSize", "Transfer size", 0, INI_STRING, "2m", & ior_easy_o.transferSize},
  {"blockSize", "Block size; must be a multiple of transferSize", 0, INI_STRING, "9920000m", & ior_easy_o.blockSize},
  {"hintsFileName", "Filename for MPI hint file", 0, INI_STRING, NULL, & ior_easy_o.hintsFileName},
  {"filePerProc", "Create one file per process", 0, INI_BOOL, "TRUE", & ior_easy_o.filePerProc},
  {"uniqueDir", "Use unique directory per file per process", 0, INI_BOOL, "FALSE", & ior_easy_o.uniqueDir},
  {"noRun", "Disable running of this phase", 0, INI_BOOL, NULL, & ior_easy_o.no_run},
  {"verbosity", "The verbosity level", 0, INI_INT, 0, & ior_easy_o.verbosity},
  {NULL} };

static void validate(void){
  if(ior_easy_o.hintsFileName){
    struct stat sb;
    int ret = stat(ior_easy_o.hintsFileName, & sb);
    if(ret != 0){
      FATAL("Cannot check hintsFileName %s\n", ior_easy_o.hintsFileName);
    }
    if(! (sb.st_mode & S_IRUSR) ){
      FATAL("The hintsFileName must be a readable file %s\n", ior_easy_o.hintsFileName);
    }
  }
  u_create_datadir("ior-easy");
}

static void cleanup(void){
  if( ! opt.dry_run && opt.rank == 0){
    //u_purge_file("ior-easy/stonewall");

    if(! ior_easy_o.filePerProc){
      u_purge_file("ior-easy/ior_file_easy");
    }
  }
  if( ! opt.dry_run && ior_easy_o.filePerProc){
      char filename[2048];
      sprintf(filename, "ior-easy/ior_file_easy.%08d", opt.rank);
      u_purge_file(filename);
    }
  u_purge_datadir("ior-easy");
}

void ior_easy_add_params(u_argv_t * argv){
  opt_ior_easy d = ior_easy_o;

  u_argv_push(argv, "./ior");
  for(int i=0; i < ior_easy_o.verbosity; i++){
    u_argv_push(argv, "-v");
  }
  u_argv_push(argv, "-C");
  u_argv_push(argv, "-Q");
  u_argv_push(argv, "1");
  u_argv_push(argv, "-g");
  u_argv_push(argv, "-G");
  u_argv_push(argv, "271");
  u_argv_push(argv, "-k");
  u_argv_push(argv, "-e");
  u_argv_push(argv, "-o");
  u_argv_push_printf(argv, "%s/ior-easy/ior_file_easy", opt.datadir);
  u_argv_push(argv, "-O");
  u_argv_push_printf(argv, "stoneWallingStatusFile=%s/ior-easy.stonewall", opt.resdir);
  u_argv_push(argv, "-t");
  u_argv_push(argv, d.transferSize);
  u_argv_push(argv, "-b");
  u_argv_push(argv, d.blockSize);

  if(ior_easy_o.uniqueDir){
    u_argv_push(argv, "-u");
  }

  if(ior_easy_o.filePerProc){
    u_argv_push(argv, "-F");
  }

}

u_phase_t p_ior_easy = {
  "ior-easy",
  IO500_PHASE_DUMMY,
  option,
  validate,
  NULL,
  .cleanup = cleanup,
};

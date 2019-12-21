#include <sys/stat.h>
#include <unistd.h>

#include <io500-phase.h>
#include <phase_ior.h>

opt_ior_easy ior_easy_o;

static ini_option_t option[] = {
  {"API", "The API to be used", 0, INI_STRING, "POSIX", & ior_easy_o.api},
  {"posix.odirect", "Use ODirect", 0, INI_BOOL, NULL, & ior_easy_o.odirect},
  {"transferSize", "Transfer size", 0, INI_STRING, "2m", & ior_easy_o.transferSize},
  {"blockSize", "Block size; must be a multiple of transferSize", 0, INI_STRING, "9920000m", & ior_easy_o.blockSize},
  {"hintsFileName", "Filename for MPI hint file", 0, INI_STRING, NULL, & ior_easy_o.hintsFileName},
  {"filePerProc", "Create one file per process", 0, INI_BOOL, "TRUE", & ior_easy_o.filePerProc},
  {"noRun", "Disable running of this phase", 0, INI_BOOL, NULL, & ior_easy_o.no_run},
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
  if(opt.rank == 0){
    u_create_datadir("ior_easy");
  }
}

void ior_easy_add_params(u_argv_t * argv){
  opt_ior_easy d = ior_easy_o;

  u_argv_push(argv, "./ior");
  u_argv_push(argv, "-C");
  u_argv_push(argv, "-Q");
  u_argv_push(argv, "1");
  u_argv_push(argv, "-g");
  u_argv_push(argv, "-G");
  u_argv_push(argv, "271");
  u_argv_push(argv, "-k");
  u_argv_push(argv, "-e");
  u_argv_push(argv, "-o");
  u_argv_push_printf(argv, "%s/ior_easy/ior_file_easy", opt.datadir);
  u_argv_push(argv, "-O");
  u_argv_push_printf(argv, "stoneWallingStatusFile=%s/ior_easy/stonewall", opt.datadir );
  u_argv_push(argv, "-O");
  u_argv_push(argv, "stoneWallingWearOut=1");
  u_argv_push(argv, "-t");
  u_argv_push(argv, d.transferSize);
  u_argv_push(argv, "-b");
  u_argv_push(argv, d.blockSize);
}

u_phase_t p_ior_easy = {
  "ior-easy",
  option,
  validate,
  NULL
};

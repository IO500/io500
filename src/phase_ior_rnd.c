#include <sys/stat.h>
#include <unistd.h>

#include <phase_ior.h>
#include <io500-phase.h>

opt_ior_rnd ior_rnd_o;

static ini_option_t option[] = {
  {"API", "The API to be used", 0, INI_STRING, NULL, & ior_rnd_o.api},
  {"posix.odirect", "Use ODirect", 0, INI_BOOL, NULL, & ior_rnd_o.odirect},
  {"hintsFileName", "Filename for hints file", 0, INI_STRING, NULL, & ior_rnd_o.hintsFileName},
  {"segmentCount", "Number of segments", 0, INI_INT, "1000000000", & ior_rnd_o.segments},
  {"noRun", "Disable running of this phase", 0, INI_BOOL, NULL, & ior_rnd_o.no_run},
  {"verbosity", "The verbosity level", 0, INI_INT, 0, & ior_rnd_o.verbosity},
  {NULL} };



static void validate(void){
  if(ior_rnd_o.hintsFileName){
    struct stat sb;
    int ret = stat(ior_rnd_o.hintsFileName, & sb);
    if(ret != 0){
      FATAL("Cannot check hintsFileName %s\n", ior_rnd_o.hintsFileName);
    }
    if(! (sb.st_mode & S_IRUSR) ){
      FATAL("The hintsFileName must be a readable file %s\n", ior_rnd_o.hintsFileName);
    }
  }
  u_create_datadir("ior-rnd");
}

static void cleanup(void){
  if( ! opt.dry_run && opt.rank == 0){
    char filename[PATH_MAX];
    sprintf(filename, "%s/ior-rnd.stonewall", opt.resdir);
    unlink(filename);
  }
  if(opt.rank == 0){
    u_purge_datadir("ior-rnd");
  }
}

void ior_rnd_add_params(u_argv_t * argv){
  opt_ior_rnd d = ior_rnd_o;

  u_argv_push(argv, "./ior");
  for(int i=0; i < ior_rnd_o.verbosity; i++){
    u_argv_push(argv, "-v");
  }
  u_argv_push(argv, "-Q");
  u_argv_push(argv, "1");
  u_argv_push(argv, "-F");
  u_argv_push(argv, "-g");
  u_argv_push(argv, "-G");
  u_argv_push(argv, "4711");
  u_argv_push(argv, "-z");
  u_argv_push(argv, "--random-offset-seed=11");
  u_argv_push(argv, "-e");
  u_argv_push(argv, "-o");
  u_argv_push_printf(argv, "%s/ior-rnd/file", opt.datadir);
  u_argv_push(argv, "-O");
  u_argv_push_printf(argv, "stoneWallingStatusFile=%s/ior-rnd.stonewall", opt.resdir );
  u_argv_push(argv, "-O");
  u_argv_push(argv, "stoneWallingWearOut=1");
  u_argv_push(argv, "-t");
  u_argv_push(argv, "4096");
  u_argv_push(argv, "-b");
  u_argv_push(argv, "4096");
  u_argv_push(argv, "-s");
  u_argv_push_printf(argv, "%d", d.segments);
}

u_phase_t p_ior_rnd = {
  "ior-rnd",
  IO500_PHASE_DUMMY,
  option,
  validate,
  NULL,
  .cleanup = cleanup,
};

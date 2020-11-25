#include <sys/stat.h>
#include <unistd.h>

#include <phase_ior.h>
#include <io500-phase.h>

opt_ior_hard ior_hard_o;

static ini_option_t option[] = {
  {"API", "The API to be used", 0, INI_STRING, NULL, & ior_hard_o.api},
  {"posix.odirect", "Use ODirect", 0, INI_BOOL, NULL, & ior_hard_o.odirect},
  {"hintsFileName", "Filename for hints file", 0, INI_STRING, NULL, & ior_hard_o.hintsFileName},
  {"segmentCount", "Number of segments", 0, INI_INT, "10000000", & ior_hard_o.segments},
  {"noRun", "Disable running of this phase", 0, INI_BOOL, NULL, & ior_hard_o.no_run},
  {"verbosity", "The verbosity level", 0, INI_INT, 0, & ior_hard_o.verbosity},
  {NULL} };



static void validate(void){
  if(ior_hard_o.hintsFileName){
    struct stat sb;
    int ret = stat(ior_hard_o.hintsFileName, & sb);
    if(ret != 0){
      FATAL("Cannot check hintsFileName %s\n", ior_hard_o.hintsFileName);
    }
    if(! (sb.st_mode & S_IRUSR) ){
      FATAL("The hintsFileName must be a readable file %s\n", ior_hard_o.hintsFileName);
    }
  }
  u_create_datadir("ior-hard");
}

static void cleanup(void){
  if( ! opt.dry_run && opt.rank == 0){
    char filename[PATH_MAX];
    sprintf(filename, "%s/ior-hard.stonewall", opt.resdir);
    unlink(filename);
    u_purge_file("ior-hard/file");
  }
  if(opt.rank == 0){
    u_purge_datadir("ior-hard");
  }
}

void ior_hard_add_params(u_argv_t * argv){
  opt_ior_hard d = ior_hard_o;

  u_argv_push(argv, "./ior");
  for(int i=0; i < ior_hard_o.verbosity; i++){
    u_argv_push(argv, "-v");	/* verbose */
  }
  u_argv_push(argv, "-C");	/* reorder tasks in constant order for read */
  u_argv_push(argv, "-Q");	/* task per node offset */
  u_argv_push(argv, "1");
  u_argv_push(argv, "-g");	/* barriers between open, read, write, close */
  u_argv_push(argv, "-G");	/* use fixed timestamp signature */
  u_argv_push(argv, "27");
  u_argv_push(argv, "-k");	/* keep file after program exit */
  u_argv_push(argv, "-e");	/* fsync upon write close */
  u_argv_push(argv, "-o");	/* filename for output file */
  u_argv_push_printf(argv, "%s/ior-hard/file", opt.datadir);
  u_argv_push(argv, "-O");	/* additional IOR options */
  u_argv_push_printf(argv, "stoneWallingStatusFile=%s/ior-hard.stonewall", opt.resdir );
  u_argv_push(argv, "-O");	/* additional IOR directives */
  u_argv_push(argv, "stoneWallingWearOut=1");
  u_argv_push(argv, "-t");	/* transfer size */
  u_argv_push(argv, "47008");
  u_argv_push(argv, "-b");	/* blocksize in bytes */
  u_argv_push(argv, "47008");
  u_argv_push(argv, "-s");	/* number of segments */
  u_argv_push_printf(argv, "%d", d.segments);
}

u_phase_t p_ior_hard = {
  "ior-hard",
  IO500_PHASE_DUMMY,
  option,
  validate,
  NULL,
  .cleanup = cleanup,
};

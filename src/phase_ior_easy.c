#include <sys/stat.h>
#include <unistd.h>

#include <io500-phase.h>
#include <phase_ior.h>

opt_ior_easy ior_easy_o;

static ini_option_t option[] = {
  {"API", "The API to be used", 0, INI_STRING, NULL, & ior_easy_o.api},
  {"transferSize", "Transfer size", 0, INI_STRING, "2m", & ior_easy_o.transferSize},
  {"blockSize", "Block size; must be a multiple of transferSize", 0, INI_STRING, "9920000m", & ior_easy_o.blockSize},
  {"filePerProc", "Create one file per process", 0, INI_BOOL, "TRUE", & ior_easy_o.filePerProc},
  {"uniqueDir", "Use unique directory per file per process", 0, INI_BOOL, "FALSE", & ior_easy_o.uniqueDir},
  {"run", "Run this phase", 0, INI_BOOL, "TRUE", & ior_easy_o.run},
  {"verbosity", "The verbosity level", 0, INI_INT, 0, & ior_easy_o.verbosity},
  {NULL} };

static void validate(void){
  u_create_datadir("ior-easy");
}

static void cleanup(void){
  if (opt.dry_run) return;
  
  if(opt.rank == 0){
    char filename[PATH_MAX];
    sprintf(filename, "%s/ior-easy.stonewall", opt.resdir);
    unlink(filename);

    if(! ior_easy_o.filePerProc){
      u_purge_file("ior-easy/ior_file_easy");
    }
  }
  if(ior_easy_o.filePerProc){
      char filename[PATH_MAX];
      sprintf(filename, "ior-easy/ior_file_easy.%08d", opt.rank);
      u_purge_file(filename);
  }
  if(opt.rank == 0){
    u_purge_datadir("ior-easy");
  }
}

void ior_easy_add_params(u_argv_t * argv){
  opt_ior_easy d = ior_easy_o;

  u_argv_push(argv, "./ior");
  u_argv_push_printf(argv, "--dataPacketType=%s", opt.dataPacketType);
  for(int i=0; i < ior_easy_o.verbosity; i++){
    u_argv_push(argv, "-v");	/* verbose */
  }
  if(opt.io_buffers_on_gpu){
    u_argv_push(argv, "-O");
    u_argv_push(argv, "allocateBufferOnGPU=1");
  }
  u_argv_push(argv, "-C");	/* reorder tasks in constant order for read */
  u_argv_push(argv, "-Q");	/* task per node offset */
  u_argv_push(argv, "1");
  u_argv_push(argv, "-g");	/* barriers between open, read, write, close */
  u_argv_push(argv, "-G");	/* use fixed timestamp signature */
  int hash = u_phase_unique_random_number("ior-easy");
  u_argv_push_printf(argv, "%d", hash);
//  u_argv_push(argv, "271");
  u_argv_push(argv, "-k");	/* keep file after program exit */
  u_argv_push(argv, "-e");	/* fsync upon write close */
  u_argv_push(argv, "-o");	/* filename for output file */
  u_argv_push_printf(argv, "%s/ior-easy/ior_file_easy", opt.datadir);
  u_argv_push(argv, "-O");	/* additional IOR options */
  u_argv_push_printf(argv, "stoneWallingStatusFile=%s/ior-easy.stonewall", opt.resdir);
  u_argv_push(argv, "-t");	/* transfer size */
  u_argv_push(argv, d.transferSize);
  u_argv_push(argv, "-b");	/* blocksize in bytes */
  u_argv_push(argv, d.blockSize);

  if(ior_easy_o.uniqueDir){
    u_argv_push(argv, "-u");	/* unique directory for each output file */
  }

  if(ior_easy_o.filePerProc){
    u_argv_push(argv, "-F");	/* write a separate file per process */
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

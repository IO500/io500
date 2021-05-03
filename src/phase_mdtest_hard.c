#include <sys/stat.h>
#include <unistd.h>

#include <mdtest.h>

#include <io500-phase.h>
#include <phase_mdtest.h>

opt_mdtest_hard mdtest_hard_o;

static ini_option_t option[] = {
  {"API", "The API to be used", 0, INI_STRING, NULL, & mdtest_hard_o.g.api},
  {"n", "Files per proc", 0, INI_UINT64, "1000000", & mdtest_hard_o.g.files_per_proc},
  {"files-per-dir", "File limit per directory (MDTest -I flag) to overcome file system limitations ", 0, INI_UINT64, NULL, & mdtest_hard_o.g.files_per_dir},
  {"run", "Run this phase", 0, INI_BOOL, "TRUE", & mdtest_hard_o.g.run},
  {NULL} };

static void validate(void){

}

static void cleanup(void){
  if( ! opt.dry_run && opt.rank == 0){
    char filename[PATH_MAX];
    sprintf(filename, "%s/mdtest-hard.stonewall", opt.resdir);
    unlink(filename);
    u_purge_datadir("mdtest-hard");
  }
}

void mdtest_hard_add_params(u_argv_t * argv){
  opt_mdtest_hard d = mdtest_hard_o;

  u_argv_push(argv, "./mdtest");
  if(opt.io_buffers_on_gpu){
    u_argv_push(argv, "--allocateBufferOnGPU");
  }
  u_argv_push(argv, "-n");
  u_argv_push_printf(argv, "%"PRIu64, d.g.files_per_proc);

  if(mdtest_hard_o.g.files_per_dir != INI_UNSET_UINT64 && mdtest_hard_o.g.files_per_dir > 0){
    u_argv_push_printf(argv, "-I=%"PRIu64, mdtest_hard_o.g.files_per_dir);
  }

  u_argv_push(argv, "-t");
  u_argv_push(argv, "-w");
  u_argv_push(argv, "3901");
  u_argv_push(argv, "-e");
  u_argv_push(argv, "3901");
  u_argv_push(argv, "-P");
  int hash = u_phase_unique_random_number("mdtest-hard");
  u_argv_push_printf(argv, "-G=%d", hash);
  u_argv_push(argv, "-N");
  u_argv_push(argv, "1");
  u_argv_push(argv, "-F");
  u_argv_push(argv, "-d");
  u_argv_push_printf(argv, "%s/mdtest-hard", opt.datadir);
  u_argv_push(argv, "-x");
  u_argv_push_printf(argv, "%s/mdtest-hard.stonewall", opt.resdir);
}

u_phase_t p_mdtest_hard = {
  "mdtest-hard",
  IO500_PHASE_DUMMY,
  option,
  validate,
  NULL,
  .cleanup = cleanup,
};

#include <sys/stat.h>
#include <unistd.h>

#include <mdtest.h>

#include <io500-phase.h>
#include <phase_mdtest.h>

opt_mdtest_easy mdtest_easy_o;

static ini_option_t option[] = {
  {"API", "The API to be used", 0, INI_STRING, NULL, & mdtest_easy_o.g.api},
  {"posix.odirect", "Use ODirect", 0, INI_BOOL, NULL, & mdtest_easy_o.g.odirect},
  {"n", "Files per proc", 0, INI_UINT64, "1000000", & mdtest_easy_o.g.files_per_proc},
  {"noRun", "Disable running of this phase", 0, INI_BOOL, NULL, & mdtest_easy_o.g.no_run},
  {NULL} };

static void validate(void){

}

static void cleanup(void){
  if( ! opt.dry_run && opt.rank == 0){
    //u_purge_file("mdtest-easy-stonewall");
    u_purge_datadir("mdtest-easy");
  }
}

void mdtest_easy_add_params(u_argv_t * argv){
  opt_mdtest_easy d = mdtest_easy_o;

  u_argv_push(argv, "./mdtest");
  u_argv_push(argv, "-n");
  u_argv_push_printf(argv, "%"PRIu64, d.g.files_per_proc);
  u_argv_push(argv, "-u");
  u_argv_push(argv, "-L");
  u_argv_push(argv, "-F");
  u_argv_push(argv, "-P");
  u_argv_push(argv, "-N");
  u_argv_push(argv, "1");
  u_argv_push(argv, "-d");
  u_argv_push_printf(argv, "%s/mdtest-easy", opt.datadir);
  u_argv_push(argv, "-x");
  u_argv_push_printf(argv, "%s/mdtest-easy.stonewall", opt.resdir);
}

u_phase_t p_mdtest_easy = {
  "mdtest-easy",
  IO500_PHASE_DUMMY,
  option,
  validate,
  NULL,
  .cleanup = cleanup,
};

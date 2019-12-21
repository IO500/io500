#include <sys/stat.h>
#include <unistd.h>

#include <mdtest.h>

#include <io500-phase.h>
#include <phase_mdtest.h>

opt_mdtest_hard mdtest_hard_o;

static ini_option_t option[] = {
  {"API", "The API to be used", 0, INI_STRING, "POSIX", & mdtest_hard_o.g.api},
  {"posix.odirect", "Use ODirect", 0, INI_BOOL, NULL, & mdtest_hard_o.g.odirect},
  {"n", "Files per proc", 0, INI_UINT64, "1000000", & mdtest_hard_o.g.files_per_proc},
  {"noRun", "Disable running of this phase", 0, INI_BOOL, NULL, & mdtest_hard_o.g.no_run},
  {NULL} };

static void validate(void){

}

void mdtest_hard_add_params(u_argv_t * argv){
  opt_mdtest_hard d = mdtest_hard_o;

  u_argv_push(argv, "./mdtest");
  u_argv_push(argv, "-n");
  u_argv_push_printf(argv, "%"PRIu64, d.g.files_per_proc);
  u_argv_push(argv, "-t");
  u_argv_push(argv, "-w");
  u_argv_push(argv, "3901");
  u_argv_push(argv, "-e");
  u_argv_push(argv, "3901");
  u_argv_push(argv, "-F");
  u_argv_push(argv, "-d");
  u_argv_push_printf(argv, "%s/mdt_hard", opt.datadir);
  u_argv_push(argv, "-x");
  u_argv_push_printf(argv, "%s/mdt_hard-stonewall", opt.datadir);
}

u_phase_t p_mdtest_hard = {
  "mdtest-hard",
  option,
  validate,
  NULL
};

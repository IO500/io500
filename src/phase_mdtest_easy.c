#include <sys/stat.h>
#include <unistd.h>

#include <mdtest.h>

#include <io500-phase.h>
#include <phase_mdtest.h>

opt_mdtest_easy mdtest_easy_o;

static ini_option_t option[] = {
  {"API", "The API to be used", 0, INI_STRING, "POSIX", & mdtest_easy_o.g.api},
  {"posix.odirect", "Use ODirect", 0, INI_BOOL, NULL, & mdtest_easy_o.g.odirect},
  {"n", "Files per proc", 0, INI_UINT64, "1000000", & mdtest_easy_o.g.files_per_proc},
  {"noRun", "Disable running of this phase", 0, INI_BOOL, NULL, & mdtest_easy_o.g.no_run},
  {NULL} };

static void validate(void){

}

void mdtest_easy_add_params(u_argv_t * argv){
  opt_mdtest_easy d = mdtest_easy_o;

  u_argv_push(argv, "./mdtest");
  u_argv_push(argv, "-n");
  u_argv_push_printf(argv, "%"PRIu64, d.g.files_per_proc);
  u_argv_push(argv, "-u");
  u_argv_push(argv, "-L");
  u_argv_push(argv, "-F");
  u_argv_push(argv, "-d");
  u_argv_push_printf(argv, "%s/mdt_easy", opt.datadir);
  u_argv_push(argv, "-x");
  u_argv_push_printf(argv, "%s/mdt_easy-stonewall", opt.datadir);
  u_argv_push(argv, "-W");
  u_argv_push_printf(argv, "%d", opt.stonewall);
}

void p_mdtest_run(u_argv_t * argv, FILE * out, mdtest_generic_res * d, mdtest_test_num_t test){
  mdtest_results_t * res = mdtest_run(argv->size, argv->vector, MPI_COMM_WORLD, out);
  d->time = res->time[test];
  INFO_PAIR("time", "%f\n", d->time);
  d->items = res->items[test];
  d->rate = res->rate[test] / 1000;
  d->rate_stonewall = res->stonewall_item_sum[test] / res->stonewall_time[test] / 1000;
  u_res_file_close(out);
  u_argv_free(argv);
}

void mdtest_add_generic_params(u_argv_t * argv, opt_mdtest_generic * dflt, opt_mdtest_generic * generic){
  u_argv_push_default_if_set(argv, "-a", dflt->api, generic->api);
  u_argv_push_default_if_set_bool(argv, "--posix.odirect", dflt->odirect, generic->odirect);

  generic->command = u_flatten_argv(argv);
  PRINT_PAIR("exe", "%s\n", generic->command);
}

u_phase_t p_mdtest_easy = {
  "mdtest-easy",
  option,
  validate,
  NULL
};

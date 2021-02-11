#include <phase_mdtest.h>

void p_mdtest_run(u_argv_t * argv, FILE * out, mdtest_generic_res * d, mdtest_test_num_t test){
  mdtest_results_t * res = mdtest_run(argv->size, argv->vector, MPI_COMM_WORLD, out);
  d->time = res->time[test];
  INFO_PAIR("time", "%f\n", d->time);
  d->items = res->items[test];
  d->rate = res->rate[test] / 1000;
  d->rate_stonewall = res->stonewall_item_sum[test] / res->stonewall_time[test] / 1000;
  u_res_file_close(out);
  u_argv_free(argv);
  free(res);
}

void mdtest_add_generic_params(u_argv_t * argv, opt_mdtest_generic * dflt, opt_mdtest_generic * generic){
  u_argv_push_default_if_set_api_options(argv, "-a", dflt->api, generic->api);

  generic->command = u_flatten_argv(argv);
  PRINT_PAIR("exe", "%s\n", generic->command);
}

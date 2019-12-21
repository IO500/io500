#ifndef IO500_PHASE_MDTEST_H
#define IO500_PHASE_MDTEST_H

#include <mdtest.h>
#include <io500-util.h>

typedef struct{
  int no_run;
  char * api;
  int odirect;

  uint64_t files_per_proc;
  char * command;
} opt_mdtest_generic;

typedef struct{
  double rate;
  double rate_stonewall;
  uint64_t items;
  double time;
} mdtest_generic_res;


typedef struct{
  opt_mdtest_generic g;
  mdtest_generic_res res;
} opt_mdtest_easy;

extern opt_mdtest_easy mdtest_easy_o;

void mdtest_add_generic_params(u_argv_t * argv, opt_mdtest_generic * dflt, opt_mdtest_generic * generic);

void mdtest_easy_add_params(u_argv_t * argv);
void mdtest_hard_add_params(u_argv_t * argv);

void p_mdtest_run(u_argv_t * argv, FILE * out, mdtest_generic_res * d, mdtest_test_num_t test);
#endif

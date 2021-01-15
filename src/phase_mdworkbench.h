#ifndef IO500_PHASE_MDWORKBENCH_H
#define IO500_PHASE_MDWORKBENCH_H

#include <md-workbench.h>
#include <io500-util.h>

typedef struct{
  int run;
  char * api;
  int verbosity;
  float waiting_time;
  uint64_t precreate_per_set;
  uint64_t files_per_proc;
} opt_mdworkbench;

extern opt_mdworkbench mdworkbench_o;

void mdworkbench_add_params(u_argv_t * argv, int is_create);
void mdworkbench_process(u_argv_t * argv, FILE * out, mdworkbench_results_t ** res_out);

#endif

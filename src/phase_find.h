#ifndef IO500_PHASE_FIND_H
#define IO500_PHASE_FIND_H

#include <pfind-options.h>
#include <io500-util.h>

typedef struct{
  char * ext_find;
  char * ext_args;
  char * ext_mpi;
  int nproc;
  int run;
  char * command;

  pfind_options_t * pfind_o;
  pfind_find_results_t * pfind_res;
  MPI_Comm pfind_com;
  int pfind_queue_length;
  int pfind_steal_from_next;
  int pfind_par_single_dir_access_hash;

  uint64_t found_files;
  double runtime;
} opt_find;

double run_find(const char * phase_name, opt_find * of);

void pfind_prepare_arguments(u_argv_t * argv, opt_find * of);
void external_find_prepare_arguments(char * args, opt_find * of);
#endif

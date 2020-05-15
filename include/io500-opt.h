#ifndef IO500_OPT_H
#define IO500_OPT_H

#include <stdbool.h>
#include <aiori.h>

typedef struct{
  int drop_caches;
  char * drop_caches_cmd;

  int stonewall;

  char * datadir;
  char * resdir;

  int dry_run;
  int verbosity;
  int is_valid_run;

  int rank;
  int mpi_size;

  char * api;
  char * timestamp;
  int timestamp_resdir;
  int timestamp_datadir;

  IOR_param_t aiori_params;
  ior_aiori_t const * aiori;
} io500_opt_t;

io500_opt_t opt;

#endif

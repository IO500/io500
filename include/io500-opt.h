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
  char * apiArgs; // for IOR and mdtest
  char * timestamp;
  int timestamp_resdir;
  int timestamp_datadir;

  aiori_xfer_hint_t backend_hints;
  aiori_mod_opt_t * backend_opt;
  ior_aiori_t const * aiori;
} io500_opt_t;

extern io500_opt_t opt;

#endif

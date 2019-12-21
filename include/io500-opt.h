#ifndef IO500_OPT_H
#define IO500_OPT_H

#include <stdbool.h>

typedef struct{
  int drop_caches;
  int stonewall;

  char * datadir;
  char * resdir;

  int dry_run;
  int verbosity;
  int is_valid_run;

  int rank;
  int mpi_size;

  int non_posix_datadir;
} io500_opt_t;

io500_opt_t opt;

#endif

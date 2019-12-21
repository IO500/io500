#ifndef IO500_OPT_H
#define IO500_OPT_H

#include <stdbool.h>

typedef struct{
  bool drop_caches;
  int stonewall;

  int dry_run;
  int verbosity;
  int is_valid_run;
  int rank;
} io500_opt_t;

io500_opt_t opt;

#endif

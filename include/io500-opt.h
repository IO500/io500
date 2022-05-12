#ifndef IO500_OPT_H
#define IO500_OPT_H

#include <stdbool.h>
#include <aiori.h>

typedef enum{
  IO500_MODE_STANDARD = 1,
  IO500_MODE_EXTENDED,
} io500_mode;

typedef struct{
  int drop_caches;
  char * drop_caches_cmd;

  int stonewall;
  char * pause_dir; // if set check for files of the phase

  char * datadir;
  char * resdir;

  int dry_run;
  int verbosity;
  int is_valid_phase;    /* set to 0 during a phase, if invalid */
  int is_valid_run;      /* set to 0 the first occurence it is invalid */
  int is_valid_extended_run;

  int rank;
  int mpi_size;
  int io_buffers_on_gpu; /* are the I/O buffers to be allocated on a GPU */

  char * api;
  char * apiArgs; // for IOR and mdtest
  char * dataPacketType; /* how test data is created */
  char * timestamp;
  int timestamp_resdir;
  int timestamp_datadir;

  int scc;
  int minwrite;
  io500_mode mode;

  aiori_xfer_hint_t backend_hints;
  aiori_mod_opt_t * backend_opt;
  ior_aiori_t const * aiori;
} io500_opt_t;

extern io500_opt_t opt;

#endif

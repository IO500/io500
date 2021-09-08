#ifndef IO500_PHASE_H
#define IO500_PHASE_H

#include <stdlib.h>

#include <io500-util.h>

#define DEBUG_OPTION "INVALIDATES RUN; FOR DEBUGGING."

typedef enum {
  IO500_NO_SCORE,
  IO500_SCORE_MD,
  IO500_SCORE_BW,
  IO500_SCORE_LAST
} io500_phase_score_group;

// bitfield of the type
typedef enum {
  IO500_PHASE_DUMMY = 1,
  IO500_PHASE_WRITE = 2,
  IO500_PHASE_READ = 4,
  IO500_PHASE_UPDATE = 8,
  IO500_PHASE_REMOVE = 16,
  IO500_PHASE_FLAG_OPTIONAL = 32
} io500_phase_type;

typedef struct{
  char const * name;
  io500_phase_type type;
  ini_option_t * options;
  void (*validate)(void); // check options
  double (*run)(void);    // returns the score
  bool verify_stonewall;  // double check that runtime meets stonewall?
  void (*cleanup)(void);  // remove generated files/directories if possible

  double score; // the measured score
  io500_phase_score_group group;
} u_phase_t;

#define IO500_PHASES (2 + 1 + 2*3 + 3 + 3 + 4 + 5 + 3 + 4)

extern u_phase_t p_opt;
extern u_phase_t p_debug;

extern u_phase_t p_timestamp;

extern u_phase_t p_find;
extern u_phase_t p_find_easy;
extern u_phase_t p_find_hard;

extern u_phase_t p_ior_hard;
extern u_phase_t p_ior_hard_write;
extern u_phase_t p_ior_hard_read;

extern u_phase_t p_ior_rnd4K;
extern u_phase_t p_ior_rnd4K_write;
extern u_phase_t p_ior_rnd4K_read;

extern u_phase_t p_ior_rnd1MB;
extern u_phase_t p_ior_rnd1MB_write;
extern u_phase_t p_ior_rnd1MB_read;

extern u_phase_t p_mdworkbench;
extern u_phase_t p_mdworkbench_create;
extern u_phase_t p_mdworkbench_bench;
extern u_phase_t p_mdworkbench_delete;

extern u_phase_t p_ior_easy;
extern u_phase_t p_ior_easy_write;
extern u_phase_t p_ior_easy_read;

extern u_phase_t p_mdtest_easy;
extern u_phase_t p_mdtest_easy_write;
extern u_phase_t p_mdtest_easy_stat;
extern u_phase_t p_mdtest_easy_delete;

extern u_phase_t p_mdtest_hard;
extern u_phase_t p_mdtest_hard_write;
extern u_phase_t p_mdtest_hard_stat;
extern u_phase_t p_mdtest_hard_read;
extern u_phase_t p_mdtest_hard_delete;

#endif

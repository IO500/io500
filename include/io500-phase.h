#ifndef IO500_PHASE_H
#define IO500_PHASE_H

#include <stdlib.h>

#include <io500-util.h>

typedef struct{
  char const * name;
  ini_option_t * options;
  void (*validate)(void);
  double (*run)(void); // returns the score
} u_phase_t;

#define IO500_PHASES (2 + 2*3 + 1 + 4 + 5)

extern u_phase_t p_opt;
extern u_phase_t p_debug;

extern u_phase_t p_find;

extern u_phase_t p_ior_hard;
extern u_phase_t p_ior_hard_write;
extern u_phase_t p_ior_hard_read;

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

#include <sys/stat.h>
#include <unistd.h>

#include <io500-phase.h>

typedef struct{
  bool odirect;
} opt_ior_hard_write;

static opt_ior_hard_write o;

static double run(void){
  return 0;
}

static ini_option_t option[] = {
  {"posix.odirect", "Use ODirect", 0, INI_BOOL, NULL, & o.odirect},
  {NULL} };

static ini_option_t * get_ini_section(void){
  return option;
}

static void validate(void){

}

u_phase_t p_ior_hard_write = {
  "ior-hard-write",
  get_ini_section,
  validate,
  run
};

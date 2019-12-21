#include <sys/stat.h>
#include <unistd.h>

#include <io500-phase.h>

typedef struct{
  char * api;
  bool odirect;
} opt_mdtest_hard_write;

static opt_mdtest_hard_write o;

static ini_option_t option[] = {
  {"API", "The API to be used", 0, INI_STRING, "POSIX", & o.api},
  {"posix.odirect", "Use ODirect", 0, INI_BOOL, NULL, & o.odirect},
  {NULL} };


static void validate(void){

}

u_phase_t p_mdtest_hard_write = {
  "mdtest-hard-write",
  option,
  validate,
  NULL,
  .verify_stonewall = 1
};

#include <sys/stat.h>
#include <unistd.h>

#include <io500-phase.h>

typedef struct{
  char * api;
  bool odirect;
} opt_mdtest_hard;

opt_mdtest_hard mdtest_hard_o;

static ini_option_t option[] = {
  {"API", "The API to be used", 0, INI_STRING, "POSIX", & mdtest_hard_o.api},
  {"posix.odirect", "Use ODirect", 0, INI_BOOL, NULL, & mdtest_hard_o.odirect},
  {NULL} };

static ini_option_t * get_ini_section(void){
  return option;
}

static void validate(void){

}

u_phase_t p_mdtest_hard = {
  "mdtest_hard",
  get_ini_section,
  validate,
  NULL
};

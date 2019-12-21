#include <sys/stat.h>
#include <unistd.h>

#include <io500-phase.h>

typedef struct{
  char * api;
  bool odirect;
} opt_ior_hard;

opt_ior_hard ior_hard_o;

static ini_option_t option[] = {
  {"API", "The API to be used", 0, INI_STRING, "POSIX", & ior_hard_o.api},
  {"posix.odirect", "Use ODirect", 0, INI_BOOL, NULL, & ior_hard_o.odirect},
  {NULL} };

static ini_option_t * get_ini_section(void){
  return option;
}

static void validate(void){

}

u_phase_t p_ior_hard = {
  "ior-hard",
  get_ini_section,
  validate,
  NULL
};

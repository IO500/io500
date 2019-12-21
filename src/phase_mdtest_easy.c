#include <sys/stat.h>
#include <unistd.h>

#include <io500-phase.h>

typedef struct{
  char * api;
  bool odirect;
} opt_mdtest_easy;

opt_mdtest_easy mdtest_easy_o;

static ini_option_t option[] = {
  {"API", "The API to be used", 0, INI_STRING, "POSIX", & mdtest_easy_o.api},
  {"posix.odirect", "Use ODirect", 0, INI_BOOL, NULL, & mdtest_easy_o.odirect},
  {NULL} };

static void validate(void){

}

u_phase_t p_mdtest_easy = {
  "mdtest_easy",
  option,
  validate,
  NULL
};

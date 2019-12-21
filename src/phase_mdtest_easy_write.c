#include <sys/stat.h>
#include <unistd.h>

#include <io500-phase.h>

typedef struct{
  char * api;
  bool odirect;
} opt_mdtest_easy_write;

static opt_mdtest_easy_write o;

static ini_option_t option[] = {
  {"API", "The API to be used", 0, INI_STRING, "POSIX", & o.api},
  {"posix.odirect", "Use ODirect", 0, INI_BOOL, NULL, & o.odirect},
  {NULL} };


static void validate(void){

}

u_phase_t p_mdtest_easy_write = {
  "mdtest_easy_write",
  option,
  validate,
  NULL
};

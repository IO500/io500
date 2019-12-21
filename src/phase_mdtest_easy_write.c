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

static double run(void){

  char timestamp_file[2048];
  sprintf(timestamp_file, "%s/timestampfile", opt.datadir);
  DEBUG_INFO("Writing timestamp file %s\n", timestamp_file);

  FILE * f = fopen(timestamp_file, "w");
  if(! f){
    FATAL("Couldn't write timestampfile: %s\n", timestamp_file);
  }
  fclose(f);

  return 0;
}

u_phase_t p_mdtest_easy_write = {
  "mdtest-easy-write",
  option,
  validate,
  run,
  .verify_stonewall = 1
};

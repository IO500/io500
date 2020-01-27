#include <sys/stat.h>
#include <unistd.h>

#include <io500-phase.h>
#include <phase_mdtest.h>

static ini_option_t option[] = {
  {NULL} };


static void validate(void){

}

static double run(void){
  if(opt.rank != 0) return 0;

  char timestamp_file[2048];
  sprintf(timestamp_file, "%s/timestampfile", opt.datadir);
  INFO_PAIR("timestamp-file", "%s\n", timestamp_file);

  FILE * f = fopen(timestamp_file, "w");
  if(! f){
    FATAL("Couldn't write timestampfile: %s\n", timestamp_file);
  }
  fclose(f);

  return 0;
}

u_phase_t p_timestamp = {
  "timestamp",
  IO500_PHASE_WRITE,
  option,
  validate,
  run,
  0
};

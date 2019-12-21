#include <io500-phase.h>

static ini_option_t option[] = {
  {"drop-caches", "Purge the caches, this is useful for testing and needed for single node runs", 0, INI_BOOL, "false", & opt.drop_caches},
  {"stonewall-time", "The stonewall timer, set to a smaller value for testing", 0, INI_UINT, "300", & opt.stonewall},
  {NULL} };


static void validate(void){
  if(opt.stonewall != 300){
    WARNING("stonewall-time != 300, this is an invalid run\n");
    opt.is_valid_run = 0;
  }
}

u_phase_t p_debug = {
  "debug",
  option,
  validate,
  NULL
};

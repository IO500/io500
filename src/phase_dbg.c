#include <io500-phase.h>

static ini_option_t option[] = {
  {"stonewall-time", "The stonewall timer, set to a smaller value for testing", 0, INI_UINT, "300", & opt.stonewall},
  {NULL} };


static void validate(void){
  if(opt.stonewall != 300 && opt.rank == 0){
    INVALID("stonewall-time != 300, this is an invalid run\n");
  }
}

u_phase_t p_debug = {
  "debug",
  IO500_PHASE_DUMMY,
  option,
  validate,
  NULL
};

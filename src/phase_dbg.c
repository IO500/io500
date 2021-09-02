#include <io500-phase.h>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

static ini_option_t option[] = {
  {"stonewall-time", "For a valid result, the stonewall timer must be set to the value according to the rules. If smaller "DEBUG_OPTION, 0, INI_INT, "300", & opt.stonewall},
  {NULL} };


static void validate(void){
  if(opt.stonewall != opt.minwrite && opt.rank == 0){
    INVALID("stonewall-time < %us\n", opt.minwrite);
  }
}

u_phase_t p_debug = {
  "debug",
  IO500_PHASE_DUMMY,
  option,
  validate,
  NULL
};

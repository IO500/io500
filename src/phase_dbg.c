#include <io500-phase.h>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

static ini_option_t option[] = {
  {"stonewall-time", "For a valid result, the stonewall timer must be set to the value according to the rules. If smaller "DEBUG_OPTION, 0, INI_INT, "300", & opt.stonewall},
  {"pause-dir", "Pause between phases while in this directory lies a file with the phase name, e.g., easy-create. This can be useful for performance testing, e.g., of tiered storage. At the moment it "DEBUG_OPTION, 0, INI_STRING, NULL, & opt.pause_dir},
  
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

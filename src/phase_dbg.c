#include <io500-phase.h>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

static ini_option_t option[] = {
  {"stonewall-time", "Stonewall timer must be "TOSTRING(IO500_MINWRITE)" for a valid result, can be smaller for testing", 0, INI_UINT, TOSTRING(IO500_MINWRITE), & opt.stonewall},
  {NULL} };


static void validate(void){
  if(opt.stonewall != IO500_MINWRITE && opt.rank == 0){
    INVALID("stonewall-time != %us\n", IO500_MINWRITE);
  }
}

u_phase_t p_debug = {
  "debug",
  IO500_PHASE_DUMMY,
  option,
  validate,
  NULL
};

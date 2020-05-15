#include <io500-phase.h>

#define STRINGIFY(foo)  #foo

static ini_option_t option[] = {
  {"stonewall-time", "The stonewall timer, set to a smaller value for testing", 0, INI_UINT, STRINGIFY(IO500_MINWRITE), & opt.stonewall},
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

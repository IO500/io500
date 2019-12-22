#include <io500-phase.h>

io500_opt_t opt;

static ini_option_t option[] = {
  {"datadir", "The directory where the IO500 runs", 1, INI_STRING, NULL, & opt.datadir},
  {"timestamp-datadir", "The data directory is suffixed by a timestamp. Useful for running several IO500 tests concurrently.", 0, INI_BOOL, "FALSE", & opt.timestamp_datadir},
  {"resultdir", "The result directory.", 0, INI_STRING, "./results", & opt.resdir},
  {"timestamp-resultdir", "The result directory is suffixed by a timestamp. Useful for running several IO500 tests concurrently.", 0, INI_BOOL, "TRUE", & opt.timestamp_resdir},
  {"api", "The API to create/delete the datadir", 0, INI_STRING, "POSIX", & opt.api},
  {"verbosity", "The verbosity level between 1 and 10", 0, INI_UINT, "1", & opt.verbosity},
  {NULL} };

static void validate(void){

}

u_phase_t p_opt = {
  "global",
  IO500_PHASE_DUMMY,
  option,
  validate,
  NULL
};

#include <io500-phase.h>

io500_opt_t opt;

static ini_option_t option[] = {
  {"datadir", "The directory where the IO500 runs", 1, INI_STRING, NULL, & opt.datadir},
  {"timestamp-datadir", "The data directory is suffixed by a timestamp. Useful for running several IO500 tests concurrently.", 0, INI_BOOL, "TRUE", & opt.timestamp_datadir},
  {"resultdir", "The result directory.", 0, INI_STRING, "./results", & opt.resdir},
  {"timestamp-resultdir", "The result directory is suffixed by a timestamp. Useful for running several IO500 tests concurrently.", 0, INI_BOOL, "TRUE", & opt.timestamp_resdir},
  {"api", "The general API for the tests (to create/delete the datadir, extra options will be passed to IOR/mdtest)", 0, INI_STRING, "POSIX", & opt.api},
  {"drop-caches", "Purge the caches, this is useful for testing and needed for single node runs", 0, INI_BOOL, "FALSE", & opt.drop_caches},
  {"drop-caches-cmd", "Cache purging command, invoked before each I/O phase", 0, INI_STRING, "sudo -n bash -c \"echo 3 > /proc/sys/vm/drop_caches\"", & opt.drop_caches_cmd},
  {"verbosity", "The verbosity level between 1 and 10", 0, INI_UINT, "1", & opt.verbosity},
  {"scc", "Declare 30 seconds stonewall for Student luster Competition", 0, INI_BOOL, "FALSE", & opt.scc},
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

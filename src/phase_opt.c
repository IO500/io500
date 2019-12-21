#include <sys/types.h>
#include <dirent.h>

#include <io500-phase.h>

io500_opt_t opt;

static ini_option_t option[] = {
  {"datadir", "The directory where the IO500 runs", 1, INI_STRING, NULL, & opt.datadir},
  {"non-posix-datadir", "The directory is non-POSIX and cannot be checked using POSIX calls", 0, INI_BOOL, NULL, & opt.non_posix_datadir},
  {"verbosity", "The verbosity level between 1 and 10", 0, INI_UINT, "1", & opt.verbosity},
  {NULL} };

static void validate(void){
  if(opt.rank != 0){
    return;
  }
  u_create_datadir("");

  DIR * d = opendir(opt.datadir);
  if (d == NULL){
    WARNING("Cannot read datadir \"%s\", this may not be an issue if you use a non-POSIX storage system\n", opt.datadir);
  }
  closedir(d);
}

u_phase_t p_opt = {
  "global",
  option,
  validate,
  NULL
};

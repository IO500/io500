#include <sys/stat.h>
#include <unistd.h>

#include <io500-phase.h>

typedef struct{
  char * ext_find;
} opt_find;

static opt_find of;

static double run(void){
  return 0;
}

static ini_option_t option[] = {
  {"external-script", "Set to an external script to perform the find phase", 0, INI_STRING, NULL, & of.ext_find},
  {NULL} };

static ini_option_t * get_ini_section(void){
  return option;
}

static void validate(void){
  if(of.ext_find){
    struct stat sb;
    int ret = stat(of.ext_find, & sb);
    if(ret != 0){
      FATAL("Cannot check external-script %s\n", of.ext_find);
    }
    if(! (sb.st_mode & S_IXUSR) ){
      FATAL("The external-script must be a executable file %s\n", of.ext_find);
    }
  }
}

u_phase_t p_find = {
  "find",
  get_ini_section,
  validate,
  run
};

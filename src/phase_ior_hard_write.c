#include <sys/stat.h>
#include <unistd.h>

#include <ior.h>
#include <io500-phase.h>

#include <phase_ior.h>

typedef struct{
  char * api;
  int odirect;
  char * hintsFileName;

  char * command;
  IOR_point_t * res;
} opt_ior_hard_write;

static opt_ior_hard_write o;

static ini_option_t option[] = {
  {"API", "The API to be used", 0, INI_STRING, NULL, & o.api},
  {"hintsFileName", "Filename for hints file", 0, INI_STRING, NULL, & o.hintsFileName},
  {NULL} };

static void validate(void){

}

static double run(void){
  opt_ior_hard d = ior_hard_o;

  u_argv_t * argv = u_argv_create();
  ior_hard_add_params(argv);
  u_argv_push(argv, "-w");
  u_argv_push(argv, "-D");
  u_argv_push_printf(argv, "%d", opt.stonewall);
  u_argv_push_default_if_set(argv, "-U", d.hintsFileName, o.hintsFileName);
  u_argv_push_default_if_set_api_options(argv, "-a", d.api, o.api);

  o.command = u_flatten_argv(argv);

  PRINT_PAIR("exe", "%s\n", o.command);
  if(opt.dry_run || d.no_run == 1){
    u_argv_free(argv);
    return 0;
  }
  FILE * out = u_res_file_prep(p_ior_hard_write.name);
  return ior_process_write(argv, out, & o.res);
}

u_phase_t p_ior_hard_write = {
  "ior-hard-write",
  IO500_PHASE_WRITE,
  option,
  validate,
  run,
  .verify_stonewall = 1,
  .group = IO500_SCORE_BW,
};

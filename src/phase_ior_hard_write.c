#include <sys/stat.h>
#include <unistd.h>

#include <ior.h>
#include <io500-phase.h>

#include <phase_ior.h>

typedef struct{
  char * api;
  char * dataPacketType;

  char * command;
  IOR_point_t * res;
  int collective;
} opt_ior_hard_write;

static opt_ior_hard_write o;

static ini_option_t option[] = {
  {"API", "The API to be used", 0, INI_STRING, NULL, & o.api},
  {"dataPacketType", "Type of packet that will be created [offset|incompressible|timestamp]", 0, INI_STRING, NULL, & o.dataPacketType},
  {"collective", "Collective operation (for supported backends)", 0, INI_BOOL, NULL, & o.collective},
  {NULL} };

static void validate(void){

}

static double run(void){
  opt_ior_hard d = ior_hard_o;

  u_argv_t * argv = u_argv_create();
  ior_hard_add_params(argv);
  u_argv_push(argv, "-w");
  u_argv_push_default_if_set_bool(argv, "-c", d.collective, o.collective);
  u_argv_push(argv, "-D");
  u_argv_push_printf(argv, "%d", opt.stonewall);
  u_argv_push_default_if_set_api_options(argv, "-a", d.api, o.api);
  u_argv_push(argv, "-O");
  u_argv_push_printf(argv, "saveRankPerformanceDetailsCSV=%s/ior-hard-write.csv", opt.resdir);
  u_argv_push_default_if_set_dataPacketType(argv, "-l", d.dataPacketType, o.dataPacketType);
  u_argv_push(argv, "-O");
  u_argv_push(argv, "stoneWallingWearOut=1");
  
  o.command = u_flatten_argv(argv);

  PRINT_PAIR("exe", "%s\n", o.command);
  if(opt.dry_run || d.run == 0){
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

#include <sys/stat.h>
#include <unistd.h>

#include <ior.h>
#include <io500-phase.h>

#include <phase_ior.h>

typedef struct{
  int run;
  char * api;

  char * command;
  IOR_point_t * res;
} opt_ior_rnd_write;

static opt_ior_rnd_write o;

static ini_option_t option[] = {
  {"API", "The API to be used", 0, INI_STRING, NULL, & o.api},
  {"run", "Run this phase", 0, INI_BOOL, "TRUE", & o.run},  
  {NULL} };

static void validate(void){
  if( ! opt.dry_run && opt.rank == 0){
    u_create_datadir("ior-rnd1MB");
  }
}

static void cleanup(void){
  if (opt.dry_run || ! ior_rnd1MB_o.run) return;

  if(opt.rank == 0){
    char filename[PATH_MAX];
    sprintf(filename, "%s/ior-rnd1MB.stonewall", opt.resdir);
    unlink(filename);
    u_purge_file("ior-rnd1MB/file");
    u_purge_datadir("ior-rnd1MB");
  }
}

static double run(void){
  opt_ior_rnd d = ior_rnd1MB_o;

  u_argv_t * argv = u_argv_create();
  ior_rnd1MB_add_params(argv);
  u_argv_push(argv, "-w");
  u_argv_push(argv, "-D");
  u_argv_push_printf(argv, "%d", opt.stonewall);
  u_argv_push_default_if_set_api_options(argv, "-a", d.api, o.api);
  if(d.random_prefill_bytes > 0){
    u_argv_push_printf(argv, "--randomPrefill=%u", d.random_prefill_bytes);
  }
  u_argv_push(argv, "-O");
  u_argv_push(argv, "stoneWallingWearOut=1");
  u_argv_push(argv, "-O");
  u_argv_push_printf(argv, "saveRankPerformanceDetailsCSV=%s/ior-rnd1MB-write.csv", opt.resdir);
  o.command = u_flatten_argv(argv);

  PRINT_PAIR("exe", "%s\n", o.command);
  if(opt.dry_run || d.run == 0 || o.run == 0){
    u_argv_free(argv);
    return 0;
  }
  FILE * out = u_res_file_prep(p_ior_rnd1MB_write.name, opt.rank);
  return ior_process_write(argv, out, & o.res, MPI_COMM_WORLD);
}

u_phase_t p_ior_rnd1MB_write = {
  "ior-rnd1MB-write",
  IO500_PHASE_WRITE | IO500_PHASE_FLAG_OPTIONAL,
  option,
  validate,
  run,
  .verify_stonewall = 1,
  .group = IO500_SCORE_BW,
  .cleanup = cleanup,
};

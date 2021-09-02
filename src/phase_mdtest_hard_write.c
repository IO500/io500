#include <sys/stat.h>
#include <unistd.h>

#include <io500-phase.h>
#include <phase_mdtest.h>

typedef struct{
  opt_mdtest_generic g;
  mdtest_generic_res res;
} opt_mdtest_hard_write;

static opt_mdtest_hard_write o;

static ini_option_t option[] = {
  {"API", "The API to be used", 0, INI_STRING, NULL, & o.g.api},
  {"run", "Run this phase", 0, INI_BOOL, "TRUE", & o.g.run},
  {NULL} };


static void validate(void){

}

static double run(void){
  u_argv_t * argv = u_argv_create();
  mdtest_hard_add_params(argv);
  u_argv_push(argv, "-C"); /* only create files */
  u_argv_push(argv, "-Y"); /* call sync command after each phase */
  
  opt_mdtest_hard d = mdtest_hard_o;
  if((d.g.files_per_dir != INI_UNSET_UINT64 && d.g.files_per_dir > 0)){
    // Must disable stonewalling for supporting this option -- for now!
    WARNING("stonewalling disabled in order to support -I option. Make sure your number of elements is big enough to meet the runtime limits!");
  }else{
    u_argv_push(argv, "-W"); /* deadline for stonewall in seconds */
    u_argv_push_printf(argv, "%d", opt.stonewall);
  }
  
  u_argv_push_printf(argv, "--saveRankPerformanceDetails=%s/mdtest-hard-write.csv", opt.resdir);

  mdtest_add_generic_params(argv, & d.g, & o.g);

  if(opt.dry_run || o.g.run == 0 || mdtest_hard_o.g.run == 0){
    u_argv_free(argv);
    return 0;
  }

  FILE * out = u_res_file_prep(p_mdtest_hard_write.name);
  p_mdtest_run(argv, out, & o.res, MDTEST_FILE_CREATE_NUM);

  PRINT_PAIR("rate-stonewall", "%f\n", o.res.rate_stonewall);
  return o.res.rate;
}

u_phase_t p_mdtest_hard_write = {
  "mdtest-hard-write",
  IO500_PHASE_WRITE,
  option,
  validate,
  run,
  .verify_stonewall = 1,
  .group = IO500_SCORE_MD
};

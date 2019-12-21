#include <sys/stat.h>
#include <unistd.h>

#include <io500-phase.h>
#include <phase_ior.h>

opt_ior_easy ior_easy_o;

static ini_option_t option[] = {
  {"API", "The API to be used", 0, INI_STRING, "POSIX", & ior_easy_o.api},
  {"posix.odirect", "Use ODirect", 0, INI_BOOL, NULL, & ior_easy_o.odirect},
  {"transferSize", "Transfer size", 0, INI_STRING, "2m", & ior_easy_o.transferSize},
  {"blockSize", "Block size; must be a multiple of transferSize", 0, INI_STRING, "9920000m", & ior_easy_o.blockSize},
  {"hintsFileName", "Filename for MPI hint file", 0, INI_STRING, NULL, & ior_easy_o.hintsFileName},
  {"filePerProc", "Create one file per process", 0, INI_BOOL, "TRUE", & ior_easy_o.filePerProc},
  {"noRun", "Disable running of this phase", 0, INI_BOOL, NULL, & ior_easy_o.no_run},
  {NULL} };

static void validate(void){
  if(ior_easy_o.hintsFileName){
    struct stat sb;
    int ret = stat(ior_easy_o.hintsFileName, & sb);
    if(ret != 0){
      FATAL("Cannot check hintsFileName %s\n", ior_easy_o.hintsFileName);
    }
    if(! (sb.st_mode & S_IRUSR) ){
      FATAL("The hintsFileName must be a readable file %s\n", ior_easy_o.hintsFileName);
    }
  }
  u_create_datadir("ior_easy");
}

void ior_easy_add_params(u_argv_t * argv){
  opt_ior_easy d = ior_easy_o;

  u_argv_push(argv, "./ior");
  u_argv_push(argv, "-C");
  u_argv_push(argv, "-Q");
  u_argv_push(argv, "1");
  u_argv_push(argv, "-g");
  u_argv_push(argv, "-G");
  u_argv_push(argv, "27");
  u_argv_push(argv, "-k");
  u_argv_push(argv, "-e");
  u_argv_push(argv, "-o");
  u_argv_push_printf(argv, "%s/ior_easy/ior_file_easy", opt.datadir);
  u_argv_push(argv, "-O");
  u_argv_push_printf(argv, "stoneWallingStatusFile=%s/ior_easy/stonewall", opt.datadir );
  u_argv_push(argv, "-O");
  u_argv_push(argv, "stoneWallingWearOut=1");
  u_argv_push(argv, "-t");
  u_argv_push(argv, d.transferSize);
  u_argv_push(argv, "-b");
  u_argv_push(argv, d.blockSize);
}

double ior_process_write(u_argv_t * argv, FILE * out, IOR_point_t ** res_out){
  IOR_results_t * res = ior_run(argv->size, argv->vector, MPI_COMM_WORLD, out)->results;
  u_res_file_close(out);
  u_argv_free(argv);

  IOR_point_t * p = & res->write;
  *res_out = p;

  if(res->errors){
    WARNING("Errors (%d) occured during phase in IOR. This invalidates your run.\n", res->errors);
    opt.is_valid_run = 0;
  }
  if(p->time < opt.stonewall ){
    WARNING("Write phase needed %fs instead of stonewall %ds. Stonewall was hit at %.1fs\n", p->time, opt.stonewall, p->stonewall_time);
    opt.is_valid_run = 0;
  }
  INFO_PAIR("accessed-pairs", "%zu\n", p->pairs_accessed);

  double tp = p->aggFileSizeForBW / p->time / GIBIBYTE;
  INFO_PAIR("throughput","%.2f\n", tp);
  INFO_PAIR("throughput-stonewall","%.2f\n", p->stonewall_avg_data_accessed / p->time / GIBIBYTE * opt.mpi_size);

  return tp;
}


double ior_process_read(u_argv_t * argv, FILE * out, IOR_point_t ** res_out){
  IOR_results_t * res = ior_run(argv->size, argv->vector, MPI_COMM_WORLD, out)->results;
  u_res_file_close(out);
  u_argv_free(argv);

  IOR_point_t * p = & res->read;
  *res_out = p;

  if(res->errors){
    WARNING("Errors (%d) occured during phase in IOR. This invalidates your run.\n", res->errors);
    opt.is_valid_run = 0;
  }
  double tp = p->aggFileSizeForBW / p->time / GIBIBYTE;
  INFO_PAIR("throughput","%.2f\n", tp);

  return tp;
}

u_phase_t p_ior_easy = {
  "ior-easy",
  option,
  validate,
  NULL
};

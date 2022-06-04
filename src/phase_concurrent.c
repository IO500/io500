#include <sys/stat.h>
#include <unistd.h>
#include <mpi.h>
#include <math.h>

#include <io500-phase.h>
#include <phase_concurrent.h>

#define CBENCHS 3

typedef struct{
  int run;
  char * command[CBENCHS];
  mdworkbench_results_t * mdw;
  IOR_point_t * iorr;
  IOR_point_t * iorw;
  int color; // benchmark that is run
} opt_concurrent_bench;

static opt_concurrent_bench o;

static ini_option_t option[] = {
  {"run", "Run this phase", 0, INI_BOOL, "TRUE", & o.run},
  //{"", "XX", 0, INI_UINT, NULL, & o.XX},
  {NULL} };

static double run(void){
  opt_concurrent_bench d = o;
  
  if(opt.mpi_size < 5){
    INVALID("The concurrent phase needs at least 5 processes\n");
    return 0;
  }
  
  MPI_Comm ccom;
  /*
    Split processes into 3 groups and benchmarks:
    benchmark 0 - 20% parallel write - ior easy
    benchmark 1 - 40% parallel read 1 MB - rnd1MB read
    benchmark 2 - 40% md-workbench for concurrent usage
  */
  int workloads[] = {opt.mpi_size * 0.2, opt.mpi_size * 0.6, opt.mpi_size};
  int color = (opt.rank < workloads[0]) ? 0 : (opt.rank < workloads[1] ? 1 : 2);
  o.color = color;
  UMPI_CHECK(MPI_Comm_split(MPI_COMM_WORLD, color, opt.rank, & ccom));
  
  int crank;
  MPI_Comm_rank(ccom, & crank);
  /* printf("%d - %d - rank %d\n", opt.rank, color, crank); */
  PRINT_PAIR("benchmark", "%d\n", color);
    
  u_argv_t * argv[CBENCHS];
  for(int i=0; i < CBENCHS; i++){
    argv[i] = u_argv_create();
  }
    
  /* prepare benchmark settings */ 
  {
  opt_ior_easy d = ior_easy_o;
  ior_easy_add_params(argv[0], 0);
  u_argv_push(argv[0], "-w");	/* write file */
  u_argv_push(argv[0], "-D");	/* deadline for stonewall in seconds */
  u_argv_push_printf(argv[0], "%d", opt.stonewall);
  //u_argv_push_default_if_set_api_options(argv[0], "-a", d.api, o.api);
  u_argv_push_default_if_set_api_options(argv[0], "-a", d.api, d.api); // TODO USE RND WRITE OPTION
  u_argv_push(argv[0], "-O");
  u_argv_push_printf(argv[0], "saveRankPerformanceDetailsCSV=%s/concurrent-ior-easy-write.csv", opt.resdir);
  o.command[0] = u_flatten_argv(argv[0]);
  PRINT_PAIR("exe-easy-write", "%s\n", o.command[0]);
  }
  
  {
  opt_ior_rnd d = ior_rnd1MB_o;
  ior_rnd1MB_add_params(argv[1]);
  u_argv_push(argv[1], "-r");
  u_argv_push(argv[1], "-R");
  u_argv_push_default_if_set_api_options(argv[1], "-a", d.api, d.api); // TODO USE RND WRITE OPTION
  u_argv_push(argv[1], "-O");
  u_argv_push_printf(argv[1], "saveRankPerformanceDetailsCSV=%s/concurrent-ior-rnd1MB-read.csv", opt.resdir);
  u_argv_push(argv[0], "-D");	/* deadline for stonewall in seconds */
  u_argv_push_printf(argv[0], "%d", opt.stonewall);
  o.command[1] = u_flatten_argv(argv[1]);
  PRINT_PAIR("exe-rnd1MB-read", "%s\n", o.command[1]);
  }
  
  {
  mdworkbench_add_params(argv[2], 0);
  u_argv_push(argv[2], "-R=1"); /* 1 repetition */
  u_argv_push(argv[2], "-X"); /* turn read verification on */
  u_argv_push_printf(argv[2], "-w=%d", opt.stonewall);
  u_argv_push_printf(argv[2], "-o=%s/mdworkbench", opt.datadir);
  u_argv_push_printf(argv[2], "--run-info-file=%s/mdworkbench.status", opt.resdir );  
  u_argv_push(argv[2], "-2"); /* run pre-create or benchmarking phase */
  o.command[2] = u_flatten_argv(argv[2]);  
  PRINT_PAIR("exe-md-workbench", "%s\n", o.command[2]);
  }
  
  if(opt.dry_run || d.run == 0){
    for(int i=0; i < CBENCHS; i++){
      u_argv_free(argv[i]);
    }
    return 0;
  }    
  double score = 0;
  double time = 0;
  if(color == 0){    
    // run ior easy write
    if(ior_easy_o.filePerProc){
        char filename[2048];
        sprintf(filename, "ior-easy/ior_file_easy.%08d", opt.rank);
        u_purge_file(filename);
    }    
    FILE * out = u_res_file_prep("concurrent-ior-easy-write", crank);
    score = ior_process_write(argv[0], out, & o.iorw, ccom);
    time = o.iorw->time;
  }else if(color == 1){
    // run ior rnd 1MB read
    FILE * out = u_res_file_prep("concurrent-rnd1MB-read", crank);
    score = ior_process_read(argv[1], out, & o.iorr, ccom);
    time = o.iorr->time;
  }else if(color == 2){
    // run md_workbench
    FILE * out = u_res_file_prep("concurrent-md-workbench", crank);
    mdworkbench_process(argv[2], out, & o.mdw, ccom);
    /* calculate score */
    double rate = o.mdw->result[0].rate;
    PRINT_PAIR("maxOpTime", "%f\n", o.mdw->result[0].max_op_time);
    score = rate / 1000.0;
    time = o.mdw->result[0].runtime;
  }
  
  if(crank == 0){    
    // exchange scores, calculate geometric mean score
    double scores[CBENCHS];
    double times[CBENCHS];
    if(opt.rank == 0){
      scores[0] = score;
      times[0] = time;
      double aggregated_score = score / workloads[0];
      //printf("%f - %f - count: %d\n", score, aggregated_score, workloads[0]);      
      for(int i=1; i < CBENCHS; i++){
        UMPI_CHECK(MPI_Recv(& scores[i], 1, MPI_DOUBLE, workloads[i-1], 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE));
        UMPI_CHECK(MPI_Recv(& times[i], 1, MPI_DOUBLE, workloads[i-1], 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE));
        aggregated_score = aggregated_score * (scores[i] / (workloads[i] - workloads[i-1]));
        //printf("%f - %f - count: %d\n", scores[i], aggregated_score, (workloads[i] - workloads[i-1]));
      }
      PRINT_PAIR("score-ior-easy-write", "%f\n", scores[0]);
      PRINT_PAIR("score-ior-rnd1MB-read", "%f\n", scores[1]);
      PRINT_PAIR("score-ior-md-workbench", "%f\n", scores[2]);
      PRINT_PAIR("time-ior-easy-write", "%f\n", times[0]);
      PRINT_PAIR("time-ior-rnd1MB-read", "%f\n", times[1]);
      PRINT_PAIR("time-ior-md-workbench", "%f\n", times[2]);
      score = pow(aggregated_score * opt.mpi_size, 1.0/CBENCHS);
    }else{
      UMPI_CHECK(MPI_Send(& score, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD));
      UMPI_CHECK(MPI_Send(& time, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD));
    }
  }else{
    score = 0;
  }
  return score;
}


u_phase_t p_concurrent = {
  "concurrent",
  IO500_PHASE_WRITE | IO500_PHASE_FLAG_OPTIONAL,
  option,
  NULL,
  run,
  0,
  .group = IO500_SCORE_CONCURRENT,
};

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
  float ratio_write;
  float ratio_read;
} opt_concurrent_bench;

static opt_concurrent_bench o;

static ini_option_t option[] = {
  {"run", "Run this phase", 0, INI_BOOL, "TRUE", & o.run},
  {"ratio-write", "The ratio of processes that perform write - this ratio has the highest priority", 0, INI_FLOAT, "0.2", & o.ratio_write},
  {"ratio-read", "The ratio of processes that perform random read - remaining processes will do metadata", 0, INI_FLOAT, "0.4", & o.ratio_read},  
  {NULL} };

static double run(void){
  opt_concurrent_bench d = o;
    
  MPI_Comm ccom;
  /*
    Split processes into 3 groups and benchmarks:
    benchmark 0 - 20% parallel write - ior easy
    benchmark 1 - 40% parallel read 1 MB - rnd1MB read
    benchmark 2 - 40% md-workbench for concurrent usage
  */
  /* Sanity check of parameters */
  if(d.ratio_write > 1){
    d.ratio_write = 0;
  }
  if(d.ratio_read > 1){
    d.ratio_read = 0;
  }

  int workloads[] = {opt.mpi_size * d.ratio_write, opt.mpi_size * (d.ratio_write+d.ratio_read), opt.mpi_size};
  int procs[] = {workloads[0], workloads[1] - workloads[0], workloads[2] - workloads[1]};
  
  if(procs[0] + procs[1] == 0 || procs[0] + procs[2] == 0 || procs[1] + procs[2] == 0){
    INVALID("The concurrent phase doesn't make sense with two benchmarks using 0 processes\n");
    return 0;
  }
  
  int color = (opt.rank < workloads[0]) ? 0 : (opt.rank < workloads[1] ? 1 : 2);
  o.color = color;
  UMPI_CHECK(MPI_Comm_split(MPI_COMM_WORLD, color, opt.rank, & ccom));
  
  int crank;
  MPI_Comm_rank(ccom, & crank);
  /* printf("%d - %d - rank %d\n", opt.rank, color, crank); */
  if(opt.rank == 0){
    PRINT_PAIR("ranks-write", "%d\n", procs[0]);
    PRINT_PAIR("ranks-read", "%d\n", procs[1]);
    PRINT_PAIR("ranks-md", "%d\n", procs[2]);
  }
    
  u_argv_t * argv[CBENCHS];
  for(int i=0; i < CBENCHS; i++){
    argv[i] = u_argv_create();
  }
    
  /* prepare benchmark settings */ 
  {
  opt_ior_easy d = ior_easy_o;
  ior_easy_add_params(argv[0], 0, 1);
  u_argv_push(argv[0], "-w");	/* write file */
  u_argv_push_printf(argv[0], "-D=%d", opt.stonewall); /* deadline for stonewall in seconds */
  u_argv_push_printf(argv[0], "-O=minTimeDuration=%d", opt.stonewall); /* minimum runtime */
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
  u_argv_push_default_if_set_api_options(argv[1], "-a", d.api, d.api); // TODO USE RND WRITE OPTION
  u_argv_push(argv[1], "-O");
  u_argv_push_printf(argv[1], "saveRankPerformanceDetailsCSV=%s/concurrent-ior-rnd1MB-read.csv", opt.resdir);
  u_argv_push_printf(argv[1], "-D=%d", opt.stonewall); /* deadline for stonewall in seconds */
  u_argv_push_printf(argv[1], "-O=minTimeDuration=%d", opt.stonewall); /* minimum runtime */
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
    if(o.iorw){
      time = o.iorw->time;
    }
  }else if(color == 1){
    // run ior rnd 1MB read
    FILE * out = u_res_file_prep("concurrent-rnd1MB-read", crank);
    score = ior_process_read(argv[1], out, & o.iorr, ccom);
    if(o.iorr){
      time = o.iorr->time;
    }
  }else if(color == 2){
    // run md_workbench
    FILE * out = u_res_file_prep("concurrent-md-workbench", crank);
    mdworkbench_process(argv[2], out, & o.mdw, ccom);
    /* calculate score */
    double rate = o.mdw->result[0].rate;
    PRINT_PAIR("maxOpTime", "%f\n", o.mdw->result[0].max_op_time);
    score = rate / 1000.0;
    if(o.mdw){
      time = o.mdw->result[0].runtime;
    }
  }
  
  if(crank == 0){    
    // exchange scores, calculate geometric mean score
    double scores[CBENCHS] = {0};
    double times[CBENCHS] = {0};
    if(opt.rank == 0){
      MPI_Request req_score, req_time;
      UMPI_CHECK(MPI_Isend(& score, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, & req_score));
      UMPI_CHECK(MPI_Isend(& time, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, & req_time));
      
      double aggregated_score = 1;
      int benchmarks = 0;
      if (procs[0] != 0){
        scores[0] = score;
        times[0] = time;
        benchmarks++;
      }
      for(int i=1; i < CBENCHS; i++){
        if (procs[i] == 0){
          scores[i] = 0;
          times[i] = 0;
          continue;
        }
        benchmarks++;
        UMPI_CHECK(MPI_Recv(& scores[i], 1, MPI_DOUBLE, workloads[i-1], 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE));
        UMPI_CHECK(MPI_Recv(& times[i], 1, MPI_DOUBLE, workloads[i-1], 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE));
        aggregated_score = aggregated_score * (scores[i] / procs[i]);
      }
      PRINT_PAIR("score-ior-easy-write", "%f\n", scores[0]);
      PRINT_PAIR("score-ior-rnd1MB-read", "%f\n", scores[1]);
      PRINT_PAIR("score-ior-md-workbench", "%f\n", scores[2]);
      PRINT_PAIR("time-ior-easy-write", "%f\n", times[0]);
      PRINT_PAIR("time-ior-rnd1MB-read", "%f\n", times[1]);
      PRINT_PAIR("time-ior-md-workbench", "%f\n", times[2]);
      score = pow(aggregated_score, 1.0/benchmarks) * opt.mpi_size;
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

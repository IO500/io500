#include <phase_ior.h>

double ior_process_write(u_argv_t * argv, FILE * out, IOR_point_t ** res_out){
  IOR_results_t * res = ior_run(argv->size, argv->vector, MPI_COMM_WORLD, out)->results;
  u_res_file_close(out);
  u_argv_free(argv);

  if(opt.rank != 0){
    return 0;
  }

  IOR_point_t * p = & res->write;
  *res_out = p;

  if(res->errors){
    WARNING("Errors (%d) occured during phase in IOR. This invalidates your run.\n", res->errors);
    opt.is_valid_run = 0;
  }
  if( p->time < opt.stonewall ){
    WARNING("Write phase needed %fs instead of stonewall %ds. Stonewall was hit at %.1fs\n", p->time, opt.stonewall, p->stonewall_time);
    opt.is_valid_run = 0;
  }
  INFO_PAIR("accessed-pairs", "%zu\n", p->pairs_accessed);

  INFO_PAIR("throughput-stonewall","%.2f\n", p->stonewall_avg_data_accessed / p->time / GIBIBYTE * opt.mpi_size);
  double tp = p->aggFileSizeForBW / p->time / GIBIBYTE;

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
  return tp;
}

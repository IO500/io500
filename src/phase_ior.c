#include <phase_ior.h>
#include <assert.h>


double ior_process_write(u_argv_t * argv, FILE * out, IOR_point_t ** res_out){
  IOR_test_t * test = ior_run(argv->size, argv->vector, MPI_COMM_WORLD, out);
  assert(test);
  IOR_results_t * res = test->results;
  assert(res);
  u_res_file_close(out);
  u_argv_free(argv);

  if(opt.rank != 0){
    return 0;
  }

  IOR_point_t * p = & res->write;
  *res_out = p;

  if(res->errors){
    INVALID("Errors (%d) occured during phase in IOR. This invalidates your run.\n", res->errors);
  }
  if( p->time < opt.stonewall ){
    INVALID("Write phase needed %fs instead of stonewall %ds. Stonewall was hit at %.1fs\n", p->time, opt.stonewall, p->stonewall_time);
  }
  INFO_PAIR("accessed-pairs", "%zu\n", p->pairs_accessed);

  PRINT_PAIR("throughput-stonewall","%.2f\n", p->stonewall_avg_data_accessed * opt.mpi_size / p->stonewall_time / GIBIBYTE);
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
    INVALID("Errors (%d) occured during phase in IOR. This invalidates your run.\n", res->errors);
  }
  double tp = p->aggFileSizeForBW / p->time / GIBIBYTE;
  return tp;
}

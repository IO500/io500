#include <stdio.h>
#include <errno.h>

#include <io500-util.h>
#include <io500-phase.h>

#include <ior.h>

#include <phase-definitions.h>

// Dummy prototypes to satisfy need for depending modules
// TODO use ifdefs to strip dependency
void pfind_find(void){}
void pfind_aggregrate_results(void){}
double GetTimeStamp(void){ return 0; }
void pfind_parse_args(void){}
IOR_test_t * ior_run(int argc, char **argv, MPI_Comm world_com, FILE * out_logfile){ return NULL; }
void mdtest_run(void){}
FILE* out_logfile;
FILE* file_out;

int main(int argc, char ** argv){
  file_out = stdout;
  ini_section_t ** cfg = u_options();
  opt.verbosity = 0;

  PRINT_PAIR("version", "%s\n", VERSION);
  if (argc < 3){
    printf("Synopsis: %s <INI file> <RESULT file>\n\n", argv[0]);
    exit(0);
  }
  u_ini_parse_file(argv[1], cfg, NULL, NULL);

  u_verify_result_files(cfg, argv[argc-1]);
  return 0;
}

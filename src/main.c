#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <mpi.h>
#include <string.h>

#include <io500-util.h>
#include <io500-phase.h>

static u_phase_t * phases[IO500_PHASES] = {
  & p_opt,
  & p_debug,

  & p_ior_easy,
  & p_ior_easy_write,

  & p_mdtest_easy,
  & p_mdtest_easy_write,

  & p_ior_hard,
  & p_ior_hard_write,

  & p_mdtest_hard,
  & p_mdtest_hard_write,

  & p_find,

  & p_ior_easy_read,
  & p_mdtest_easy_stat,

  & p_ior_hard_read,
  & p_mdtest_hard_stat,

  & p_mdtest_easy_delete,
  & p_mdtest_hard_read,
  & p_mdtest_hard_delete,
};

static ini_section_t ** options(void){
  ini_section_t ** ini_section = u_malloc(sizeof(ini_section_t*) * (IO500_PHASES + 2));
  for(int i=0; i < IO500_PHASES; i++){
    ini_section[i] = u_malloc(sizeof(ini_section_t));
    ini_section[i]->name = phases[i]->name;
    ini_section[i]->option = phases[i]->options;
  }
  return ini_section;
}

static void parse_ini_file(char * file, ini_section_t** cfg){
  struct stat statbuf;
  int ret = stat(file, & statbuf);
  if(ret != 0){
    FATAL("Cannot open config file %s\n", file);
  }

  char * buff = "";
  if(statbuf.st_size > 0){
    buff = malloc(statbuf.st_size + 1);
    if(! buff){
      FATAL("Cannot malloc();")
    }

    FILE * f = fopen(file, "r");
    if(ret != 0){
      FATAL("Cannot open config file %s\n", file);
    }
    ret = fread(buff, statbuf.st_size, 1, f);
    fclose(f);
    if( ret != 1 ){
      FATAL("Couldn't read config file %s\n", file);
    }
    buff[statbuf.st_size] = '\0';
  }

  ret = u_parse_ini(buff, cfg);
  if (ret != 0){
    FATAL("Couldn't parse config file %s\n", file);
  }

  free(buff);
}

int main(int argc, char ** argv){
  ini_section_t ** cfg = options();

  MPI_Init(& argc, & argv);
  MPI_Comm_rank(MPI_COMM_WORLD, & opt.rank);
  MPI_Comm_size(MPI_COMM_WORLD, & opt.mpi_size);

  if (argc < 2){
    help:
    r0printf("Synopsis: %s <INI file> [-v=<verbosity level>] [--dry-run]\n\n", argv[0]);
    r0printf("Supported and current values of the ini file:\n");
    u_ini_print_values(cfg);
    exit(1);
  }

  int print_help = 0;
  if(argc > 2){
    for(int i = 2; i < argc; i++){
      if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0 ){
        print_help = 1;
      }else if(strncmp(argv[i], "-v=", 3) == 0){
        opt.verbosity = atoi(argv[i]+3);
      }else if(strcmp(argv[i], "--dry-run") == 0 ){
        opt.dry_run = 1;
      }else{
        FATAL("Unknown option: %s\n", argv[i]);
      }
    }
  }

  parse_ini_file(argv[1], cfg);
  if(print_help){
    goto help;
  }

  MPI_Barrier(MPI_COMM_WORLD);
  if(opt.verbosity > 0 && opt.rank == 0){
    printf("; START ");
    u_print_timestamp();
    printf("\n");
  }

  for(int i=0; i < IO500_PHASES; i++){
    phases[i]->validate();
  }

  printf("\n");

  for(int i=0; i < IO500_PHASES; i++){
    if(! phases[i]->run) continue;
    MPI_Barrier(MPI_COMM_WORLD);
    if(opt.verbosity > 0 && opt.rank == 0){
      printf("[%s]\nstart=", phases[i]->name);
      u_print_timestamp();
      printf("\n");
    }

    double score = phases[i]->run();
    printf("score=%f\n", score);

    if(opt.verbosity > 0 && opt.rank == 0){
      printf("end=");
      u_print_timestamp();
      printf("\n\n");
    }
  }

  MPI_Barrier(MPI_COMM_WORLD);
  if(opt.verbosity > 0 && opt.rank == 0){
    printf("; END ");
    u_print_timestamp();
    printf("\n");
  }
  MPI_Finalize();
  return 0;
}

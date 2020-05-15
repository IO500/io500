#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <mpi.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <sys/types.h>
#include <dirent.h>

#include <io500-util.h>
#include <io500-phase.h>

#include <phase-definitions.h>

FILE* file_out = NULL;

static char const * io500_phase_str[IO500_SCORE_LAST] = {
  "NO SCORE",
  "MD",
  "BW"};

static void init_dirs(void){
  // load general IO backend for data dir
  aiori_initialize(NULL);
  opt.aiori = aiori_select(opt.api);
  if(opt.aiori == NULL){
    FATAL("Could not load AIORI backend for %s\n", opt.api);
  }

  if(opt.rank == 0 && opt.timestamp == NULL){
    char buffer[30];
    struct tm* tm_info;
    time_t timer;
    time(&timer);
    tm_info = localtime(&timer);
    strftime(buffer, 30, "%Y.%m.%d-%H.%M.%S", tm_info);
    opt.timestamp = strdup(buffer);
  }
  UMPI_CHECK(MPI_Bcast(opt.timestamp, 30, MPI_CHAR, 0, MPI_COMM_WORLD));

  char resdir[PATH_MAX];
  if(opt.timestamp_resdir){
    sprintf(resdir, "%s/%s-app", opt.resdir, opt.timestamp);
  }else{
    sprintf(resdir, "%s/io500-app", opt.resdir);
  }
  opt.resdir = strdup(resdir);

  if(opt.timestamp_datadir){
    sprintf(resdir, "%s/%s-app", opt.datadir, opt.timestamp);
  }else{
    sprintf(resdir, "%s/io500-app", opt.datadir);
  }
  opt.datadir = strdup(resdir);

  if(opt.rank == 0){
    u_create_dir_recursive(opt.resdir, "POSIX");

    u_create_datadir("");
  }
}

static void print_cfg_hash(FILE * out, ini_section_t ** cfg){
  if(opt.rank == 0){
    PRINT_PAIR_HEADER("config-hash");
    uint32_t hash = u_ini_gen_hash(cfg);
    u_hash_print(out, hash);
    fprintf(out, "\n");
  }
}

#define dupprintf(...) do{ if(opt.rank == 0) { fprintf(res_summary, __VA_ARGS__); printf(__VA_ARGS__); } }while(0);


int main(int argc, char ** argv){
  int mpi_init = 0;
  file_out = stdout;

  ini_section_t ** cfg = u_options();

  if (argc < 2 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0){
    help:
    r0printf("Synopsis: %s <INI file> [-v=<verbosity level>] [--dry-run] [--cleanup] [--config-hash] [--timestamp <timestamp>]\n\n", argv[0]);
    r0printf("--config-hash Compute the configuration hash\n");
    r0printf("--cleanup will run the delete phases of the benchmark useful to get rid of a partially executed benchmark\n");
    r0printf("--dry-run will show the executed IO benchmark arguments but not run them (It will run drop caches, though, if enabled)\n");
    r0printf("--list list available options for the .ini file\n");
    r0printf("--timestamp use <timestamp> for the output directory\n");
    r0printf("--verify to verify that the output hasn't been modified accidentially; call like: io500 test.ini --verify test.out\n\n");

    goto out;
  }
  if (argc < 2 || strcmp(argv[1], "-l") == 0 || strcmp(argv[1], "--list") == 0){
    if (rank == 0){
      r0printf("Supported and current values of the ini file:\n");
      u_ini_print_values(stdout, cfg, TRUE);
    }
    goto out;
  }

  mpi_init = 1;
  MPI_Init(& argc, & argv);
  MPI_Comm_rank(MPI_COMM_WORLD, & opt.rank);
  MPI_Comm_size(MPI_COMM_WORLD, & opt.mpi_size);

  init_IOR_Param_t(& opt.aiori_params);
  opt.is_valid_run = 1;

  int verbosity_override = -1;
  int print_help = 0;

  int config_hash_only = 0;
  int cleanup_only = 0;
  int verify_only = 0;
  if(argc > 2){
    for(int i = 2; i < argc; i++){
      if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0 ){
        print_help = 1;
      }else if(strncmp(argv[i], "-v=", 3) == 0){
        verbosity_override = atoi(argv[i]+3);
        opt.verbosity = verbosity_override;
      }else if(strcmp(argv[i], "--cleanup") == 0 ){
        cleanup_only = 1;
      }else if(strcmp(argv[i], "--config-hash") == 0 ){
        config_hash_only = 1;
      }else if(strcmp(argv[i], "--dry-run") == 0 ){
        opt.dry_run = 1;
      }else if(strncmp(argv[i], "--timestamp", sizeof("--timestamp") - 1) == 0){
        if (strchr(argv[i], '=') != NULL)
          opt.timestamp = strdup(strchr(argv[i], '=') + 1);
        else if (argv[i + 1] != NULL && argv[i + 1][0] != '\0')
          opt.timestamp = strdup(argv[++i]);
        else
          FATAL("Missing timestamp argument\n");
      }else if(strcmp(argv[i], "--verify") == 0 ){
        verify_only = 1;
        break;
      }else{
        FATAL("Unknown option: %s\n", argv[i]);
      }
    }
  }


  char * ini_data = NULL;
  {
    int ini_len = 0;
    if( opt.rank == 0){
      u_ini_parse_file(argv[1], cfg, NULL, & ini_data);
      ini_len = strlen(ini_data);
      UMPI_CHECK(MPI_Bcast(& ini_len, 1, MPI_INT, 0, MPI_COMM_WORLD));
      UMPI_CHECK(MPI_Bcast(ini_data, ini_len, MPI_CHAR, 0, MPI_COMM_WORLD));
    }else{
      UMPI_CHECK(MPI_Bcast(& ini_len, 1, MPI_INT, 0, MPI_COMM_WORLD));
      ini_data = u_malloc(ini_len + 1);
      ini_data[ini_len] = 0;
      UMPI_CHECK(MPI_Bcast(ini_data, ini_len, MPI_CHAR, 0, MPI_COMM_WORLD));
      int ret = u_parse_ini(ini_data, cfg, NULL);
      if (ret != 0){
        FATAL("Couldn't parse config file on rank %d\n", opt.rank);
      }
    }
  }
  if(verbosity_override > -1){
    opt.verbosity = verbosity_override;
  }
  if(print_help){
    goto help;
  }

  if(verify_only){
    if(argc == 3){
      FATAL("--verify option requires the output file as last parameter!");
    }
    if(verbosity_override == -1){
      opt.verbosity = 0;
    }
    u_verify_result_files(cfg, argv[argc-1]);
    goto out;
  }

  if(config_hash_only){
    print_cfg_hash(stdout, cfg);
    goto out;
  }

  init_dirs();

  FILE * res_summary = NULL;
  if(opt.rank == 0){
    char file[2048];
    sprintf(file, "%s/result_summary.txt", opt.resdir);
    res_summary = fopen(file, "w");
    if(! res_summary){
      FATAL("Could not open \"%s\" for writing (%s)\n", file, strerror(errno));
    }
    sprintf(file, "%s/result.txt", opt.resdir);
    file_out = fopen(file, "w");
    if(! file_out){
      FATAL("Could not open \"%s\" for writing (%s)\n", file, strerror(errno));
    }

    sprintf(file, "%s/config-orig.ini", opt.resdir);
    FILE * fd = fopen(file, "w");
    fwrite(ini_data, strlen(ini_data), 1, fd);
    fclose(fd);
  }

  PRINT_PAIR("version", "%s\n", VERSION);
  print_cfg_hash(file_out, cfg);
  PRINT_PAIR("result-dir", "%s\n", opt.resdir);

  if(opt.rank == 0){
    // create configuration in result directory to ensure it is preserved
    char file[2048];
    sprintf(file, "%s/config.ini", opt.resdir);
    FILE * fd = fopen(file, "w");
    if(! fd){
      FATAL("Couldn't write the configuration file to the result directory: %s (%s)\n", file, strerror(errno));
    }
    u_ini_print_values(fd, cfg, 0);
    fclose(fd);
  }

  if(opt.dry_run){
    INVALID("DRY RUN MODE ACTIVATED\n");
  }

  MPI_Barrier(MPI_COMM_WORLD);
  if(opt.verbosity > 0 && opt.rank == 0){
    fprintf(file_out, "; START ");
    u_print_timestamp(file_out);
    fprintf(file_out, "\n");
  }

  for(int i=0; i < IO500_PHASES; i++){
    phases[i]->validate();
  }
  if(opt.rank == 0){
    fprintf(file_out, "\n");
  }

  // manage a hash for the scores
  uint32_t score_hash = 0;
  u_hash_update_key_val(& score_hash, "version", VERSION);

  dupprintf("IO500 version %s\n", VERSION);

  for(int i=0; i < IO500_PHASES; i++){
    u_phase_t * phase = phases[i];
    if(! phase->run) continue;
    if( cleanup_only && phase->type != IO500_PHASE_REMOVE ) continue;

    if(opt.drop_caches && phase->type != IO500_PHASE_DUMMY){
      DEBUG_INFO("Dropping cache\n");
      if(opt.rank == 0)
        u_call_cmd("LANG=C free -m");
      u_call_cmd(opt.drop_caches_cmd);
      if(opt.rank == 0)
        u_call_cmd("LANG=C free -m");
    }

    MPI_Barrier(MPI_COMM_WORLD);
    if(opt.rank == 0){
      fprintf(file_out, "\n[%s]\n", phase->name);
      if(opt.verbosity > 0){
        PRINT_PAIR_HEADER("t_start");
        u_print_timestamp(file_out);
        fprintf(file_out, "\n");
      }
    }

    double start = GetTimeStamp();
    double score = phase->run();
    double runtime = GetTimeStamp() - start;

    if(phase->group > IO500_NO_SCORE){
      if(opt.rank == 0){
        PRINT_PAIR("score", "%f\n", score);

        char score_str[40];
        sprintf(score_str, "%f", score);
        dupprintf("[RESULT%s] %20s %15s %s : time %.3f seconds\n",
		  phase->type == IO500_PHASE_WRITE && runtime < IO500_MINWRITE ?
			"-invalid" : "",
		  phase->name, score_str, phase->name[0] == 'i' ? "GiB/s " : "kIOPS", runtime);
      }
      u_hash_update_key_val_dbl(& score_hash, phase->name, score);
    }
    phases[i]->score = score;

    // This is an additional sanity check
    if( phases[i]->verify_stonewall && opt.rank == 0){
      if(runtime < opt.stonewall && ! opt.dry_run){
        INVALID("Runtime of phase (%f) is below stonewall time. This shouldn't happen!\n", runtime);
      }
    }

    if(opt.verbosity > 0 && opt.rank == 0){
      PRINT_PAIR("t_delta", "%.4f\n", runtime);
      PRINT_PAIR_HEADER("t_end");
      u_print_timestamp(file_out);
      fprintf(file_out, "\n");
    }
  }

  MPI_Barrier(MPI_COMM_WORLD);
  if(opt.rank == 0){
    // compute the overall score
    fprintf(file_out, "\n[SCORE]\n");
    double overall_score = 1;
    double scores[IO500_SCORE_LAST];

    for(int g=1; g < IO500_SCORE_LAST; g++){
      char score_string[2048];
      char *p = score_string;
      double score = 1;
      int numbers = 0;
      p += sprintf(p, " %s = (", io500_phase_str[g]);
      for(int i=0; i < IO500_PHASES; i++){
        if(phases[i]->group == g){
          double t = phases[i]->score;
          score *= t;
          if(numbers > 0)
            p += sprintf(p, " * ");
          numbers++;
          p += sprintf(p, "%.8f", t);
        }
      }
      DEBUG_INFO("%s)^%f\n", score_string, 1.0/numbers);
      score = pow(score, 1.0/numbers);
      scores[g] = score;
      PRINT_PAIR(io500_phase_str[g], "%f\n", score);
      u_hash_update_key_val_dbl(& score_hash, io500_phase_str[g], score);

      overall_score *= score;
    }
    overall_score = sqrt(overall_score);
    PRINT_PAIR("SCORE", "%f %s\n", overall_score, opt.is_valid_run ? "" : " [INVALID]");
    u_hash_update_key_val_dbl(& score_hash, "SCORE", overall_score);
    if( ! opt.is_valid_run ){
      u_hash_update_key_val(& score_hash, "valid", "NO");
    }
    PRINT_PAIR("hash", "%X\n", (int) score_hash);

    dupprintf("[SCORE%s] Bandwidth %f GB/s : IOPS %f kiops : TOTAL %f\n",
      opt.is_valid_run ? "" : "-invalid",
      scores[IO500_SCORE_BW], scores[IO500_SCORE_MD], overall_score);
  }

  for(int i=0; i < IO500_PHASES; i++){
    if(phases[i]->cleanup)
      phases[i]->cleanup();
  }

  if(opt.rank == 0){
    fclose(res_summary);
    u_purge_datadir("");
  }

  if(opt.rank == 0 && opt.verbosity > 0){
    fprintf(file_out, "; END ");
    u_print_timestamp(file_out);
    fprintf(file_out, "\n");
  }

  fclose(file_out);
out:
  if (mpi_init)
    MPI_Finalize();
  return 0;
}

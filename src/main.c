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

#define INVALID_RUN(...) do{ if (opt.rank == 0){fprintf(file_out, "; ERROR INVALID "__VA_ARGS__); printf("ERROR INVALID (%s:%d) ", __FILE__, __LINE__); printf(__VA_ARGS__); fflush(file_out); opt.is_valid_run = 0; } }while(0);

#define RUN_PHASE(phase) ( ! (phase->type & IO500_PHASE_FLAG_OPTIONAL && opt.mode == IO500_MODE_STANDARD) )

FILE* file_out = NULL;

static char const * io500_phase_str[IO500_SCORE_LAST] = {
  "NO SCORE",
  "MD",
  "BW"};

extern int rank;

static void prepare_aiori(void){
  // check selected API, might be followed by API options
  char * api = strdup(opt.api);
  char * token = strstr(api, " ");

  option_help options [] = {
    LAST_OPTION
  };
  options_all_t * global_options = airoi_create_all_module_options(options);
  if(token){
    *token = '\0';
    opt.apiArgs = strdup(opt.api);
    opt.api = api;

    // parse the API options, a bit cumbersome at the moment
    // find the next token for all the APIs
    token++;
    for(char * p = token ; ; p++){
      if(*p == ' '){
        *p = '\0';
        if( p - token > 1 && *token != ' '){
          DEBUG_INFO("API token: \"%s\"\n", token);
          if( option_parse_str(token, global_options) ){
            FATAL("Couldn't parse API option: %s\n", token);
          }
        }
        token = p + 1;
      }else if(*p == '\0'){
        break;
      }else if(p[0] == '\\' && p[1] == ' '){
        // skip escaped whitespace by moving it forward
        for(char * c = p ; *c != '\0' ; c++){
          c[0] = c[1];
        }
        p++;
      }
    }
    DEBUG_INFO("API token: \"%s\"\n", token);
    if( option_parse_str(token, global_options) ){
      FATAL("Coudln't parse API option: %s\n", token);
    }
  }
  opt.aiori = aiori_select(opt.api);
  opt.backend_opt = airoi_update_module_options(opt.aiori, global_options);
  if(opt.aiori == NULL){
    FATAL("Could not load AIORI backend for %s with options: %s\n", opt.api, opt.apiArgs);
  }
  if (opt.aiori->xfer_hints){
    memset(& opt.backend_hints, 0, sizeof(opt.backend_hints));
    opt.aiori->xfer_hints(& opt.backend_hints);
  }
  if(opt.aiori->check_params){
    opt.aiori->check_params(opt.backend_opt);
  }
  if (opt.aiori->initialize){
    opt.aiori->initialize(opt.backend_opt);
  }

  if(opt.timestamp == NULL){
    opt.timestamp = malloc(30);
    if(opt.rank == 0){
      struct tm* tm_info;
      time_t timer;
      time(&timer);
      tm_info = localtime(&timer);
      strftime(opt.timestamp, 30, "%Y.%m.%d-%H.%M.%S", tm_info);
    }
    UMPI_CHECK(MPI_Bcast(opt.timestamp, 30, MPI_CHAR, 0, MPI_COMM_WORLD));
  }

  if(opt.timestamp_resdir){
    char resdir[PATH_MAX];
    sprintf(resdir, "%s/%s", opt.resdir, opt.timestamp);
    opt.resdir = strdup(resdir);
  }

  if(opt.timestamp_datadir){
    char resdir[PATH_MAX];
    sprintf(resdir, "%s/%s", opt.datadir, opt.timestamp);
    opt.datadir = strdup(resdir);
  }

  if(opt.rank == 0){
    ior_aiori_t const * posix = aiori_select("POSIX");
    u_create_dir_recursive(opt.resdir, posix, NULL);
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

static double calc_score(double scores[IO500_SCORE_LAST], int extended, uint32_t * hash){
  double overall_score = 1;
  for(io500_phase_score_group g=1; g < IO500_SCORE_LAST; g++){
    char score_string[2048];
    char *p = score_string;
    double score = 1;
    int numbers = 0;
    p += sprintf(p, " %s = (", io500_phase_str[g]);
    for(int i=0; i < IO500_PHASES; i++){
      if(phases[i]->group == g && (extended || ! (phases[i]->type & IO500_PHASE_FLAG_OPTIONAL)) ){
        double t = phases[i]->score;
        if(t <= 0){
          continue;
        }
        score *= t;
        p += sprintf(p, " * ");
        numbers++;
        p += sprintf(p, "%.8f", t);
      }
    }
    if(numbers == 0){
      score = 0;
    }else{      
      DEBUG_INFO("%s)^%f\n", score_string, 1.0/numbers);
      score = pow(score, 1.0/numbers);
    }
    scores[g] = score;
    PRINT_PAIR(io500_phase_str[g], "%f\n", score);
    u_hash_update_key_val_dbl(hash, io500_phase_str[g], score);

    overall_score *= score;
  }
  return sqrt(overall_score);
}

static const char * io500_mode_str(io500_mode mode){
  switch(mode){
    case (IO500_MODE_STANDARD): return "standard";
    case (IO500_MODE_EXTENDED): return "extended";
    default:
      return "unknown";
  }
}

int main(int argc, char ** argv){
  int mpi_init = 0;
  file_out = stdout;
  opt = (io500_opt_t) {
    .mode = IO500_MODE_STANDARD,
    .is_valid_run = 1,
    .is_valid_extended_run = 1
  };

  ini_section_t ** cfg = u_options();

  if (argc < 2 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0){
    help:
    r0printf("Synopsis: %s <INI file> [-v=<verbosity level>] [--dry-run] [--cleanup] [--config-hash] [--timestamp <timestamp>]\n\n", argv[0]);
    r0printf("--config-hash Compute the configuration hash\n");
    r0printf("--cleanup will run the delete phases of the benchmark useful to get rid of a partially executed benchmark\n");
    r0printf("--dry-run will show the executed IO benchmark arguments but not run them (It will run drop caches, though, if enabled)\n");
    r0printf("--list list available options for the .ini file\n");
    r0printf("--mode=standard|extended define the mode to run the benchmark\n");
    r0printf("--timestamp use <timestamp> for the output directory\n");
    r0printf("--verify to verify that the output hasn't been modified accidentially; call like: io500 test.ini --verify test.out\n\n");

    goto out;
  }
  if (argc < 2 || strcmp(argv[1], "-l") == 0 || strcmp(argv[1], "--list") == 0){
    if (opt.rank == 0){
      /* print this as a comment, in case it is saved into the .ini file */
      r0printf("# Supported and current values of the ini file:\n");
      u_ini_print_values(stdout, cfg, TRUE);
    }
    goto out;
  }

  mpi_init = 1;
  MPI_Init(& argc, & argv);
  MPI_Comm_rank(MPI_COMM_WORLD, & opt.rank);
  MPI_Comm_size(MPI_COMM_WORLD, & opt.mpi_size);
  rank = opt.rank;

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
      }else if(strcmp(argv[i], "--mode=standard") == 0){
        opt.mode = IO500_MODE_STANDARD;
      }else if(strcmp(argv[i], "--mode=extended") == 0){
        opt.mode = IO500_MODE_EXTENDED;
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
      }else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--list") == 0){
        r0printf("# Supported and current values of the ini file:\n");
        u_ini_print_values(stdout, cfg, TRUE);
        exit(1);
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
  if(opt.scc){
    // student cluster competition
    opt.minwrite = 30;
  }else{
    opt.minwrite = 300;
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

  prepare_aiori();

  FILE * res_summary = NULL;
  if(opt.rank == 0){
    char file[PATH_MAX];
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
    fprintf(file_out, "[run]\n");    
    PRINT_PAIR("procs", "%d\n", opt.mpi_size);
    sprintf(file, "%s/config-orig.ini", opt.resdir);
    FILE * fd = fopen(file, "w");
    fwrite(ini_data, strlen(ini_data), 1, fd);
    fclose(fd);
  }
  
  PRINT_PAIR("version", "%s\n", VERSION);
  print_cfg_hash(file_out, cfg);
  PRINT_PAIR("result-dir", "%s\n", opt.resdir);
  PRINT_PAIR("mode", "%s\n", io500_mode_str(opt.mode));

  if(opt.rank == 0){
    // create configuration in result directory to ensure it is preserved
    char file[PATH_MAX];
    sprintf(file, "%s/config.ini", opt.resdir);
    FILE * fd = fopen(file, "w");
    if(! fd){
      FATAL("Couldn't write the configuration file to the result directory: %s (%s)\n", file, strerror(errno));
    }
    u_ini_print_values(fd, cfg, 0);
    fclose(fd);
  }

  if(opt.dry_run){
    INVALID_RUN("DRY RUN MODE ACTIVATED\n");
  }
  if(opt.pause_dir){
    INVALID_RUN("PAUSING BETWEEN RUNS ACTIVATED\n");
  }

  MPI_Barrier(MPI_COMM_WORLD);
  if(opt.verbosity > 0 && opt.rank == 0){
    fprintf(file_out, "; START ");
    u_print_timestamp(file_out);
    fprintf(file_out, "\n");
  }

  // manage a hash for the scores
  uint32_t score_hash = 0;
  uint32_t score_extended_hash = 0;
  u_hash_update_key_val(& score_hash, "version", VERSION);

  dupprintf("IO500 version %s (%s)\n", VERSION, io500_mode_str(opt.mode));

  for(int i=0; i < IO500_PHASES; i++){
    if(RUN_PHASE(phases[i]) && phases[i]->validate){
      phases[i]->validate();
    }
  }
  if(opt.rank == 0){
    fprintf(file_out, "\n");
  }

  for(int i=0; i < IO500_PHASES; i++){
    u_phase_t * phase = phases[i];
    if(! phase->run) continue;
    if( cleanup_only && ! (phase->type & IO500_PHASE_REMOVE) ) continue;
    if(! RUN_PHASE(phase)){
      continue;
    }

    if(opt.drop_caches && ! (phase->type & IO500_PHASE_DUMMY) ){
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
      
      if(opt.pause_dir){
        // if the file exists
        char path[PATH_MAX];
        int ret; 
        struct stat statbuf;
        if(opt.verbosity > 0){
          PRINT_PAIR_HEADER("t_pause");
          u_print_timestamp(file_out);
          fprintf(file_out, "\n");
        }
        sprintf(path, "%s/%s", opt.pause_dir, phase->name);
        fprintf(file_out, "; Checking for pause file %s\n", path);
        fflush(file_out);
        while(1){
          ret = stat(path, & statbuf);
          if(ret != 0){
            break;
          }
          sleep(1);
        }
        MPI_Barrier(MPI_COMM_WORLD);
      }
      
      if(opt.verbosity > 0){
        PRINT_PAIR_HEADER("t_start");
        u_print_timestamp(file_out);
        fprintf(file_out, "\n");
      }
    }

    if(opt.pause_dir && opt.rank != 0){
      MPI_Barrier(MPI_COMM_WORLD);
    }

    double start = GetTimeStamp();
    opt.is_valid_phase = 1;
    double score = phase->run();
    double runtime = GetTimeStamp() - start;

    if(opt.rank == 0){
      // This is an additional sanity check
      if(! opt.dry_run){
        if( phases[i]->verify_stonewall && runtime < opt.stonewall ){
          INVALID("Runtime of phase (%f) is below stonewall time. This shouldn't happen!\n", runtime);
        }else if(score == 0.0 && ! (phase->type & IO500_PHASE_DUMMY)){
          INVALID("Resulting score shouldn't be 0.0\n");
        }
      }
      if(runtime < opt.minwrite && phases[i]->verify_stonewall){
        INVALID("Runtime is smaller than expected minimum runtime\n");
      }
      if(! opt.is_valid_phase){
        opt.is_valid_extended_run = 0;
        if(! (phase->type & IO500_PHASE_FLAG_OPTIONAL)){
          opt.is_valid_run = 0;
        }
      }
      if(! (phase->type & IO500_PHASE_DUMMY)){
        PRINT_PAIR("score", "%f\n", score);
      }
      char * valid_str = opt.is_valid_phase ? "" : " [INVALID]";
      char score_str[40];
      sprintf(score_str, "%f", score);
      if(phase->group > IO500_NO_SCORE){
        dupprintf("[RESULT]");
      }else{
        dupprintf("[      ]");
      }
      dupprintf(" %20s %15s %s : time %.3f seconds%s\n", phase->name, score_str, phase->name[0] == 'i' ? "GiB/s" : "kIOPS", runtime, valid_str);
    }
    if(phase->group > IO500_NO_SCORE){
      if(phase->type & IO500_PHASE_FLAG_OPTIONAL){
        u_hash_update_key_val_dbl(& score_extended_hash, phase->name, score);
      }else{
        u_hash_update_key_val_dbl(& score_hash, phase->name, score);
      }
    }
    phases[i]->score = score;


    if(opt.verbosity > 0 && opt.rank == 0){
      PRINT_PAIR("t_delta", "%.4f\n", runtime);
      PRINT_PAIR_HEADER("t_end");
      u_print_timestamp(file_out);
      fprintf(file_out, "\n");
    }
  }

  MPI_Barrier(MPI_COMM_WORLD);
  if(opt.rank == 0){
    char * valid_str = opt.is_valid_run ? "" : " [INVALID]";
    // compute the overall score
    fprintf(file_out, "\n[SCORE]\n");
    double scores[IO500_SCORE_LAST];
    double overall_score = calc_score(scores, 0, & score_hash);
    PRINT_PAIR("SCORE", "%f%s\n", overall_score, valid_str);
    u_hash_update_key_val_dbl(& score_hash, "SCORE", overall_score);
    if( ! opt.is_valid_run ){
      u_hash_update_key_val(& score_hash, "valid", "NO");
    }
    PRINT_PAIR("hash", "%X\n", (int) score_hash);
    dupprintf("[SCORE ] Bandwidth %f GiB/s : IOPS %f kiops : TOTAL %f%s\n",
    scores[IO500_SCORE_BW], scores[IO500_SCORE_MD], overall_score, valid_str);

    // extended run
    if(opt.mode == IO500_MODE_EXTENDED){
      valid_str = opt.is_valid_extended_run ? "" : " [INVALID]";
      fprintf(file_out, "\n[SCOREX]\n");
      double overall_extended_score = calc_score(scores, 1, & score_extended_hash);
      u_hash_update_key_val_dbl(& score_extended_hash, "SCORE", overall_extended_score);
      if( ! opt.is_valid_extended_run ){
        u_hash_update_key_val(& score_extended_hash, "valid", "NO");
      }
      PRINT_PAIR("SCORE", "%f%s\n", overall_extended_score, valid_str);
      PRINT_PAIR("hash", "%X\n", (int) score_extended_hash);
      dupprintf("[SCOREX] Bandwidth %f GiB/s : IOPS %f kiops : TOTAL %f%s\n", scores[IO500_SCORE_BW], scores[IO500_SCORE_MD], overall_extended_score, valid_str);
    }

    printf("\nThe result files are stored in the directory: %s\n", opt.resdir);
  }

  for(int i=0; i < IO500_PHASES; i++){
    if(RUN_PHASE(phases[i]) && phases[i]->cleanup){
      phases[i]->cleanup();
    }
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

  if (opt.aiori->finalize){
    opt.aiori->finalize(opt.backend_opt);
  }

  fclose(file_out);
out:
  if (mpi_init)
    MPI_Finalize();
  return 0;
}

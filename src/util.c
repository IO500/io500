#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <assert.h>


#include <aiori.h>

#include <io500-util.h>
#include <io500-opt.h>


/**
 * rotate the value and add the current character
 */
static uint32_t rotladd (uint32_t x, char c){
  return ((x<<2) | (x>>(32-2))) + c;
}

uint32_t u_hash_update(uint32_t hash, char const * str){
  for(char const * c = str; *c != 0; c++){
    hash = rotladd(hash, *c);
  }
  return hash;
}

/* compute a unique random number based on the phase and the timestamp */
uint32_t u_phase_unique_random_number(char const * phase_name){
  uint32_t hash = 0;
  hash = u_hash_update(hash, phase_name);
  hash = u_hash_update(hash, opt.timestamp);
  if(hash == 0 || hash == -1){
    return 4711;
  }
  return hash;
}


void u_hash_update_key_val(uint32_t * hash, char const * key, char const * val){
  uint32_t hsh = 0;
  hsh = u_hash_update(hsh, key);
  hsh = u_hash_update(hsh, val);
  *hash = *hash ^ hsh;
  DEBUG_INFO("hash current: %X updated with (%s=%s)\n", (int)*hash, key, val);
}

void u_hash_update_key_val_dbl(uint32_t * hash, char const * key, double val){
  char str[40];
  sprintf(str, "%f", val);
  u_hash_update_key_val(hash, key, str);
}

void u_hash_print(FILE * file, uint32_t hash){
  fprintf(file, "%X", (int) hash);
}

void u_call_cmd(char const * str){
  int ret = system(str);
  if (ret != 0) {
    WARNING("Calling \"%s\" returned %d\n", str, ret);
  }
}
void u_purge_datadir(char const * dir){
  char d[PATH_MAX];
  sprintf(d, "%s/%s", opt.datadir, dir);
  DEBUG_INFO("Removing dir %s\n", d);

  opt.aiori->rmdir(d, opt.backend_opt);
}

void u_purge_file(char const * file){
  char f[PATH_MAX];
  sprintf(f, "%s/%s", opt.datadir, file);
  DEBUG_INFO("Removing file %s\n", f);
  opt.aiori->delete(f, opt.backend_opt);
}

void u_create_datadir(char const * dir){
  if(opt.rank != 0){
    return;
  }
  char d[PATH_MAX];
  sprintf(d, "%s/%s", opt.datadir, dir);
  u_create_dir_recursive(d, opt.aiori, opt.backend_opt);
}

void u_create_dir_recursive(char const * dir, ior_aiori_t const * api, aiori_mod_opt_t * module_options){
  char * d = strdup(dir);
  char outdir[PATH_MAX];
  char * wp = outdir;
  if (dir[0] == '/'){
    wp += sprintf(wp, "/");
  }

  char * next = strtok(d, "/");
  while(next){
    if(*next == '/' || *next == 0) continue;
    wp += sprintf(wp, "%s/", next);

    struct stat sb;
    int ret = api->stat(outdir, & sb, module_options);
    if(ret != 0){
      DEBUG_INFO("Creating dir %s\n", outdir);
      ret = api->mkdir(outdir, S_IRWXU, module_options);
      if(ret != 0){
        FATAL("Couldn't create directory %s (Error: %s)\n", outdir, strerror(errno));
      }
    }
    next = strtok(NULL, "/");
  }
  free(d);
}

u_argv_t * u_argv_create(void){
  u_argv_t * p = u_malloc(sizeof(u_argv_t));
  memset(p, 0, sizeof(u_argv_t));
  return p;
}

void u_argv_free(u_argv_t * argv){
  for(int i=0; i < argv->size; i++){
    free(argv->vector[i]);
  }
  free(argv);
}

void u_argv_push(u_argv_t * argv, char const * str){
  int size = ++argv->size;
  argv->vector = realloc(argv->vector, size * sizeof(void*));
  argv->vector[argv->size - 1] = strdup(str);
}

void u_argv_push_default_if_set_bool(u_argv_t * argv, char * const arg, int dflt, int var){
  if(var != INI_UNSET_BOOL){
    if((int) var)
      u_argv_push(argv, arg);
  }else if(dflt != INI_UNSET_BOOL){
    if((int) dflt)
      u_argv_push(argv, arg);
  }
}

static void push_api_args(u_argv_t * argv, char const * var){
  char * str = strdup(var);
  char * saveptr;
  char * t = strtok_r(str, " ", & saveptr);
  u_argv_push(argv, str); // this is the API
  if(t){
    int len = strlen(str) + 3;
    char buff[len + 1];
    sprintf(buff, "--%s.", str);
    while(true){
      t = strtok_r(NULL, " ", & saveptr);
      if(! t){
        break;
      }
      if(strncasecmp(buff, t, len) == 0){
        u_argv_push(argv, t);
      }else{
        FATAL("Provided API option %s appears to be no API supported version\n", t);
      }
    }
  }
  free(str);
}

void u_argv_push_default_if_set_api_options(u_argv_t * argv, char * const arg, char const * dflt, char const * var){
  if(var != INI_UNSET_STRING){
    u_argv_push(argv, arg);
    push_api_args(argv, var);
  }else if(dflt != INI_UNSET_STRING){
    u_argv_push(argv, arg);
    push_api_args(argv, dflt);
  }else{
    // add generic args
    u_argv_push(argv, arg);
    if(opt.apiArgs){
      push_api_args(argv, opt.apiArgs);
    }else{
      u_argv_push(argv, opt.api);
    }
  }
}

void u_argv_push_default_if_set(u_argv_t * argv, char * const arg, char const * dflt, char const * var){
  if(var != INI_UNSET_STRING){
    u_argv_push(argv, arg);
    u_argv_push(argv, var);
  }else if(dflt != INI_UNSET_STRING){
    u_argv_push(argv, arg);
    u_argv_push(argv, dflt);
  }
}


void u_argv_push_printf(u_argv_t * argv, char const * format, ...){
  char buff[PATH_MAX];
  va_list args;
  va_start(args, format);
  vsprintf(buff, format, args);
  va_end (args);
  u_argv_push(argv, buff);
}

char * u_flatten_argv(u_argv_t * argv){
  char command[PATH_MAX];
  char * p = command;
  *p = '\0';
  for(int i = 0; i < argv->size; i++){
    if(i != 0) p+= sprintf(p, " ");
    p += sprintf(p, "%s", argv->vector[i]);
  }
  return strdup(command);
}

void * u_malloc(int size){
  char * buff = malloc(size);
  if(! buff){
    FATAL("Cannot malloc();")
  }
  return buff;
}

void u_print_timestamp(FILE * out){
  char buffer[30];
  struct tm* tm_info;
  time_t timer;

  time(&timer);
  tm_info = localtime(&timer);
  strftime(buffer, 30, "%Y-%m-%d %H:%M:%S", tm_info);
  fprintf(out, "%s", buffer);
}

FILE * u_res_file_prep(char const * name){
  FILE * out = stdout;
  if(opt.rank == 0){
    char fname[PATH_MAX];
    sprintf(fname, "%s/%s.txt", opt.resdir, name);
    INFO_PAIR("result-file", "%s\n", fname);
    out = fopen(fname, "w");
    if( out == NULL ){
      FATAL("Couldn't create results file: %s (Error: %s)\n", fname, strerror(errno));
    }
  }
  return out;
}

void u_res_file_close(FILE * out){
  if(opt.rank == 0){
    fclose(out);
    out_logfile = stdout;
  }
}

typedef struct{
  uint32_t score_hash;

  char const * cfg_hash_read;
  char const * score_hash_read;
} res_file_data_t ;

static res_file_data_t res_data;

static void hash_func(bool is_section, char const * key, char const * val){
  static char const * last_section = NULL;
  if(is_section){
    last_section = key;
    return;
  }
  if(strcmp(key, "version") == 0){
    printf("result file ver = %s\n", val);
    if(strcmp(val, VERSION) != 0){
      WARNING("Verify the output with the matching version of the benchmark.\n");
    }
    u_hash_update_key_val(& res_data.score_hash, key, val);
    return;
  }
  if(strcmp(key, "config-hash") == 0){
    res_data.cfg_hash_read = strdup(val);
    return;
  }

  if(! last_section) return;

  if(strcmp(last_section, "SCORE") == 0){
    if(strcmp(key, "SCORE") == 0){
      // might be followed with the info INVALID
      char * strippedScore = strtok((char*)val, " ");
      u_hash_update_key_val(& res_data.score_hash, key, strippedScore);
    }else if(strcmp(key, "hash") == 0){
      res_data.score_hash_read = strdup(val);
    }else{
      u_hash_update_key_val(& res_data.score_hash, key, val);
    }
    return;
  }
  if(strcmp(key, "score") == 0){
    u_hash_update_key_val(& res_data.score_hash, last_section, val);
    return;
  }
  DEBUG_INFO("ignored: [%s] %s %s\n", last_section, key, val);
}

void u_verify_result_files(ini_section_t ** cfg, char const * result){
  uint32_t hash = 0;
  int error = 0;

  hash = u_ini_gen_hash(cfg);
  u_ini_parse_file(result, NULL, hash_func, NULL);

  printf("[run]\n");
  printf("config-hash     = %s\n", res_data.cfg_hash_read);
  printf("score-hash      = %s\n", res_data.score_hash_read);

  if(res_data.cfg_hash_read == NULL){
    FATAL("ERROR: Empty config hash read\n");
  }

  char hash_str[30];
  sprintf(hash_str, "%X", hash);
  if(strcmp(hash_str, res_data.cfg_hash_read) != 0){
    printf("ERROR: Configuration hash expected: %s read: %s\n", hash_str, res_data.cfg_hash_read);
    error = 1;
  }

  sprintf(hash_str, "%X", res_data.score_hash);
  // check if this is a valid run
  if(strcmp(hash_str, res_data.score_hash_read) != 0){
    char const * shash = strdup(hash_str);
    u_hash_update_key_val(& res_data.score_hash, "valid", "NO");
    sprintf(hash_str, "%X", res_data.score_hash);
    error = 1;
    if(strcmp(hash_str, res_data.score_hash_read) != 0){
      printf("\nERROR: Score hash expected: \"%s\" read: \"%s\"\n", shash, res_data.score_hash_read);
    }else{
      printf("\n[OK] But this is an invalid run!\n");
    }
  }

  if(error == 0){
    printf("\n[OK]\n");
  }

  exit(error);
}

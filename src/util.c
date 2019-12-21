#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <assert.h>


#include <io500-util.h>
#include <io500-opt.h>

void u_create_datadir(char const * dir){
  if(opt.rank != 0){
    return;
  }
  char d[2048];
  sprintf(d, "%s/%s", opt.datadir, dir);
  char outdir[2048];
  char * wp = outdir;

  char * next = strtok(d, "/");
  while(next){
    if(*next == '/' || *next == 0) continue;
    wp += sprintf(wp, "%s/", next);

    struct stat sb;
    int ret = stat(outdir, & sb);
    if(ret != 0){
      DEBUG_INFO("Creating dir %s\n", outdir);
      ret = mkdir(outdir, S_IRWXU);
      if(ret != 0){
        FATAL("Couldn't create directory %s (Error: %s)\n", outdir, strerror(errno));
      }
    }
    next = strtok(NULL, "/");
  }
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
  char buff[2048];
  va_list args;
  va_start(args, format);
  vsprintf(buff, format, args);
  va_end (args);
  u_argv_push(argv, buff);
}

char * u_flatten_argv(u_argv_t * argv){
  char command[2048];
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

void u_print_timestamp(void){
  char buffer[30];
  struct tm* tm_info;
  time_t timer;

  time(&timer);
  tm_info = localtime(&timer);
  strftime(buffer, 30, "%Y-%m-%d %H:%M:%S", tm_info);
  printf("%s", buffer);
}

FILE * u_res_file_prep(char const * name){
  FILE * out = stdout;
  if(opt.rank == 0){
    char fname[2048];
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
  }
}

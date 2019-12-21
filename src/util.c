#include <stdlib.h>

#include <io500-util.h>

void * u_malloc(int size){
  char * buff = malloc(size);
  if(! buff){
    FATAL("Cannot malloc();")
  }
  return buff;
}

double u_time_diff(clock_t start){
  clock_t t_end = clock();
  return ((double) (t_end - start)) / CLOCKS_PER_SEC;
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

#ifndef IO500_UTIL_H
#define IO500_UTIL_H

#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <time.h>

#include <io500-debug.h>

typedef enum{
  INI_STRING,
  INI_INT,
  INI_UINT,
  INI_BOOL
} ini_var_type_e;

typedef struct {
  char const * name; // the flag, e.g., testArg
  char const * help; // help text provided to understand what this flag does

  bool mandatory;
  ini_var_type_e type; // for checking during the parsing

  char * default_val; // the default value, NULL if no value set
  void * var;         // the pointer to the variable to fill of the given type
} ini_option_t;

typedef struct {
  char const * name; // the section name
  ini_option_t * option;
} ini_section_t;

/**
 Parse the ini file in data according to the expected specification
 @Return 0 if parsing is successfull
 */
int u_parse_ini(char const * data, ini_section_t ** specification);

/**
 Compute a hash based on the current values
 */
uint32_t u_ini_gen_hash(ini_section_t ** sections);

void u_ini_print_hash(FILE * file, ini_section_t ** sections);
void u_ini_print_values(ini_section_t ** sections);

double u_time_diff(clock_t start);
void u_print_timestamp(void);

void * u_malloc(int size);

#endif

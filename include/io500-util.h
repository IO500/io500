#ifndef IO500_UTIL_H
#define IO500_UTIL_H

#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>

#include <io500-debug.h>

typedef enum{
  INI_STRING,
  INI_INT,
  INI_UINT
} ini_var_type_e;

typedef struct {
  char const * name; // the flag, e.g., testArg
  char const * help; // help text provided to understand what this flag does

  bool mandatory;
  ini_var_type_e type; // for checking during the parsing

  char * current_val; // the current value (default) or value after parsing, NULL if no value set
} ini_option_t;

typedef struct {
  char const * name; // the section name
  ini_option_t * option;
} ini_section_t;

/**
 Parse the ini file in data according to the expected specification
 @Return 0 if parsing is successfull
 */
int u_parse_ini(char const * data, ini_section_t * specification);

/**
 Compute a hash based on the current values
 */
uint32_t u_ini_gen_hash(ini_section_t * sections);

void u_ini_print_hash(FILE * file, ini_section_t * sections);
#endif

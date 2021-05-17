#ifndef IO500_UTIL_H
#define IO500_UTIL_H

#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <time.h>

#include <io500-debug.h>

#define INI_UNSET_STRING NULL
#define INI_UNSET_INT (-2147483648)
#define INI_UNSET_UINT (unsigned)(-1)
#define INI_UNSET_UINT64 (uint64_t)(-1)
#define INI_UNSET_BOOL 2
#define INI_UNSET_FLOAT 1e38

typedef enum{
  INI_STRING,
  INI_INT,
  INI_UINT,
  INI_UINT64,
  INI_BOOL,
  INI_FLOAT
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

typedef void(*ini_call_back_f)(bool is_section, char const * key, char const * val);

/**
 Parse the ini file in data according to the expected specification
 @Return 0 if parsing is successfull
 */
int u_parse_ini(char const * data, ini_section_t ** specification, ini_call_back_f func);


void u_ini_parse_file(char const * file, ini_section_t** out_cfg, ini_call_back_f func, char ** out_buffer);

/**
 Compute a hash based on the current values
 */
uint32_t u_ini_gen_hash(ini_section_t ** sections);
void u_ini_print_values(FILE * fd, ini_section_t ** sections, bool show_help);

/**
 * Hash functions to increase integrity
 */
uint32_t u_hash_update(uint32_t hash, char const * str);
void u_hash_update_key_val(uint32_t * hash, char const * key, char const * val);
void u_hash_update_key_val_dbl(uint32_t * hash, char const * key, double val);
void u_hash_print(FILE * file, uint32_t hash);
void u_verify_result_files(ini_section_t ** cfg, char const * result_file);

// imported from IOR
double GetTimeStamp(void);
void u_create_datadir(char const * dir);
void u_purge_datadir(char const * dir);
void u_purge_file(char const * file);

void u_create_dir_recursive(char const * dir, ior_aiori_t const * api, aiori_mod_opt_t * module_options);

// invoke an external shell command
void u_call_cmd(char const * command);

void u_print_timestamp(FILE * out);
void * u_malloc(int size);


FILE * u_res_file_prep(char const * name);
void u_res_file_close(FILE * out);

uint32_t u_phase_unique_random_number(char const * phase_name);

/**
 * Functions to handle the argument vectors for invoking other APIs
 */
typedef struct{
 int size;
 char ** vector;
} u_argv_t;

u_argv_t * u_argv_create(void);
void u_argv_free(u_argv_t * argv);
void u_argv_push(u_argv_t * argv, char const * str);

void u_argv_push_default_if_set(u_argv_t * argv, char * const arg, char const * dflt, char const * var);
/* if the first argument is the API, subsequent options provided with --api.option will be permitted*/
void u_argv_push_default_if_set_api_options(u_argv_t * argv, char * const arg, char const * dflt, char const * var);
void u_argv_push_default_if_set_bool(u_argv_t * argv, char * const arg, int dflt, int var);

void u_argv_push_printf(u_argv_t * argv, char const * format, ...)
  __attribute__ ((format (printf, 2, 3)));
char * u_flatten_argv(u_argv_t * argv);

#endif

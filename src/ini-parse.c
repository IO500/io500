#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <string.h>

#include <io500-util.h>

uint32_t u_ini_gen_hash(ini_section_t ** sections){
  uint32_t value = 0;
  for( ini_section_t ** ps = sections ; *ps != NULL; ps++){
    ini_section_t * s = *ps;
    for( ini_option_t * o = s->option ; o->name != NULL; o++){
      if(o->default_val){
        // compute a hash for each option individually to make it invariant to reordered options
        u_hash_update_key_val(& value, o->name, o->default_val);
      }
    }
  }
  return value;
}

int u_parse_ini(char const * data, ini_section_t ** sections){

  int reti;

  // prepare regexes
  regex_t r_section, r_int, r_uint, r_str, r_empty;
  reti = regcomp(& r_section, "^[[:space:]]*\\[[[:space:]]*([0-9a-zA-Z_-]+)[[:space:]]*\\][[:space:]]*([[:space:]][#;].*)?$", REG_EXTENDED);
  if (reti){
    FATAL("Could not compile regex\n");
  }
  reti = regcomp(& r_str, "^[[:space:]]*([0-9a-zA-Z_.-]+)[[:space:]]*=[[:space:]]*([^#;]+)[[:space:]]*([[:space:]][#;].*)?$", REG_EXTENDED);
  if (reti){
    FATAL("Could not compile regex\n");
  }
  reti = regcomp(& r_int, "^[-]?[0-9]+$", REG_EXTENDED);
  if (reti){
    FATAL("Could not compile regex\n");
  }
  reti = regcomp(& r_uint, "^[0-9]+$", REG_EXTENDED);
  if (reti){
    FATAL("Could not compile regex\n");
  }
  reti = regcomp(& r_empty, "^[[:space:]]*([#;].*)?$", REG_EXTENDED);
  if (reti){
    FATAL("Could not compile regex\n");
  }

  char * copy = strdup(data);
  char * token;
  char * saveptr;
  token = strtok_r(copy, "\n", &saveptr);

  ini_section_t * section = NULL;
  int line = 0;

  char * lastsaveptr = saveptr;

  // parse each line
  while(token){
    line += 1 + ((token - lastsaveptr) > 0 ? (token - lastsaveptr) : 0); // strtok skips whole lines
    lastsaveptr = saveptr;

    regmatch_t match[3];
    DEBUG_INFO("Parsing: \"%s\"\n", token);

    reti = regexec(&r_section, token, 2, match, 0);
    if (reti == 0) {
      char * sname = token + match[1].rm_so;
      token[match[1].rm_eo] = '\0';
      DEBUG_INFO("Section: \"%s\"\n", sname);
      section = NULL;
      for( ini_section_t ** ps = sections ; *ps != NULL; ps++){
        if(strcasecmp((*ps)->name, sname) == 0){
          section = *ps;
          break;
        }
      }
      if(section == NULL){
        ERROR("Parsing error in line %d, unknown section %s\n", line, sname);
        return 1;
      }
      token = strtok_r(NULL, "\n", &saveptr);
      continue;
    }

    reti = regexec(&r_str, token, 3, match, 0);
    if (reti == 0) {
      char * var = token + match[1].rm_so;
      char * val = token + match[2].rm_so;
      token[match[1].rm_eo] = '\0';

      // trim whitespace from the right of the string value
      for(int p = match[2].rm_eo; p >= match[2].rm_so; p--){
        switch(token[p]){
          case ' ':
          case '\t':
          case '\r':
          token[p] = '\0';
          break;
          default:
            goto end_loop;
        }
      }
      end_loop:

      DEBUG_INFO("Var: \"%s\"=\"%s\"\n", var, val);

      if(section == NULL){
        ERROR("Parsing error in line %d, variable assigned outside section\n", line);
        return 1;
      }

      ini_option_t * option = section->option;
      for( ; option->name != NULL; option++){
        if(strcasecmp(option->name, var) == 0){
          break;
        }
      }

      if(option->name == NULL){
        ERROR("Parsing error in section %s line %d, unknown option \"%s\" with value \"%s\"\n", section->name, line, var, val);
        return 1;
      }

      if(option->type == INI_INT){
        reti = regexec(& r_int, val, 0, NULL, 0);
        if(reti != 0){
          ERROR("Parsing error in section %s line %d, option %s expects integer, received \"%s\"\n", section->name, line, var, val);
          return 1;
        }
      }else if(option->type == INI_UINT || option->type == INI_UINT64){
        reti = regexec(& r_uint, val, 0, NULL, 0);
        if(reti != 0){
          ERROR("Parsing error in section %s line %d, option %s expects integer >= 0, received \"%s\"\n", section->name, line, var, val);
          return 1;
        }
      }else if(option->type == INI_BOOL){
        if(strcasecmp(val, "true") == 0 || strcmp(val, "1") == 0){
          val = "TRUE";
        }else if(strcasecmp(val, "false") == 0 || strcmp(val, "0") == 0){
          val = "FALSE";
        }else{
          ERROR("Parsing error in section %s line %d, option %s expects bool (true|false), received \"%s\"\n", section->name, line, var, val);
          return 1;
        }
      }
      // assign new value
      option->default_val = strdup(val);

      token = strtok_r(NULL, "\n", &saveptr);
      continue;
    }

    // must be the empty line
    reti = regexec(&r_empty, token, 0, NULL, 0);
    if (reti != 0) {
        ERROR("Parsing error in section %s line %d, unexpected content: \"%s\"\n", section->name, line, token);
        return 1;
    }

    token = strtok_r(NULL, "\n", &saveptr);
  }

  // check for mandatory options and assign values
  int error = 0;
  for( ini_section_t ** ps = sections ; *ps != NULL; ps++){
    ini_section_t * s = *ps;

    for( ini_option_t * o = s->option ; o->name != NULL; o++){
      if( o->mandatory && o->default_val == NULL ){
          ERROR("[%s]: The mandatory option \"%s\" is not set\n", s->name, o->name);
          error = 1;
      }
      if(! o->var) continue;
      // assing a value to the configuration
      if(o->default_val){
        switch(o->type){
        case(INI_INT):{
          *(int*) o->var = atoi(o->default_val);
          break;
        }case(INI_UINT):{
          *(unsigned*) o->var = atoi(o->default_val);
          break;
        }case(INI_UINT64):{
          *(uint64_t*) o->var = (uint64_t) atoll(o->default_val);
          break;
        }case(INI_BOOL):{
          *(int*) o->var = o->default_val[0] == 'T';
          break;
        }case(INI_STRING):{
          *(char**) o->var = o->default_val;
          break;
        }
        }
      }else{
        switch(o->type){
        case(INI_INT):{
          *(int*) o->var = INI_UNSET_INT;
          break;
        }case(INI_UINT):{
          *(unsigned*) o->var = INI_UNSET_UINT;
          break;
        }case(INI_UINT64):{
          *(uint64_t*) o->var = INI_UNSET_UINT64;
          break;
        }case(INI_BOOL):{
          *(int*) o->var = INI_UNSET_BOOL;
          break;
        }case(INI_STRING):{
          *(char**) o->var = INI_UNSET_STRING;
          break;
        }
        }
      }
    }
  }
  // cleanup
  regfree(&r_section);
  regfree(&r_int);
  regfree(&r_uint);
  regfree(&r_str);
  free(copy);
  return error;
}

void u_ini_print_values(ini_section_t ** sections){
  for( ini_section_t ** ps = sections ; *ps != NULL; ps++){
    ini_section_t * s = *ps;
    printf("[%s]\n", s->name);

    for( ini_option_t * o = s->option ; o->name != NULL; o++){
      if (o->help){
        printf("# %s\n", o->help);
      }
      printf("%s = %s\n", o->name, o->default_val ? o->default_val : "");
    }
    printf("\n");
  }
}

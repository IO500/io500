#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <string.h>

#include <io500-util.h>

/**
 * rotate the value and add the current character
 */
static uint32_t rotladd (uint32_t x, char c){
  return ((x<<2) | (x>>(32-2))) + c;
}

void u_ini_print_hash(FILE * file, ini_section_t * sections){
  uint32_t hash = u_ini_gen_hash(sections);
  fprintf(file, "%X", (int) hash);
}

uint32_t u_ini_gen_hash(ini_section_t * sections){
  uint32_t value = 0;
  for( ini_section_t * s = sections ; s->name != NULL; s++){
    for( ini_option_t * o = s->option ; o->name != NULL; o++){
      if(o->current_val){
        // compute a hash for each option individually to make it invariant to reordered options
        uint32_t val = 0;
        for(char const * c = o->name; *c != 0; c++){
          val = rotladd(val, *c);
        }
        for(char * c = o->current_val; *c != 0; c++){
          val = rotladd(val, *c);
        }
        value = value ^ val;
      }
    }
  }
  return value;
}

int u_parse_ini(char const * data, ini_section_t * specification){

  int reti;

  // prepare regexes
  regex_t r_section, r_int, r_uint, r_str;
  reti = regcomp(& r_section, "^[[:space:]]*\\[[[:space:]]*([0-9a-zA-Z_]+)[[:space:]]*\\][[:space:]]*([[:space:]][#;].*)?$", REG_EXTENDED);
  if (reti){
    FATAL("Could not compile regex\n");
  }
  reti = regcomp(& r_str, "^[[:space:]]*([0-9a-zA-Z_]+)[[:space:]]*=[[:space:]]*([-0-9a-zA-Z_ ]+)[[:space:]]*([[:space:]][#;].*)?$", REG_EXTENDED);
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

  char * copy = strdup(data);
  char * token;
  char * saveptr;
  token = strtok_r(copy, "\n", &saveptr);

  ini_section_t * section = NULL;
  int line = 0;

  // parse each line
  while(token){
    line++;
    regmatch_t match[3];
    DEBUG_INFO("Parsing: \"%s\"\n", token);

    reti = regexec(&r_section, token, 2, match, 0);
    if (reti == 0) {
      char * sname = token + match[1].rm_so;
      token[match[1].rm_eo] = '\0';
      DEBUG_INFO("Section: \"%s\"\n", sname);
      for( section = specification ; section->name != NULL; section++){
        if(strcasecmp(section->name, sname) == 0){
          break;
        }
      }
      if(section->name == NULL){
        ERROR("Parsing error in line %d, unknown section %s\n", line, sname);
        return 1;
      }
    }

    reti = regexec(&r_str, token, 3, match, 0);
    if (reti == 0) {
      char * var = token + match[1].rm_so;
      char * val = token + match[2].rm_so;
      token[match[1].rm_eo] = '\0';
      token[match[2].rm_eo] = '\0';

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
        ERROR("Parsing error in line %d, unknown option %s\n", line, var);
        return 1;
      }

      if(option->type == INI_INT){
        reti = regexec(& r_int, val, 0, NULL, 0);
        if(reti != 0){
          ERROR("Parsing error in line %d, option %s expects integer, received \"%s\"\n", line, var, val);
          return 1;
        }
      }else if(option->type == INI_UINT){
        reti = regexec(& r_uint, val, 0, NULL, 0);
        if(reti != 0){
          ERROR("Parsing error in line %d, option %s expects integer >= 0, received \"%s\"\n", line, var, val);
          return 1;
        }
      }
      // assign new value
      option->current_val = strdup(val);
    }

    token = strtok_r(NULL, "\n", &saveptr);
  }

  // check for mandatory options
  int error = 0;
  for( ini_section_t * s = specification ; s->name != NULL; s++){
    for( ini_option_t * o = s->option ; o->name != NULL; o++){
      if( o->mandatory && o->current_val == NULL ){
          ERROR("[%s]: The mandatory option \"%s\" is not set\n", s->name, o->name);
          error = 1;
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

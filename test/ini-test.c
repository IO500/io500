#include <stdio.h>
#include <assert.h>

#include <io500-util.h>

FILE* file_out;
FILE* out_logfile;

int main(void){
  file_out = stdout;

  int ret;
  {
    ini_section_t * testsec[] = {NULL};
    ret = u_parse_ini("[general] \ntest=24", testsec, NULL);
    assert(ret != 0);
  }

  {
    ini_option_t option[] = {
      {"--test", "test help", 1, INI_INT, NULL},
      {NULL} };
    ini_section_t * testsec[] = {
        & (ini_section_t) { .name = "general", option},
        NULL
      };
    ret = u_parse_ini(" \t[general ] \n test \t= 24 ; ignore me\ndata=test string\ntest3 =-333", testsec, NULL);
    assert(ret == 1);
  }

  {
    ini_option_t option[] = {
      {"--test", "test help", 1, INI_INT, NULL},
      {"test", "test help2", 1, INI_INT, "123"},
      {NULL} };
    ini_section_t * testsec[] = {
        & (ini_section_t) { .name = "general", option},
        NULL
      };
    ret = u_parse_ini(" \t[general ] \n test \t= 24 ; ignore me\ndata=test string\ntest3 =-333", testsec, NULL);
    assert(ret == 1);
  }

  {
    ini_option_t option[] = {
      {"--test", "test help", 1, INI_INT, NULL},
      {"test", "test help2", 1, INI_INT, "123"},
      {"data", "test help2", 1, INI_STRING, "test"},
      {"test3", "test help2", 1, INI_UINT, "test"},
      {NULL} };
    ini_section_t * testsec[] = {
        & (ini_section_t) { .name = "general", option},
        NULL
      };
    ret = u_parse_ini(" \t[general ] \n test \t= 24 ; ignore me\ndata=test string\ntest3 =-333", testsec, NULL);
    assert(ret == 1);
    uint32_t hash = u_ini_gen_hash(testsec);
    u_hash_print(stdout, hash);
    printf("\n");
  }

  {
    ini_option_t option[] = {
      {"--test", "test help", 1, INI_INT, NULL},
      {"test", "test help2", 1, INI_INT, "123"},
      {"data", "test help2", 1, INI_STRING, "test"},
      {"test3", "test help2", 1, INI_INT, "test"},
      {NULL} };
    ini_section_t * testsec[] = {
        & (ini_section_t) { .name = "general", option},
        NULL
      };
    ret = u_parse_ini(" \t[general ] \n test \t= 24 ; ignore me\ndata=test string\ntest3 =-333", testsec, NULL);
    assert(ret == 1);
    uint32_t hash = u_ini_gen_hash(testsec);
    u_hash_print(stdout, hash);
    printf("\n");
  }

  {
    ini_option_t option[] = {
      {"test", "test help2", 1, INI_INT, "123"},
      {"data", "test help2", 1, INI_STRING, "test"},
      {"test3", "test help2", 1, INI_INT, "test"},
      {NULL} };
    ini_section_t * testsec[] = {
        & (ini_section_t) { .name = "general", option},
        NULL
      };
    ret = u_parse_ini(" \t[general ] \n test \t= 24 ; ignore me\ndata=test string\ntest3 =-333", testsec, NULL);
    uint32_t hash = u_ini_gen_hash(testsec);
    u_hash_print(stdout, hash);
    printf("\n");
    assert(ret == 0);
  }

  printf("\nOK\n");

  return 0;
}

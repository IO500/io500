#include <stdio.h>
#include <assert.h>

#include <io500-util.h>

int main(void){
  int ret;
  {
    ini_section_t * testsec[] = {NULL};
    ret = u_parse_ini("[general] \ntest=24", testsec);
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
    ret = u_parse_ini(" \t[general ] \n test \t= 24 ; ignore me\ndata=test string\ntest3 =-333", testsec);
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
    ret = u_parse_ini(" \t[general ] \n test \t= 24 ; ignore me\ndata=test string\ntest3 =-333", testsec);
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
    ret = u_parse_ini(" \t[general ] \n test \t= 24 ; ignore me\ndata=test string\ntest3 =-333", testsec);
    assert(ret == 1);
    u_ini_print_hash(stdout, testsec);
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
    ret = u_parse_ini(" \t[general ] \n test \t= 24 ; ignore me\ndata=test string\ntest3 =-333", testsec);
    assert(ret == 1);
    u_ini_print_hash(stdout, testsec);
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
    ret = u_parse_ini(" \t[general ] \n test \t= 24 ; ignore me\ndata=test string\ntest3 =-333", testsec);
    u_ini_print_hash(stdout, testsec);
    printf("\n");
    assert(ret == 0);
  }

  printf("\nOK\n");

  return 0;
}

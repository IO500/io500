static u_phase_t * phases[IO500_PHASES] = {
  & p_opt,
  & p_debug,

  & p_ior_easy,
  & p_ior_easy_write,

  & p_ior_rnd4K,
  & p_ior_rnd4K_write,

  & p_mdtest_easy,
  & p_mdtest_easy_write,
  
  & p_ior_rnd1MB,
  & p_ior_rnd1MB_write,

  & p_mdworkbench,
  & p_mdworkbench_create,

  & p_timestamp,

  & p_find_easy,

  & p_ior_hard,
  & p_ior_hard_write,

  & p_mdtest_hard,
  & p_mdtest_hard_write,

  & p_find,

  & p_ior_rnd4K_read,
  & p_ior_rnd1MB_read,

  & p_find_hard,

  & p_mdworkbench_bench,

  & p_ior_easy_read,
  & p_mdtest_easy_stat,

  & p_ior_hard_read,
  & p_mdtest_hard_stat,

  & p_mdworkbench_delete,

  & p_mdtest_easy_delete,
  & p_mdtest_hard_read,
  & p_mdtest_hard_delete
};

static ini_section_t ** u_options(void){
  ini_section_t ** ini_section = u_malloc(sizeof(ini_section_t*) * (IO500_PHASES + 1));
  for(int i=0; i < IO500_PHASES; i++){
    ini_section[i] = u_malloc(sizeof(ini_section_t));
    ini_section[i]->name = phases[i]->name;
    ini_section[i]->option = phases[i]->options;
  }
  ini_section[IO500_PHASES] = NULL;
  return ini_section;
}

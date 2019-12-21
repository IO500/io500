#ifndef IO500_DEBUG_H
#define IO500_DEBUG_H

#include <stdlib.h>

#include <io500-opt.h>

#define FATAL(...) do{ printf("FATAL ("__FILE__":%d) ", __LINE__); printf(__VA_ARGS__); exit(1); }while(0);

#define DEBUG_ALL(...) do{ if(opt.verbosity > 5 && opt.rank == 0){ printf("; [D] "__VA_ARGS__);}  }while(0);
#define DEBUG_INFO(...) do{ if(opt.verbosity > 4 && opt.rank == 0){ printf("; [I] "__VA_ARGS__);}  }while(0);

#define PRINT_PAIR(key, format, ...) do{ if(opt.rank == 0){ printf("%-15s = "format, key, __VA_ARGS__); }  }while(0);
#define PRINT_PAIR_HEADER(key) do{ if(opt.rank == 0){ printf("%-15s = ", key); }  }while(0);
#define INFO_PAIR(key, format, ...) do{ if(opt.verbosity > 1){ PRINT_PAIR(key, format, __VA_ARGS__);}  }while(0);

#define ERROR(...) do{ printf("; ERROR "__VA_ARGS__); }while(0);
#define WARNING(...) do{ printf("; WARNING "__VA_ARGS__); }while(0);

#define r0printf(...) do{ if(opt.rank == 0){ printf(__VA_ARGS__);}  }while(0);
#endif

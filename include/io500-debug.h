#ifndef IO500_DEBUG_H
#define IO500_DEBUG_H

#include <stdlib.h>

#include <io500-opt.h>

extern FILE* file_out;

#define FATAL(...) do{ printf("FATAL ("__FILE__":%d) ", __LINE__); printf(__VA_ARGS__); exit(1); }while(0);

#define DEBUG_ALL(...) do{ if(opt.verbosity > 5 && opt.rank == 0){ printf("; [D] "__VA_ARGS__);}  }while(0);
#define DEBUG_INFO(...) do{ if(opt.verbosity > 4 && opt.rank == 0){ printf("; [I] "__VA_ARGS__);}  }while(0);

#define PRINT_PAIR(key, format, ...) do{ if(opt.rank == 0){ fprintf(file_out, "%-15s = "format, key, __VA_ARGS__); fflush(file_out); }  }while(0);
#define PRINT_PAIR_HEADER(key) do{ if(opt.rank == 0){ fprintf(file_out, "%-15s = ", key); }  }while(0);
#define INFO_PAIR(key, format, ...) do{ if(opt.verbosity > 1){ PRINT_PAIR(key, format, __VA_ARGS__);}  }while(0);

#define ERROR(...) do{ fprintf(file_out, "; ERROR "__VA_ARGS__); printf("ERROR "__VA_ARGS__); fflush(file_out); }while(0);
#define WARNING(...) do{ fprintf(file_out, "; WARNING "__VA_ARGS__); printf("WARNING "__VA_ARGS__); fflush(file_out); }while(0);

#define INVALID(...) do{ if (opt.rank == 0){fprintf(file_out, "; ERROR INVALID "__VA_ARGS__); printf("ERROR INVALID (%s:%d) ", __FILE__, __LINE__); printf(__VA_ARGS__); fflush(file_out); opt.is_valid_phase = 0; } }while(0);


#define r0printf(...) do{ if(opt.rank == 0){ printf(__VA_ARGS__); fflush(stdout); }  }while(0);


#define UMPI_CHECK(MPI_STATUS) do {                                    \
  char resultString[MPI_MAX_ERROR_STRING];                             \
  int resultLength;                                                    \
                                                                       \
  if (MPI_STATUS != MPI_SUCCESS) {                                     \
      MPI_Error_string(MPI_STATUS, resultString, &resultLength);       \
      FATAL("MPI ERROR: MPI %s, (%s:%d)\n",                            \
             resultString, __FILE__, __LINE__);                        \
  }                                                                    \
} while(0)


#endif

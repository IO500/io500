#ifndef IO500_DEBUG_H
#define IO500_DEBUG_H

extern int io500_debug;

#define FATAL(...) do{ printf("FATAL ("__FILE__":%d) ", __LINE__); printf(__VA_ARGS__); exit(1); }while(0);
#define DEBUG_INFO(...) do{ if(io500_debug > 3){ printf("I "__VA_ARGS__);}  }while(0);

#define ERROR(...) do{ printf("ERROR "__VA_ARGS__); }while(0);

#endif

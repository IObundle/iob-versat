#ifndef INCLUDED_VERSAT_COMMON
#define INCLUDED_VERSAT_COMMON

struct SimpleFUInstance{
   const char* name;
   int* memMapped;
   int* config;
   int* state;
   int* delay;
};

#endif // INCLUDED_VERSAT_COMMON
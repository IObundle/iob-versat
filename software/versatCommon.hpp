#ifndef INCLUDED_VERSAT_COMMON
#define INCLUDED_VERSAT_COMMON

struct FUInstance{
   const char* name;
   volatile int* memMapped;
   volatile int* config;
   volatile int* state;
   volatile int* delay;

   int numberChilds;
};

#endif // INCLUDED_VERSAT_COMMON

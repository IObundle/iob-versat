#ifndef INCLUDED_DEBUG_VERSAT
#define INCLUDED_DEBUG_VERSAT

#include <cstdio>

#include "debug.hpp"
#include "utils.hpp"

struct Versat;
struct Accelerator;
struct Arena;

// Useful to have a bit of memory for debugging
extern Arena* debugArena;

String FuzzText(String formattedExample,Arena* arena,int seed = COMPILE_TIME); // Pass a string of valid tokens separated by spaces

//bool IsGraphValid(AcceleratorView view);
void OutputGraphDotFile(Versat* versat,Accelerator* accel,bool collapseSameEdges,const char* filenameFormat,...);

String PushMemoryHex(Arena* arena,void* memory,int size);
void OutputMemoryHex(void* memory,int size);

#endif // INCLUDED_DEBUG_VERSAT
 

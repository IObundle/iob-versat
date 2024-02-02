#pragma once

#include <cstdio>

#include "debug.hpp"
#include "utils.hpp"

struct Versat;
struct Accelerator;
struct Arena;
struct FUInstance;

// Useful to have a bit of memory for debugging
extern Arena* debugArena;

String FuzzText(String formattedExample,Arena* out,int seed = COMPILE_TIME); // Pass a string of valid tokens separated by spaces

void OutputGraphDotFile(Versat* versat,Accelerator* accel,bool collapseSameEdges,String filename,Arena* temp);
void OutputGraphDotFile(Versat* versat,Accelerator* accel,bool collapseSameEdges,FUInstance* highlighInstance,String filename,Arena* temp);
void OutputGraphDotFile(Versat* versat,Accelerator* accel,bool collapseSameEdges,Set<FUInstance*>* highlight,String filename,Arena* temp);

String PushMemoryHex(Arena* out,void* memory,int size);
void OutputMemoryHex(void* memory,int size);
 

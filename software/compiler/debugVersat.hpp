#pragma once

#include <cstdio>

#include "debug.hpp"
#include "utils.hpp"

#include "delayCalculation.hpp"
#include "versat.hpp"

struct Accelerator;
struct Arena;
struct FUInstance;

// Useful to have a bit of memory for debugging
extern Arena* debugArena;

String FuzzText(String formattedExample,Arena* out,int seed = COMPILE_TIME); // Pass a string of valid tokens separated by spaces

void OutputGraphDotFile(Accelerator* accel,bool collapseSameEdges,FUInstance* highlighInstance,CalculateDelayResult delays,String filename,Arena* temp);

#if 0
void OutputGraphDotFile(Accelerator* accel,bool collapseSameEdges,String filename,Arena* temp);
void OutputGraphDotFile(Accelerator* accel,bool collapseSameEdges,FUInstance* highlighInstance,String filename,Arena* temp);
void OutputGraphDotFile(Accelerator* accel,bool collapseSameEdges,Set<FUInstance*>* highlight,String filename,Arena* temp);
#endif

void OutputContentToFile(String filepath,String content);

String PushMemoryHex(Arena* out,void* memory,int size);
void OutputMemoryHex(void* memory,int size);

// folderName can be empty string, file is created inside debug folder directly. 
String PushDebugPath(Arena* out,String folderName,String fileName);

// For this overload, no string can be empty, otherwise error
String PushDebugPath(Arena* out,String folderName,String subFolder,String fileName);

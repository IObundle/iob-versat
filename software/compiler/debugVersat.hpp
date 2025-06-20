#pragma once

#include <cstdio>

#include "debug.hpp"
#include "utils.hpp"

#include "delayCalculation.hpp"

struct Accelerator;
struct Arena;
struct FUInstance;

// Useful to have a bit of memory for debugging
extern Arena* debugArena;

String FuzzText(String formattedExample,Arena* out,int seed = COMPILE_TIME); // Pass a string of valid tokens separated by spaces

void OutputGraphDotFile(Accelerator* accel,bool collapseSameEdges,FUInstance* highlighInstance,CalculateDelayResult delays,String filename);

void OutputContentToFile(String filepath,String content);

// folderName can be empty string, file is created inside debug folder directly. 
String PushDebugPath(Arena* out,String folderName,String fileName);

// For this overload, no string can be empty, otherwise error
String PushDebugPath(Arena* out,String folderName,String subFolder,String fileName);


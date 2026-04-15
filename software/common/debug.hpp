#pragma once

#include "utils.hpp"

struct Arena;

 // Call before any other debug function but after setting up general purpose arenas
void InitDebug(const char* exeName);

struct Location{
  String functionName;
  String fileName;
  u32 line;
};

extern Arena* debugArena;
extern bool debugFlag; // True if currently debugging
extern bool currentlyDebugging;

#define debugRegion() if(debugFlag)
#define debugRegionIf(COND) if(debugFlag && (COND))

typedef void (*SignalHandler)(int sig);

bool CurrentlyDebugging();
void SetDebugSignalHandler(SignalHandler func);

Array<Location> CollectStackTrace(Arena* out,int offset = 1);
void PrintStacktrace();

#pragma once

#include <cstdio>

struct Arena;

void InitDebug(); // Call before any other debug function

extern Arena* debugArena;
extern bool debugFlag; // True if currently debugging
extern bool currentlyDebugging;

#define debugRegion() if(debugFlag)
#define debugRegionIf(COND) if(debugFlag && (COND))

typedef void (*SignalHandler)(int sig);

bool CurrentlyDebugging();
void SetDebugSignalHandler(SignalHandler func);

void PrintStacktrace();

#pragma once

struct Arena;

 // Call before any other debug function but after setting up general purpose arenas
void InitDebug(const char* exeName);

extern Arena* debugArena;
extern bool debugFlag; // True if currently debugging
extern bool currentlyDebugging;

#define debugRegion() if(debugFlag)
#define debugRegionIf(COND) if(debugFlag && (COND))

typedef void (*SignalHandler)(int sig);

bool CurrentlyDebugging();
void SetDebugSignalHandler(SignalHandler func);

void PrintStacktrace();

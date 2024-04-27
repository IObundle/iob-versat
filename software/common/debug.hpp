#pragma once

#include <cstdio>

struct Arena;

void InitDebug(); // Call before any other debug function

extern Arena* debugArena;
extern bool debugFlag; // True if currently debugging
extern bool currentlyDebugging;

#define DEBUG_BREAK_IF_DEBUGGING() DEBUG_BREAK_IF(debugFlag)

#define DEBUG_CONTROLLED_BREAK() DEBUG_BREAK_IF(debugFlag)
#define DEBUG_CONTROLLED_BREAK_IF(COND) DEBUG_BREAK_IF(debugFlag && (COND))

#define debugControlledRegion() if(debugFlag)
#define debugControlledRegionIf(COND) if(debugFlag && (COND))

typedef void (*SignalHandler)(int sig);

bool CurrentlyDebugging();
void SetDebugSignalHandler(SignalHandler func);

void PrintStacktrace();

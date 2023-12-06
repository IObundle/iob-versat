#ifndef INCLUDED_DEBUG
#define INCLUDED_DEBUG

#include <cstdio>

void InitDebug(); // Call before any other debug function

// Set this during debugging (either in code or by using gdb set command) and check using the macros below for better control on debugging events
extern bool debugFlag;

#define DEBUG_CONTROLLED_BREAK() DEBUG_BREAK_IF(debugFlag)
#define DEBUG_CONTROLLED_BREAK_IF(COND) DEBUG_BREAK_IF(debugFlag && (COND))

#define debugControlledRegion() if(debugFlag)
#define debugControlledRegionIf(COND) if(debugFlag && (COND))

typedef void (*SignalHandler)(int sig);

void SetDebugSignalHandler(SignalHandler func);

void PrintStacktrace();

#endif // INCLUDED_DEBUG

#ifndef INCLUDED_DEBUG
#define INCLUDED_DEBUG

#include "type.hpp"

// Set this during debugging (either in code or by using gdb set command) and check using the macros below for better control on debugging events
extern bool debugVariableFlag;

// Useful to have a bit of memory for debugging
extern Arena* debugArena;

void InitDebug();

#define DEBUG_CONTROLLED_BREAK() DEBUG_BREAK_IF(debugVariableFlag)
#define DEBUG_CONTROLLED_BREAK_IF(COND) DEBUG_BREAK_IF(debugVariableFlag && (COND))

#define debugControlledRegion() if(debugVariableFlag)
#define debugControlledRegionIf(COND) if(debugVariableFlag && (COND))

typedef void (*SignalHandler)(int sig);

String FuzzText(String formattedExample,Arena* arena,int seed = COMPILE_TIME); // Pass a string of valid tokens separated by spaces

void CheckMemory(AcceleratorIterator iter);
void CheckMemory(AcceleratorIterator iter,MemType type,Arena* arena);
void DisplayInstanceMemory(ComplexFUInstance* inst);
void DisplayAcceleratorMemory(Accelerator* topLevel);
void DisplayUnitConfiguration(Accelerator* topLevel);
void DisplayUnitConfiguration(AcceleratorIterator iter);

//bool IsGraphValid(AcceleratorView view);
void OutputGraphDotFile(Versat* versat,Accelerator* accel,bool collapseSameEdges,const char* filenameFormat,...);

String PushMemoryHex(Arena* arena,void* memory,int size);
void OutputMemoryHex(void* memory,int size);

Array<int> PrintVCDDefinitions(FILE* accelOutputFile,Accelerator* accel,Arena* sameCheckSpace);
void PrintVCD(FILE* accelOutputFile,Accelerator* accel,int time,int clock,Array<int> sameCheckSpace);

void SetDebugSignalHandler(SignalHandler func);

#endif // INCLUDED_DEBUG

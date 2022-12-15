#ifndef INCLUDED_DEBUG
#define INCLUDED_DEBUG

#include "type.hpp"

void CheckMemory(Accelerator* topLevel,Accelerator* accel);
void DisplayInstanceMemory(ComplexFUInstance* inst);
void DisplayAcceleratorMemory(Accelerator* topLevel);
void DisplayUnitConfiguration(Accelerator* topLevel);
void DisplayUnitConfiguration(AcceleratorView topLevel);

bool IsGraphValid(AcceleratorView view);
void OutputGraphDotFile(Versat* versat,AcceleratorView view,bool collapseSameEdges,const char* filenameFormat,...);

void OutputMemoryHex(void* memory,int size);

void PrintVCDDefinitions(FILE* accelOutputFile,Accelerator* accel);
void PrintVCD(FILE* accelOutputFile,Accelerator* accel,int time,int clock);

void SetDebuggingValue(Value val);
void StartDebugTerminal();

void DebugTerminal(Value initialValue);

#endif // INCLUDED_DEBUG

#ifndef INCLUDED_DEBUG
#define INCLUDED_DEBUG

#include "type.hpp"

typedef void (*SignalHandler)(int sig);

String FuzzText(String formattedExample,Arena* arena,int seed = COMPILE_TIME); // Pass a string of valid tokens separated by spaces

void OutputGraphDotFile(SimpleGraph graph,bool collapseSameEdges,const char* filenameFormat,...);

void CheckMemory(AcceleratorIterator iter);
void DisplayInstanceMemory(ComplexFUInstance* inst);
void DisplayAcceleratorMemory(Accelerator* topLevel);
void DisplayUnitConfiguration(Accelerator* topLevel);
void DisplayUnitConfiguration(AcceleratorIterator iter);

bool IsGraphValid(AcceleratorView view);
void OutputGraphDotFile(Versat* versat,AcceleratorView& view,bool collapseSameEdges,const char* filenameFormat,...);

String PushMemoryHex(Arena* arena,void* memory,int size);
void OutputMemoryHex(void* memory,int size);

Array<int> PrintVCDDefinitions(FILE* accelOutputFile,Accelerator* accel,Arena* sameCheckSpace);
void PrintVCD(FILE* accelOutputFile,Accelerator* accel,int time,int clock,Array<int> sameCheckSpace);

void SetDebuggingValue(Value val);
void StartDebugTerminal();
void SetDebugSignalHandler(SignalHandler function);

void DebugTerminal(Value initialValue);

#endif // INCLUDED_DEBUG

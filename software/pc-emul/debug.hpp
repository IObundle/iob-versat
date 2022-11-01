#ifndef INCLUDED_DEBUG
#define INCLUDED_DEBUG

#include "type.hpp"

void DisplayInstanceMemory(ComplexFUInstance* inst);
void DisplayAcceleratorMemory(Accelerator* topLevel);

void OutputMemoryHex(void* memory,int size);

void DebugTerminal(Value initialValue);

#endif // INCLUDED_DEBUG

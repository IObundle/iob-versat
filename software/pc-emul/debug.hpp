#ifndef INCLUDED_DEBUG
#define INCLUDED_DEBUG

#include "type.hpp"

struct RandomGraphTypeOption{
   FUDeclaration* decl;
   float probability;
};

struct RandomGraphOptions{
   int numberInputs;
   int numberOutputs;

   int maxNodes;
   int minNodes;

   int maxDepth;
   int minDepth;

   RandomGraphTypeOption* types;
   int numberTypes;
};

Accelerator* GenerateRandomGraph(Versat* versat,RandomGraphOptions* options,unsigned int seed);

void DisplayInstanceMemory(ComplexFUInstance* inst);
void DisplayAcceleratorMemory(Accelerator* topLevel);

void OutputMemoryHex(void* memory,int size);

void SetDebuggingValue(Value val);
void StartDebugTerminal();

void DebugTerminal(Value initialValue);

#endif // INCLUDED_DEBUG

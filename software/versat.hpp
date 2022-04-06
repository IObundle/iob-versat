#ifndef INCLUDED_VERSAT_HPP
#define INCLUDED_VERSAT_HPP

#include <stdio.h>
#include <stdint.h>
#include <unordered_map>

extern "C"{
   #include "utils.h"
   #include "versat.h"
}

#include "utils.hpp"

struct ConnectionInfo{
   PortInstance inst;
   int port;
   int delay;
};

struct GraphComputedData{
   int numberInputs;
   int numberOutputs;
   ConnectionInfo* inputs; // Delay not used
   ConnectionInfo* outputs;
   enum {TAG_UNCONNECTED,TAG_COMPUTE,TAG_SOURCE,TAG_SINK,TAG_SOURCE_AND_SINK} nodeType;
   int inputDelay;
};

struct Versat{
	FUDeclaration* declarations;
	int nDeclarations;
	Pool<Accelerator> accelerators;

	int numberConfigurations;

   // Options
   int byteAddressable;
   int useShadowRegisters;
};

struct Accelerator{
   Versat* versat;

   Pool<FUInstance> instances;
	Pool<Edge> edges;

   Pool<FUInstance*> inputInstancePointers;
   FUInstance* outputInstance;

	void* configuration;
	int configurationSize;

	bool init;

   //Configuration* savedConfigurations;
};

struct UnitInfo{
   int nConfigsWithDelay;
   int configBitSize;
   int nStates;
   int stateBitSize;
   int memoryMappedBytes;
   int implementsDelay;
   int numberDelays;
   int implementsDone;
   int doesIO;
};

#endif // INCLUDED_VERSAT_HPP

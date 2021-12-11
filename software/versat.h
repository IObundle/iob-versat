#ifndef INCLUDED_VERSAT
#define INCLUDED_VERSAT

#include "stdint.h"
#include "stdbool.h"

#ifdef __cplusplus
#define EXPORT extern "C"
#else
#define EXPORT
#endif

typedef unsigned char byte;

typedef struct FUInstance_t FUInstance;

typedef int32_t* (*FUFunction)(FUInstance*);
typedef int32_t (*MemoryAccessFunction)(FUInstance* instance, int address, int value,int write);

// Type system to prevent misuse
typedef struct FU_Type_t{
	int type;
} FU_Type;

typedef struct Wire_t{
   const char* name;
   int bitsize;
} Wire;

#define VERSAT_TYPE_SINK             0x1 // Sink of data in the circuit.
#define VERSAT_TYPE_SOURCE           0x2 // If the unit is a source of data
#define VERSAT_TYPE_IMPLEMENTS_DELAY 0x4 // Wether unit implements delay or not
#define VERSAT_TYPE_SOURCE_DELAY     0x8 // Wether the unit has delay to produce data, or to process data (1 - delay on source,0 - delay on compute/sink)

typedef struct FUDeclaration_t{
	const char* name;

   int nInputs;
   int nOutputs;

   // Config and state interface
   int nConfigs;
   const Wire* configWires;

   int nStates;
   const Wire* stateWires;

   int latency; // Assume, for now, every port has the same latency
   int type;

   int memoryMapBytes;
   int extraDataSize;
   bool doesIO;
   FUFunction initializeFunction;
	FUFunction startFunction;
	FUFunction updateFunction;
   MemoryAccessFunction memAccessFunction;
} FUDeclaration;

typedef struct{
   FUInstance* instance;
   int index;
} FUInput;

typedef struct FUInstance_t{
	FUDeclaration* declaration;
	FUInput* inputs;
	int32_t* outputs;
	int32_t* storedOutputs;
   void* extraData;

   // Embedded memory
   volatile int* memMapped;
   volatile int* config;
   volatile int* state;

   // Auxiliary used by other algorithms
   // For now, we do not need to know which output is connected
   // (Ex: a unit with 4 outputs might only have 1 in numberOutputs,
   //  we do not need to know which output is the one that is connected)
   struct FUInstance_t** outputInstances;
   int numberOutputs;

   int* delays; // How many cycles unit must wait before seeing valid data
   char tag; // Various uses
} FUInstance;

typedef struct Accelerator_t Accelerator;

typedef struct Versat_t{
	FUDeclaration* declarations;
	int nDeclarations;
	Accelerator* accelerators;
	int nAccelerators;

   // Options
   int byteAddressable;
   int useShadowRegisters;
} Versat;

typedef struct Accelerator_t{
	Versat* versat;
	FUInstance* instances;
	int nInstances;

   // Used by other algorithms
   FUInstance** nodeOutputsAuxiliary;
} Accelerator;

// Versat functions
EXPORT void InitVersat(Versat* versat,int base);

EXPORT FU_Type RegisterFU(Versat* versat,FUDeclaration declaration);

EXPORT void OutputVersatSource(Versat* versat,const char* definitionFilepath,const char* sourceFilepath);

EXPORT void OutputMemoryMap(Versat* versat);

// Accelerator functions
EXPORT Accelerator* CreateAccelerator(Versat* versat);

EXPORT FUInstance* CreateFUInstance(Accelerator* accel,FU_Type type);

EXPORT void AcceleratorRun(Accelerator* accel,FUInstance* endRoot,FUFunction terminateFunction);

EXPORT void IterativeAcceleratorRun(Accelerator* accel,FUInstance* endRoot,FUFunction terminateFunction);

// Helper functions
EXPORT int32_t GetInputValue(FUInstance* instance,int index);

EXPORT void ConnectUnits(FUInstance* out,int outIndex,FUInstance* in,int inIndex);

EXPORT void VersatUnitWrite(FUInstance* instance,int address, int value);
EXPORT int32_t VersatUnitRead(FUInstance* instance,int address);

EXPORT void CalculateDelay(Accelerator* accel);
EXPORT void CalculateNodesOutputs(Accelerator* accel);
EXPORT void DAGOrdering(Accelerator* accel);

/*
EXPORT void CalculatePropagateDelay(Accelerator* accel);
EXPORT int CalculateFullLatency(FUInstance* root);
EXPORT void CalculateDelay(FUInstance* root);
*/

// FDInstance functions
#if 0
void SetMemoryMappedValue(FDInstance* instance,int byte_offset,int32_t value);

int32_t GetMemoryMappedValue(FDInstance* instance,int byte_offset);

void SetConfig(FDInstance* instance,int byte_offset,int32_t value);

int32_t GetState(FDInstance* instance,int byte_offset);
#endif

#endif

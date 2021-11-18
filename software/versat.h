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

typedef struct FUDeclaration_t{
	const char* name;
	
   int nInputs;
   int nOutputs;

   // Config and state interface
   int nConfigs;
   const Wire* configWires;
   
   int nStates;
   const Wire* stateWires;

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
	FU_Type declaration;
	FUInput* inputs;
	int32_t* outputs;
	int32_t* storedOutputs;
   void* extraData;

   // Embedded memory 
   volatile int* memMapped;
   volatile int* config;
   volatile int* state;
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
} Accelerator;

// Versat functions
EXPORT void InitVersat(Versat* versat,int base);

EXPORT FU_Type RegisterFU(Versat* versat,const char* declarationName,
                          int nInputs,int nOutputs,
                          int nConfigs,const Wire* configWires,
                          int nStates,const Wire* stateWires,
                          int memoryMapBytes,bool doesIO,
                          int extraDataSize,
                          FUFunction initializeFunction,FUFunction startFunction,FUFunction updateFunction,
                          MemoryAccessFunction memAccessFunction);

EXPORT void OutputVersatSource(Versat* versat,const char* definitionFilepath,const char* sourceFilepath);

EXPORT void OutputMemoryMap(Versat* versat);

// Accelerator functions
EXPORT Accelerator* CreateAccelerator(Versat* versat);

EXPORT FUInstance* CreateFUInstance(Accelerator* accel,FU_Type type);

EXPORT void AcceleratorRun(Versat* versat,Accelerator* accel,FUInstance* endRoot,FUFunction terminateFunction);

EXPORT void IterativeAcceleratorRun(Versat* versat,Accelerator* accel,FUInstance* endRoot,FUFunction terminateFunction);

// Helper functions
EXPORT FUDeclaration* GetDeclaration(Versat* versat,FUInstance* instance);

EXPORT int32_t GetInputValue(FUInstance* instance,int index);

EXPORT void ConnectUnits(Versat* versat,FUInstance* out,int outIndex,FUInstance* in,int inIndex);

EXPORT void VersatUnitWrite(Versat* versat,FUInstance* instance,int address, int value);
EXPORT int32_t VersatUnitRead(Versat* versat,FUInstance* instance,int address);

// FDInstance functions
#if 0
void SetMemoryMappedValue(FDInstance* instance,int byte_offset,int32_t value);

int32_t GetMemoryMappedValue(FDInstance* instance,int byte_offset);

void SetConfig(FDInstance* instance,int byte_offset,int32_t value);

int32_t GetState(FDInstance* instance,int byte_offset);
#endif

#endif
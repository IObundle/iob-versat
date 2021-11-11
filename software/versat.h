#ifndef INCLUDED_VERSAT
#define INCLUDED_VERSAT

#include "stdint.h"
#include "stdbool.h"

typedef unsigned char byte;

typedef struct FUInstance_t FUInstance;

typedef int32_t* (*FUFunction)(FUInstance*);

// Type system to prevent misuse
typedef struct FU_Type_t{
	int type;
} FU_Type;

typedef struct FUDeclaration_t{
	const char* name;
	
   int nInputs;
   int nOutputs;

   // Config and state interface
   int nConfigs;
   int const* configBits;
   int nStates;
   int const* stateBits;

   int memoryMapBytes;
   int extraDataSize;
   bool doesIO;
   FUFunction initializeFunction;
	FUFunction startFunction;
	FUFunction updateFunction;
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
void InitVersat(Versat* versat,int base);

FU_Type RegisterFU(Versat* versat,const char* declarationName,int nInputs,int nOutputs,int nConfigs,const int* configBits,int nStates,const int* stateBits,int memoryMapBytes,bool doesIO,int extraDataSize,FUFunction initializeFunction,FUFunction startFunction,FUFunction updateFunction);

void OutputVersatSource(Versat* versat,const char* definitionFilepath,const char* sourceFilepath);

void OutputMemoryMap(Versat* versat);

// Accelerator functions
Accelerator* CreateAccelerator(Versat* versat);

FUInstance* CreateFUInstance(Accelerator* accel,FU_Type type);

void AcceleratorRun(Versat* versat,Accelerator* accel,FUInstance* endRoot,FUFunction terminateFunction);

void IterativeAcceleratorRun(Versat* versat,Accelerator* accel,FUInstance* endRoot,FUFunction terminateFunction);

// Helper functions
FUDeclaration* GetDeclaration(Versat* versat,FUInstance* instance);

int32_t GetInputValue(FUInstance* instance,int index);

void ConnectUnits(Versat* versat,FUInstance* out,int outIndex,FUInstance* in,int inIndex);

// FDInstance functions
#if 0
void SetMemoryMappedValue(FDInstance* instance,int byte_offset,int32_t value);

int32_t GetMemoryMappedValue(FDInstance* instance,int byte_offset);

void SetConfig(FDInstance* instance,int byte_offset,int32_t value);

int32_t GetState(FDInstance* instance,int byte_offset);
#endif

#endif
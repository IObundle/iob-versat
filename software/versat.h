#ifndef INCLUDED_VERSAT
#define INCLUDED_VERSAT

#include "stdint.h"
#include "stdbool.h"

typedef unsigned char byte;

typedef struct FUInstanceTag FUInstance;

typedef int32_t (*FUFunction)(FUInstance*);

// Type system to prevent misuse
typedef struct FU_TypeTag{
	int type;
} FU_Type;

typedef struct FUDeclarationTag{
	const char* name;
	int extraBytes;
   int configBits;
   int memoryMapBytes;
   bool doesIO;
	FUFunction startFunction;
	FUFunction updateFunction;
} FUDeclaration;

typedef struct FUInstanceTag{
	FU_Type declaration;
	bool computed;
	FUInstance* inputs[2]; // Only value that can be changed by user, the rest leave it as it is
	int32_t output; // For now hardcode output to 32bits
	int32_t storedOutput;
	void* extraData;
   int configDataIndex;
} FUInstance;

struct AcceleratorTag;

typedef struct VersatTag{
	FUDeclaration* declarations;
	int nDeclarations;
	struct AcceleratorTag* accelerators;
	int nAccelerators;
} Versat;

typedef struct AcceleratorTag{
	Versat* versat;
	FUInstance* instances;
	int nInstances;
   int configDataSize;
} Accelerator;

// Versat functions
void InitVersat(Versat* versat);

FU_Type RegisterFU(Versat* versat,const char* declarationName,int extraBytes,int configBits,int memoryMapBytes,bool doesIO,FUFunction startFunction,FUFunction updateFunction);

void OutputVersatSource(Versat* versat,const char* definitionFilepath,const char* sourceFilepath);

// Accelerator functions
Accelerator* CreateAccelerator(Versat* versat);

FUInstance* CreateFUInstance(Accelerator* accel,FU_Type type);

void AcceleratorRun(Versat* versat,Accelerator* accel,FUInstance* endRoot,FUFunction terminateFunction);

#endif
#ifndef INCLUDED_VERSAT
#define INCLUDED_VERSAT

#include <stdint.h>

typedef unsigned char byte;

struct FUInstance;

typedef int32_t (*FUFunction)(FUInstance*);

// Type system to prevent misuse
struct FU_Type{
	int type;
};

struct FUDeclaration{
	const char* name;
	int extraBytes;
	FUFunction startFunction;
	FUFunction updateFunction;
};

struct FUInstance{
	FU_Type declaration;
	bool computed;
	FUInstance* inputs[2]; // Only value that can be changed by user, the rest leave it as it is
	int32_t output; // For now hardcode output to 32bits
	int32_t storedOutput;
	void* extraData;
};

struct Accelerator;

struct Versat{
	FUDeclaration* declarations;
	int nDeclarations;
	Accelerator* accelerators;
	int nAccelerators;
};

struct Accelerator{
	Versat* versat;
	FUInstance* instances;
	int nInstances;
};

// Versat functions
void InitVersat(Versat* versat);

FU_Type RegisterFU(Versat* versat,const char* declarationName,int extraBytes,FUFunction startFunction,FUFunction updateFunction);

// Accelerator functions
Accelerator* CreateAccelerator(Versat* versat);

FUInstance* CreateFUInstance(Accelerator* accel,FU_Type type);

void AcceleratorRun(Versat* versat,Accelerator* accel,FUInstance* endRoot,FUFunction terminateFunction);

#endif
#include <stdlib.h>
#include <stdio.h>

#include "../versat.h"

// TODO: change memory from static to dynamically allocation. For now, allocate a static amount of memory
void InitVersat(Versat* versat){
	versat->accelerators = (Accelerator*) malloc(10 * sizeof(Accelerator));
	versat->declarations = (FUDeclaration*) malloc(10 * sizeof(FUDeclaration));
}

FU_Type RegisterFU(Versat* versat,const char* declarationName,int extraBytes,FUFunction startFunction,FUFunction updateFunction){
	FUDeclaration decl = {};
	FU_Type type = {};

	decl.name = declarationName;
	decl.extraBytes = extraBytes;
	decl.startFunction = startFunction;
	decl.updateFunction = updateFunction;

	type.type = versat->nDeclarations;
	versat->declarations[versat->nDeclarations++] = decl;

	return type;
}

static void InitAccelerator(Accelerator* accel){
	accel->instances = (FUInstance*) malloc(100 * sizeof(FUInstance));
}

Accelerator* CreateAccelerator(Versat* versat){
	Accelerator accel = {};

	InitAccelerator(&accel);

	accel.versat = versat;
	versat->accelerators[versat->nAccelerators] = accel;

	return &versat->accelerators[versat->nAccelerators++];
}

FUInstance* CreateFUInstance(Accelerator* accel,FU_Type type){
	FUInstance instance = {};
	FUDeclaration decl = accel->versat->declarations[type.type];

	instance.declaration = type;
	if(decl.extraBytes)
		instance.extraData = (void*) malloc(decl.extraBytes);

	accel->instances[accel->nInstances] = instance;

	return &accel->instances[accel->nInstances++];
}

static void Compute(Versat* versat,FUInstance* instance){
	if(instance->computed)
		return;

	instance->computed = true;

	if(instance->inputs[0])
		Compute(versat,instance->inputs[0]);
	if(instance->inputs[1])
		Compute(versat,instance->inputs[1]);

	instance->storedOutput = versat->declarations[instance->declaration.type].updateFunction(instance);
}

void AcceleratorRun(Versat* versat,Accelerator* accel,FUInstance* endRoot,FUFunction terminateFunction){
	for(int i = 0; i < accel->nInstances; i++){
		FUInstance* inst = &accel->instances[i];
		FUFunction startFunction = versat->declarations[inst->declaration.type].startFunction;

		if(startFunction)
			startFunction(inst);
	}

	while(true){
		for(int i = 0; i < accel->nInstances; i++){
			accel->instances[i].computed = false;
			accel->instances[i].output = accel->instances[i].storedOutput; 
		}

		Compute(versat,endRoot);

		// For now, the end is kinda hardcoded.
		if(terminateFunction(endRoot))
			break;
	}
}
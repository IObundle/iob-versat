#include <stdlib.h>
#include <stdio.h>

#include "../versat.h"
#include "versat_instance_template.h"

// TODO: change memory from static to dynamically allocation. For now, allocate a static amount of memory
void InitVersat(Versat* versat){
	versat->accelerators = (Accelerator*) malloc(10 * sizeof(Accelerator));
	versat->declarations = (FUDeclaration*) malloc(10 * sizeof(FUDeclaration));
}

FU_Type RegisterFU(Versat* versat,const char* declarationName,int extraBytes,int configBits,int memoryMapBytes,bool doesIO,FUFunction startFunction,FUFunction updateFunction){
	FUDeclaration decl = {};
	FU_Type type = {};

	decl.name = declarationName;
	decl.extraBytes = extraBytes;
   decl.configBits = configBits;
   decl.memoryMapBytes = memoryMapBytes;
   decl.doesIO = doesIO;
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
   if(decl.configBits){
      instance.configDataIndex = accel->configDataSize;
      accel->configDataSize += decl.configBits;
   }

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

		#if 0
		for(int i = 0; i < accel->nInstances; i++){
			if(i % 5 == 0 && i != 0)
				printf("\n");
			printf("%4d ",accel->instances[i].output);
		}
		printf("\n");
		printf("\n");
		#endif

		// For now, the end is kinda hardcoded.
		if(terminateFunction(endRoot))
			break;
	}
	//exit(0);
}

#define TAB "   "

static int log2i(int value){
   int res = 0;
   while(value != 1){
      value /= 2;
      res++;
   }

   return res;
}

void OutputVersatSource(Versat* versat,const char* definitionFilepath,const char* sourceFilepath){
   FILE* s = fopen(sourceFilepath,"w");

   if(!s){
      printf("Error creating file, check if filepath is correct: %s\n",sourceFilepath);
      return;
   }

   FILE* d = fopen(definitionFilepath,"w");

   if(!d){
      printf("Error creating file, check if filepath is correct: %s\n",sourceFilepath);
      fclose(s);
      return;
   }

   // Only dealing with 1 accelerator, for now
   Accelerator* accel = &versat->accelerators[0];

   int configBits = 0;
   for(int i = 0; i < accel->nInstances; i++){
      FUInstance* instance = &accel->instances[i];

      FU_Type type = instance->declaration;
      FUDeclaration* decl = &versat->declarations[type.type];

      configBits += decl->configBits;
   }

   fprintf(d,"`define CONFIG_W %d\n",configBits);

   fprintf(s,versat_instance_template_str);

   fprintf(s,"wire [31:0] unused");
   for(int i = 0; i < accel->nInstances; i++){
      fprintf(s,",output_%d",i);
   }
   fprintf(s,";\n\n");

   for(int i = 0; i < accel->nInstances; i++){
      FUInstance* instance = &accel->instances[i];

      FU_Type type = instance->declaration;
      FUDeclaration* decl = &versat->declarations[type.type];

      fprintf(s,"%s%s unit%d(\n",TAB,decl->name,i);
      fprintf(s,"%s.out(output_%d),\n",TAB,i);
      
      if(instance->inputs[0]){
         int index = instance->inputs[0] - accel->instances;
         fprintf(s,"%s.in1(output_%d),\n",TAB,index);
      } else {
         fprintf(s,"%s.in1(32'h0),\n",TAB);
      }

      if(instance->inputs[1]){
         int index = instance->inputs[1] - accel->instances;
         fprintf(s,"%s.in2(output_%d),\n",TAB,index);
      } else {
         fprintf(s,"%s.in2(32'h0),\n",TAB);
      }
      #if 1
      if(decl->configBits){
         fprintf(s,"%s.configdata(configdata[%d:%d]),\n",TAB,(instance->configDataIndex + decl->configBits) - 1,instance->configDataIndex);
      }
      #else
         fprintf(s,"%s.configdata(0),\n",TAB);
      #endif
      if(decl->memoryMapBytes){
         fprintf(s,"%s.valid(valid),\n",TAB);
         fprintf(s,"%s.we(we),\n",TAB);
         fprintf(s,"%s.addr(addr[%d:0]),\n",TAB,log2i(decl->memoryMapBytes) - 1);
         fprintf(s,"%s.rdata(rdata),\n\n",TAB);
      }
      fprintf(s,"%s.clk(clk),\n",TAB);
      fprintf(s,"%s.rst(rst)\n",TAB);
      fprintf(s,"%s);\n\n",TAB);
   }

   fprintf(s,"endmodule");

   fclose(s);
   fclose(d);
}
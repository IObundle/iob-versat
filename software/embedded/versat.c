#include "versat.h"
#include "printf.h"

#define MEMSET(base, location, value) (*((volatile int*) (base + (sizeof(int)) * location)) = value)
#define MEMGET(base, location)        (*((volatile int*) (base + (sizeof(int)) * location)))

static int versat_base;

extern int* number_versat_configurations;
extern int  versat_configurations;

// TODO: change memory from static to dynamically allocation. For now, allocate a static amount of memory
void InitVersat(Versat* versat,int base,int numberConfigurations){
   static Accelerator accel;
   static FUDeclaration decls[100];

   printf("Embedded Versat\n");

   versat->accelerators = &accel;
   versat->declarations = decls;
   versat_base = base;

   MEMSET(versat_base,0x0,2);
   MEMSET(versat_base,0x0,0);
}

// Start adding the concept of configuration to versat
// The board does not compute anything, the firmware generates static structs and arrays that are coded in the firmware
// The versat side simply uses those values

FU_Type RegisterFU(Versat* versat,FUDeclaration decl){
   FU_Type type = {};

   type.type = versat->nDeclarations;
   versat->declarations[versat->nDeclarations++] = decl;

   return type;
}

static void InitAccelerator(Accelerator* accel){
   static FUInstance instanceBuffer[100];

   accel->instances = instanceBuffer;
}

// Accelerator functions
Accelerator* CreateAccelerator(Versat* versat){
   Accelerator accel = {};

   InitAccelerator(&accel);

   accel.versat = versat;
   versat->accelerators[versat->nAccelerators] = accel;

   return &versat->accelerators[versat->nAccelerators++];
}

FUInstance* CreateFUInstance(Accelerator* accel,FU_Type type){
   static int createdMem = 0;
   static int createdConfig = 1; // Versat registers occupy position 0
   static int createdState = 1; // Versat registers occupy position 0

   FUInstance instance = {};
   FUDeclaration* decl = &accel->versat->declarations[type.type];

   instance.declaration = decl;

   instance.delays = (volatile int*) (versat_base + sizeof(int) * createdConfig);
   createdConfig += UnitDelays(&instance);

   if(decl->nStates){
      instance.state = (volatile int*) (versat_base + sizeof(int) * createdState);
      createdState += decl->nStates;
   }
   if(decl->nConfigs){
      instance.config = (volatile int*) (versat_base + sizeof(int) * createdConfig);
      createdConfig += decl->nConfigs;
   }
   if(decl->memoryMapBytes){
      instance.memMapped = (volatile int*) (versat_base + 0x10000 + 1024 * sizeof(int) * (createdMem++));
   }
   accel->instances[accel->nInstances] = instance;

   return &accel->instances[accel->nInstances++];
}

void AcceleratorRun(Accelerator* accel){
   MEMSET(versat_base,0x0,1);

   while(1){
      int val = MEMGET(versat_base,0x0);

      if(val)
         break;
   }
}

void OutputVersatSource(Versat* versat,const char* declarationFilepath,const char* sourceFilepath,const char* configurationFilepath){

}

void OutputMemoryMap(Versat* versat){

}

int32_t GetInputValue(FUInstance* instance,int index){
   return 0;
}

void ConnectUnits(FUInstance* out,int outIndex,FUInstance* in,int inIndex){

}

void VersatUnitWrite(FUInstance* instance,int address, int value){
   instance->memMapped[address] = value;
}

int32_t VersatUnitRead(FUInstance* instance,int address){
   int32_t res = instance->memMapped[address];

   return res;
}

void SaveConfiguration(Accelerator* accel,int configuration){

}

void LoadConfiguration(Accelerator* accel,int configuration){

}

extern StoredConfigData config_0[];
extern int configSize;

// In versat space, simple extract delays from configuration data
void CalculateDelay(Accelerator* accel){
   for(int i = 0; i < configSize; i++){
      StoredConfigData data = config_0[i];

      FUInstance* inst = &accel->instances[data.index];

      for(int ii = 0; ii < data.size; ii++){
         inst->delays[ii] = data.config[ii];
      }
   }
}






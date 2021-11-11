#include "versat.h"
#include "printf.h"

#define MEMSET(base, location, value) (*((volatile int*) (base + (sizeof(int)) * location)) = value)
#define MEMGET(base, location)        (*((volatile int*) (base + (sizeof(int)) * location)))

static int versat_base;

// TODO: change memory from static to dynamically allocation. For now, allocate a static amount of memory
void InitVersat(Versat* versat,int base){
   static Accelerator accel;
   static FUDeclaration decls[100];

   printf("Embedded Versat\n");

   versat->accelerators = &accel;
   versat->declarations = decls;
   versat_base = base;

   MEMSET(versat_base,0x0,2);
   MEMSET(versat_base,0x0,0);
}

FU_Type RegisterFU(Versat* versat,const char* declarationName,int nInputs,int nOutputs,int nConfigs,const int const* configBits,int nStates,const int const* stateBits,int memoryMapBytes,bool doesIO,int extraDataSize,FUFunction startFunction,FUFunction updateFunction){
   FUDeclaration decl = {};
   FU_Type type = {};

   decl.name = declarationName;
   decl.nStates = nStates;
   decl.stateBits = stateBits;
   decl.nConfigs = nConfigs;
   decl.configBits = configBits;
   decl.memoryMapBytes = memoryMapBytes;
   decl.doesIO = doesIO;
   decl.extraDataSize = extraDataSize;
   decl.startFunction = startFunction;
   decl.updateFunction = updateFunction;

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
   FUDeclaration decl = accel->versat->declarations[type.type];

   instance.declaration = type;

   if(decl.stateBits){
      instance.state = (volatile int*) (versat_base + sizeof(int) * createdState);
      createdState += decl.nStates;
   }
   if(decl.configBits){
      instance.config = (volatile int*) (versat_base + sizeof(int) * createdConfig);
      createdConfig += decl.nConfigs;
   }
   if(decl.memoryMapBytes){
      instance.memMapped = (volatile int*) (versat_base + 0x02000 + 1024 * sizeof(int) * (createdMem++));
   }
   accel->instances[accel->nInstances] = instance;

   return &accel->instances[accel->nInstances++];
}

void AcceleratorRun(Versat* versat,Accelerator* accel,FUInstance* endRoot,FUFunction terminateFunction){
   MEMSET(versat_base,0x0,1);

   while(1){
      int val = MEMGET(versat_base,0x0);

      if(val)
         break;
   }
}

void OutputVersatSource(Versat* versat,const char* declarationFilepath,const char* sourceFilepath){

}

void OutputMemoryMap(Versat* versat){

}

FUDeclaration* GetDeclaration(Versat* versat,FUInstance* instance){
   return 0;
}

int32_t GetInputValue(FUInstance* instance,int index){
   return 0;
}

void ConnectUnits(Versat* versat,FUInstance* out,int outIndex,FUInstance* in,int inIndex){

}
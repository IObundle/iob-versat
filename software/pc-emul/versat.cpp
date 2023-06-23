#include "accel.hpp"

#include "versat.hpp"
#include "accelerator.hpp"

#include "verilated.h"

static int zeros[99] = {};
static Array<int> zerosArray = {zeros,99};

#define IMPLEMENT_VERILOG_UNITS

#include "wrapper.inc"

static Versat* versat = nullptr;
static Accelerator* accel = nullptr;

volatile AcceleratorConfig* accelConfig = nullptr;
volatile AcceleratorState* accelState = nullptr;

void versat_init(int base){
   Verilated::traceEverOn(true);

   versat = InitVersat(base,1,false);

   RegisterAllVerilogUnitsVerilog(versat);

   //SetDebug(versat,VersatDebugFlags::OUTPUT_VCD,1);

   accel = CreateAccelerator(versat);
   FUDeclaration* type = GetTypeByName(versat,STRING(acceleratorTypeName));
   CreateFUInstance(accel,type,STRING("TOP"));

   InitializeAccelerator(versat,accel,&versat->temp);

   char* configView = (char*) accel->configAlloc.ptr;
   
   accelConfig = (volatile AcceleratorConfig*) accel->configAlloc.ptr;
   accelState = (volatile AcceleratorState*) accel->stateAlloc.ptr;

   int* delayPtr = (int*) (configView + (delayStart - configStart));
   int* staticPtr = (int*) (configView + (staticStart - configStart));
   
   for(int i = 0; i < ARRAY_SIZE(delayBuffer); i++){
      delayPtr[i] = delayBuffer[i];
   }

   for(int i = 0; i < ARRAY_SIZE(staticBuffer); i++){
      staticPtr[i] = staticBuffer[i];
   }
}

void RunAccelerator(int times){
   AcceleratorRun(accel,times);
}

void VersatMemoryCopy(int* dest,int* data,int size){
   memcpy(dest,data,sizeof(int) * size);
}

void VersatUnitWrite(int addr,int val){
   Arena* temp = &versat->temp;
   BLOCK_REGION(temp);

   AcceleratorIterator iter = {};
   for(InstanceNode* node = iter.Start(accel,temp,true); node; node = iter.Skip()){
      FUInstance* inst = node->inst;
      FUDeclaration* decl = inst->declaration;
      iptr memAddress = (iptr) inst->memMapped;
      iptr delta = (1 << decl->memoryMapBits);
      if(addr >= memAddress && addr < (memAddress + delta)){
         VersatUnitWrite(inst,addr,val);
      }
   }

}

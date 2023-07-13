#include "versat_accel.h"

#define IMPLEMENT_VERILOG_UNITS

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

#define nullptr 0

typedef struct Versat Versat;
typedef struct Accelerator Accelerator;
typedef struct FUDeclaration FUDeclaration;

#ifndef __cplusplus
typedef unsigned char bool;
#else
#include <cstdlib>
#include <cstring>
#endif

static Versat* versat = nullptr;
static Accelerator* accel = nullptr;
volatile AcceleratorConfig* accelConfig = nullptr;
volatile AcceleratorState* accelState = nullptr;

int versat_base;

#ifdef __cplusplus
extern "C"{
#endif

void InitializeVerilator();
Versat* InitVersatC(int base,int numberConfigurations,bool initUnits);
void DebugAcceleratorC(Accelerator* accel);
void RegisterAllVerilogUnitsVerilog(Versat* versat);
Accelerator* CreateSimulableAccelerator(Versat* versat,const char* typeName);
void AcceleratorRunC(Accelerator* accel,int times);
void UnitWrite(Versat* versat,Accelerator* accel,int addrArg,int val);
int UnitRead(Versat* versat,Accelerator* accel,int addr);
void SignalLoopC(Accelerator* accel);
   
void* GetStartOfConfig(Accelerator* accel);
void* GetStartOfState(Accelerator* accel);

#ifdef __cplusplus
}
#endif

void Debug(){
   DebugAcceleratorC(accel);
}

void versat_init(int base){
   InitializeVerilator();

   versat_base = base;
   versat = InitVersatC(base,1,0);

   RegisterAllVerilogUnitsVerilog(versat);

   accel = CreateSimulableAccelerator(versat,acceleratorTypeName);

   char* configView = (char*) GetStartOfConfig(accel);

   accelConfig = (volatile AcceleratorConfig*) GetStartOfConfig(accel);
   accelState = (volatile AcceleratorState*) GetStartOfState(accel);

   iptr* delayPtr = (iptr*) (configView + (delayStart - configStart));
   iptr* staticPtr = (iptr*) (configView + (staticStart - configStart));

   for(int i = 0; i < ARRAY_SIZE(delayBuffer); i++){
      delayPtr[i] = delayBuffer[i];
   }

   for(int i = 0; i < ARRAY_SIZE(staticBuffer); i++){
      staticPtr[i] = staticBuffer[i];
   }   
}

void StartAccelerator(){
   RunAccelerator(1);
}

void EndAccelerator(){
   // Do nothing. Start accelerator does everything, for now
}

void RunAccelerator(int times){
   AcceleratorRunC(accel,times);
}

void VersatMemoryCopy(iptr* dest,iptr* data,int size){
   memcpy(dest,data,sizeof(iptr) * size);
}

void VersatUnitWrite(int addrArg,int val){
   int addr = addrArg - (versat_base + memMappedStart); // Convert back to zero based address
   UnitWrite(versat,accel,addr,val);
}

int VersatUnitRead(int base,int index){
   int addr = base + (index * 4) - (versat_base + memMappedStart); // Convert back to zero based byte space address
   return UnitRead(versat,accel,addr);
}

void SignalLoop(){
   SignalLoopC(accel);
   // Do nothing, for now
}

#include "versat_accel.h"

#ifdef __cplusplus
#include <cstdlib>
#include <cstring>
#else
#include <string.h>
#include <stdbool.h>
#endif

#define IMPLEMENT_VERILOG_UNITS
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#define nullptr 0

typedef struct Versat Versat;
typedef struct Accelerator Accelerator;
typedef struct FUDeclaration FUDeclaration;

//static Versat* versat = nullptr;
//static Accelerator* accel = nullptr;
volatile AcceleratorConfig* accelConfig = nullptr;
volatile AcceleratorState* accelState = nullptr;

iptr versat_base;

// Functions exported by wrapper that allow the accelerator to be simulated
void InitializeVerilator();
void VersatAcceleratorCreate();
void VersatAcceleratorSimulate();

AcceleratorConfig* GetStartOfConfig();
AcceleratorState* GetStartOfState();

bool CreateVCD;
bool SimulateDatabus;

void ConfigCreateVCD(bool value){
   CreateVCD = value;
}

void ConfigSimulateDatabus(bool value){
   SimulateDatabus = value;
}

void versat_init(int base){
   CreateVCD = true;
   SimulateDatabus = true;

   InitializeVerilator();
   VersatAcceleratorCreate();

   accelConfig = GetStartOfConfig();
   accelState = GetStartOfState();

   char* configView = (char*) accelConfig;
   iptr* delayPtr = (iptr*) (configView + (delayStart - configStart));
   iptr* staticPtr = (iptr*) (configView + (staticStart - configStart));

   for(int i = 0; i < ARRAY_SIZE(delayBuffer); i++){
      delayPtr[i] = delayBuffer[i];
   }

   for(int i = 0; i < ARRAY_SIZE(staticBuffer); i++){
      staticPtr[i] = staticBuffer[i];
   }
}

void RunAccelerator(int times){
  for(int i = 0; i < times; i++){
    VersatAcceleratorSimulate();
  }
}

void VersatMemoryCopy(void* dest,void* data,int size){
   memcpy(dest,data,size);
}

#if 0
void StartAccelerator(){
  RunAccelerator(1);
}

void EndAccelerator(){
   // Do nothing. Start accelerator does everything, for now
}

void RunAccelerator(int times){
   AcceleratorRunC(accel,times);
}

void VersatMemoryCopy(void* dest,void* data,int size){
   memcpy(dest,data,size);
}

void VersatUnitWrite(int baseaddr,int index,int val){
  int addr = baseaddr + (index * sizeof(int)) - (versat_base + memMappedStart); // Convert back to zero based address
  UnitWrite(versat,accel,addr,val);
}

int VersatUnitRead(int baseaddr,int index){
  int addr = baseaddr + (index * sizeof(int)) - (versat_base + memMappedStart); // Convert back to zero based byte space address
   return UnitRead(versat,accel,addr);
}

float VersatUnitReadFloat(int base,int index){
   int addr = base + (index * sizeof(int)) - (versat_base + memMappedStart); // Convert back to zero based byte space address
   int value = UnitRead(versat,accel,addr);
   float* view = (float*) &value;
   return *view;
}

void SignalLoop(){
   SignalLoopC(accel);
   // Do nothing, for now
}
#endif

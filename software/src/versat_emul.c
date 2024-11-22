#include "versat_accel.h"

#ifdef __cplusplus
#include <cstdio>
#include <cstdlib>
#include <cstring>
#else
#define nullptr 0
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#endif

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

typeof(accelConfig) accelConfig = nullptr;
typeof(accelState) accelState = nullptr;
volatile AcceleratorStatic* accelStatic = nullptr;

iptr versat_base;

#ifdef __cplusplus
extern "C"{
#endif

// Functions exported by wrapper that allow the accelerator to be simulated
int VersatAcceleratorCyclesElapsed();
void InitializeVerilator();
void VersatAcceleratorCreate();
void VersatAcceleratorSimulate();
void VersatSignalLoop();
int MemoryAccess(int address,int value,int write);

int GetAcceleratorCyclesElapsed();

typeof(accelConfig) GetStartOfConfig();
typeof(accelState) GetStartOfState();
AcceleratorStatic* GetStartOfStatic();

#ifdef __cplusplus
  }
#endif

bool CreateVCD;
bool SimulateDatabus;
bool versatInitialized;

void ConfigEnableDMA(bool value){
}

void ConfigCreateVCD(bool value){
  CreateVCD = value;
}

void ConfigSimulateDatabus(bool value){
  SimulateDatabus = value;
}

void versat_init(int base){
  versatInitialized = true;
  CreateVCD = true;
  SimulateDatabus = true;
  versat_base = base;

  InitializeVerilator();
  VersatAcceleratorCreate();

  VersatLoadDelay(delayBuffer);
  
  accelConfig = GetStartOfConfig();
  accelState = GetStartOfState();
  accelStatic = GetStartOfStatic();
}

static void CheckVersatInitialized(){
  if(!versatInitialized){
    printf("Versat has not been initialized\n");
    fflush(stdout);
    exit(-1);
  }
}

void SignalLoop(){
  VersatSignalLoop();
}

int GetAcceleratorCyclesElapsed(){
  return VersatAcceleratorCyclesElapsed();
}

void RunAccelerator(int times){
  CheckVersatInitialized();

  for(int i = 0; i < times; i++){
    VersatAcceleratorSimulate();
  }
}

void StartAccelerator(){
  CheckVersatInitialized();

  VersatAcceleratorSimulate();
}

void EndAccelerator(){
  // Do nothing. Start accelerator does everything, for now
}

void VersatMemoryCopy(void* dest,const void* data,int size){
  CheckVersatInitialized();

  char* byteViewDest = (char*) dest;
  char* configView = (char*) accelConfig;
  int* view = (int*) data;

  bool destInsideConfig = (byteViewDest >= configView && byteViewDest < configView + AcceleratorConfigSize);
  bool destEndOutsideConfig = destInsideConfig && (byteViewDest + size > configView + AcceleratorConfigSize);

  if(destEndOutsideConfig){
    printf("VersatMemoryCopy: Destination starts inside config and ends outside\n");
    printf("This is most likely an error, no transfer is being made\n");
    return;
  }
  
  if(destInsideConfig){
    memcpy(dest,data,size);
  } else {
    for(int i = 0; i < (size / 4); i++){
      VersatUnitWrite(dest,i,view[i]);
    }
  }
}

void VersatUnitWrite(const void* baseaddr,int index,int val){
  CheckVersatInitialized();

  iptr addr = (iptr) baseaddr + (index * sizeof(int)) - (versat_base + memMappedStart); // Convert back to zero based address
  MemoryAccess(addr,val,1);
}

int VersatUnitRead(const void* baseaddr,int index){
  CheckVersatInitialized();

  iptr addr = (iptr) baseaddr + (index * sizeof(int)) - (versat_base + memMappedStart); // Convert back to zero based byte space address
  int res = MemoryAccess(addr,0,0);
  return res;
}

float VersatUnitReadFloat(const void* base,int index){
  int res = VersatUnitRead(base,index);
  float* view = (float*) &res;
  return *view;
}

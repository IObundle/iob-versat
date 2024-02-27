#include "versat_accel.h"

#ifdef __cplusplus
#include <cstdio>
#include <cstdlib>
#include <cstring>
#else
#define nullptr 0
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#endif

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

volatile AcceleratorConfig* accelConfig = nullptr;
volatile AcceleratorState* accelState = nullptr;

iptr versat_base;

#ifdef __cplusplus
extern "C"{
#endif

// Functions exported by wrapper that allow the accelerator to be simulated
int VersatAcceleratorCyclesElapsed();
void InitializeVerilator();
void VersatAcceleratorCreate();
void VersatAcceleratorSimulate();
int MemoryAccess(int address,int value,int write);

int GetAcceleratorCyclesElapsed();

AcceleratorConfig* GetStartOfConfig();
AcceleratorState* GetStartOfState();

#ifdef __cplusplus
  }
#endif

bool CreateVCD;
bool SimulateDatabus;
bool versatInitialized;

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

  accelConfig = GetStartOfConfig();
  accelState = GetStartOfState();

  char* configView = (char*) accelConfig;
  iptr* delayPtr = (iptr*) (configView + (delayStart - configStart));

  for(int i = 0; i < ARRAY_SIZE(delayBuffer); i++){
    delayPtr[i] = delayBuffer[i];
  }

#if 0
  iptr* staticPtr = (iptr*) (configView + (staticStart - configStart));
  for(int i = 0; i < ARRAY_SIZE(staticBuffer); i++){
    staticPtr[i] = staticBuffer[i];
  }
#endif
}

static void CheckVersatInitialized(){
  if(!versatInitialized){
    printf("Versat has not been initialized\n");
    fflush(stdout);
    exit(-1);
  }
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

void VersatMemoryCopy(void* dest,void* data,int size){
  CheckVersatInitialized();

  char* byteViewDest = (char*) dest;
  char* configView = (char*) accelConfig;
  int* view = (int*) data;

  bool destInsideConfig = (byteViewDest >= configView && byteViewDest < configView + sizeof(AcceleratorConfig));
  bool destEndOutsideConfig = destInsideConfig && (byteViewDest + size >= configView + sizeof(AcceleratorConfig));

  if(destEndOutsideConfig){
    printf("VersatMemoryCopy: Destination starts inside config and ends outside\n");
    printf("This is most likely an error, no transfer is being made\n");
    return;
  }
  
  if(destInsideConfig){
    memcpy(dest,data,size);
  } else {
    for(int i = 0; i < (size / 4); i++){
      VersatUnitWrite((iptr) dest,i,view[i]);
    }
  }
}

void VersatUnitWrite(int baseaddr,int index,int val){
  CheckVersatInitialized();

  int addr = baseaddr + (index * sizeof(int)) - (versat_base + memMappedStart); // Convert back to zero based address
  MemoryAccess(addr,val,1);
}

int VersatUnitRead(int baseaddr,int index){
  CheckVersatInitialized();

  int addr = baseaddr + (index * sizeof(int)) - (versat_base + memMappedStart); // Convert back to zero based byte space address
  int res = MemoryAccess(addr,0,0);
  return res;
}

float VersatUnitReadFloat(int base,int index){
  int res = VersatUnitRead(base,index);
  float* view = (float*) &res;
  return *view;
}

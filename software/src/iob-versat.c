#include "versat_accel.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#define TIME_IT(...) ((void)0)
  
#define MEMSET(base, location, value) (*((volatile int*) (base + (sizeof(int)) * location)) = (int) value)
#define MEMGET(base, location)        (*((volatile int*) (base + (sizeof(int)) * location)))

#ifdef __cplusplus
extern "C"{
#endif

//#include "iob-uart.h"
//#include "printf.h"

#ifdef __cplusplus
  }
#endif

#ifdef __cplusplus
#include <cstdint>
#else
#include "stdint.h"
#include "stdbool.h"
#endif

typedef uint64_t uint64;

// There should be a shared header for common structures, but do not share code.
// It does not work as well and keeps giving compile and linker errors. It's not worth it.

iptr versat_base;
static bool enableDMA;

volatile AcceleratorConfig*  accelConfig  = 0;
volatile AcceleratorState*   accelState   = 0;
volatile AcceleratorStatic*  accelStatics = 0;

void versat_init(int base){
  versat_base = (iptr) base;
  //enableDMA = acceleratorSupportsDMA;
  enableDMA = false;
  
  //printf("Embedded Versat\n");

  MEMSET(versat_base,0x0,0x80000000); // Soft reset

  accelConfig = (volatile AcceleratorConfig*) (versat_base + configStart);
  accelState  = (volatile AcceleratorState*)  (versat_base + stateStart);
  accelStatics = (volatile AcceleratorStatic*)  (versat_base + staticStart);

  VersatLoadDelay(delayBuffer);
}

void StartAccelerator(){
  //printf("Start accelerator\n");
  MEMSET(versat_base,0x0,1);
}

void EndAccelerator(){
  //printf("End accelerator\n");
  while(1){
    volatile int val = MEMGET(versat_base,0x0);
    if(val){
      break;
    }
  } 
}

static inline void RunAcceleratorOnce(int times){ // times inside value amount
   MEMSET(versat_base,0x0,times);
   EndAccelerator();
}

void RunAccelerator(int times){
  for(; times > 0xffff; times -= 0xffff){
    RunAcceleratorOnce(times);
  } 

  RunAcceleratorOnce(times);
}

void SignalLoop(){
  MEMSET(versat_base,0x0,0x40000000);
}

void VersatMemoryCopy(void* dest,const void* data,int size){
  if(size <= 0){
    return;
  }

  //TIME_IT("Memory copy");

  iptr destInt = (iptr) dest;
  iptr dataInt = (iptr) data;

  bool destInsideVersat = false;
  bool dataInsideVersat = false;

  if(destInt >= versat_base && (destInt < versat_base + versatAddressSpace)){
    destInsideVersat = true;
  }

  if(dataInt >= versat_base && (dataInt < versat_base + versatAddressSpace)){
    dataInsideVersat = true;
  }

  if(dataInsideVersat == destInsideVersat){
    if(dataInsideVersat){
      //printf("Warning, Versat currently cannot DMA between two memory regions inside itself\n");
    } else {
      //printf("Warning, Versat cannot use its DMA when both regions are outside its address space\n");
    }
    //printf("Using a simple copy loop for now\n");
  }

  if(enableDMA && acceleratorSupportsDMA && (dataInsideVersat != destInsideVersat)){
    if(destInsideVersat){
      destInt = destInt - versat_base;
    }
    if(dataInsideVersat){
      dataInt = dataInt - versat_base;
    }

    MEMSET(versat_base,0x1,destInt); // Dest inside 
    MEMSET(versat_base,0x2,dataInt); // Memory address
    MEMSET(versat_base,0x3,size); // Byte size
    MEMSET(versat_base,0x4,0x1); // Start DMA

    while(1){
      int val = MEMGET(versat_base,0x1);
      if(val) break;
    }
  } else {
    int* destView = (int*) dest;
    int* dataView = (int*) data;
    for(int i = 0; i < size / sizeof(int); i++){
      destView[i] = dataView[i];
    }
  }
}

void VersatUnitWrite(const void* baseaddr,int index,int val){
  //int* ptr = (int*) (baseaddr + index * sizeof(int));
  //*ptr = val;
  iptr base = (iptr) baseaddr;

  MEMSET(base,index,val);
}

int VersatUnitRead(const void* baseaddr,int index){
  iptr base = (iptr) baseaddr;
  return MEMGET(base,index);
  //int* ptr = (int*) (base + index * sizeof(int));
  //return *ptr;
}

float VersatUnitReadFloat(const void* baseaddr,int index){
  // float* ptr = (float*) (base + index * sizeof(float)
  iptr base = (iptr) baseaddr;
  int val = MEMGET(base,index);
  float* ptr = (float*) &val;
  return *ptr;
}

void ConfigEnableDMA(bool value){
  enableDMA = value;
}

void ConfigCreateVCD(bool value){}
void ConfigSimulateDatabus(bool value){}

void VersatLoadDelay(const unsigned int* buffer){
  void* delayBase = (void*) (versat_base + delayStart);
  VersatMemoryCopy(delayBase,buffer,sizeof(int) * ARRAY_SIZE(delayBuffer));
}

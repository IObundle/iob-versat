#include "versat_accel.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#define TIME_IT(...) ((void)0)
  
#define MEMSET(base, location, value) (*((volatile int*) (base + (sizeof(int)) * location)) = (int) value)
#define MEMGET(base, location)        (*((volatile int*) (base + (sizeof(int)) * location)))

#ifdef __cplusplus
extern "C"{
#endif

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

iptr versat_base;
VersatPrintf versatPrintf;

// NOTE: Use PRINT instead of versatPrintf, since versatPrintf is not required to be set.
// NOTE: This is mostly a debug tool that should not be built upon. We want to use pc-emul to report errors to user, we do not want to polute embedded with error checkings that waste precious cpu cycles.
#define PRINT(...) if(versatPrintf){versatPrintf(__VA_ARGS__);}

static bool enableDMA;

// TODO: Need to dephase typeof, it is a gnu extension, not valid C until version C23 which is recent to support
typeof(accelConfig) accelConfig  = 0;
typeof(accelState)  accelState   = 0;
typeof(accelStatic) accelStatic = 0;

@{registerLocation}

void versat_init(int base){
  versat_base = (iptr) base;
  enableDMA = false; // It is more problematic for the general case if we start enabled. More error prone, especially when integrating with linux.

  // TODO: Need to receive a printf like function from outside to enable this, I do not want to tie the implementation to IObSoC.
  //PRINT("Embedded Versat\n");

  MEMSET(versat_base,VersatRegister_Control,0x80000000); // Soft reset

  accelConfig = (typeof(accelConfig)) (versat_base + configStart);
  accelState  = (typeof(accelState))  (versat_base + stateStart);
  accelStatic = (volatile AcceleratorStatic*)  (versat_base + staticStart);

  VersatLoadDelay(delayBuffer);
}

void SetVersatDebugPrintfFunction(VersatPrintf function){
  versatPrintf = function;
  PRINT("Versat successfully set print function\n");
}

void StartAccelerator(){
  //PRINT("Start accelerator\n");
  MEMSET(versat_base,VersatRegister_Control,1);
}

void EndAccelerator(){
  //PRINT("End accelerator\n");
  while(1){
    volatile int val = MEMGET(versat_base,VersatRegister_Control);
    if(val){
      break;
    }
  } 
}

void ResetAccelerator(){
  MEMSET(versat_base,VersatRegister_Control,0x80000000); // Soft reset
  VersatLoadDelay(delayBuffer);
}

static inline void RunAcceleratorOnce(int times){ // times inside value amount
   EndAccelerator();

   MEMSET(versat_base,VersatRegister_Control,times);
   EndAccelerator();
}

void RunAccelerator(int times){
  for(; times > 0xffff; times -= 0xffff){
    RunAcceleratorOnce(0xffff);
  } 

  RunAcceleratorOnce(times);
}

void SignalLoop(){
  MEMSET(versat_base,VersatRegister_Control,0x40000000);
}

void VersatMemoryCopy(volatile void* dest,volatile const void* data,int size){
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
      //PRINT("Warning, Versat currently cannot DMA between two memory regions inside itself\n");
    } else {
      //PRINT("Warning, Versat cannot use its DMA when both regions are outside its address space\n");
    }
    //PRINT("Using a simple copy loop for now\n");
  }

  if(enableDMA && acceleratorSupportsDMA && (dataInsideVersat != destInsideVersat)){
    if(destInsideVersat){
      destInt = destInt - versat_base;
    }
    if(dataInsideVersat){
      dataInt = dataInt - versat_base;
    }

    MEMSET(versat_base,VersatRegister_DmaInternalAddress,destInt); 
    MEMSET(versat_base,VersatRegister_DmaExternalAddress,dataInt);
    MEMSET(versat_base,VersatRegister_DmaTransferLength,size); // Byte size
    MEMSET(versat_base,VersatRegister_DmaControl,0x1); // Start DMA

    while(1){
      int val = MEMGET(versat_base,VersatRegister_DmaControl);
      if(val) break;
    }
  } else {
    volatile int* destView = (volatile int*) dest;
    volatile int* dataView = (volatile int*) data;
    for(int i = 0; i < size / sizeof(int); i++){
      destView[i] = dataView[i];
    }
  }
}

void VersatUnitWrite(volatile const void* baseaddr,int index,int val){
  iptr base = (iptr) baseaddr;

  MEMSET(base,index,val);
}

int VersatUnitRead(volatile const void* baseaddr,int index){
  iptr base = (iptr) baseaddr;
  return MEMGET(base,index);
}

float VersatUnitReadFloat(volatile const void* baseaddr,int index){
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
int SimulateAddressGen(iptr* arrayToFill,int arraySize,AddressVArguments args){return 0;}
SimulateVReadResult SimulateVRead(AddressVArguments args){return (SimulateVReadResult){};}

void VersatLoadDelay(volatile const unsigned int* buffer){
  volatile void* delayBase = (void*) (versat_base + delayStart);
  VersatMemoryCopy(delayBase,buffer,sizeof(int) * ARRAY_SIZE(delayBuffer));
}

// ======================================
// Debug facilities

static inline void DebugEndAccelerator(int upperBound){
  for(int i = 0; i < upperBound; i++){  
    volatile int val = MEMGET(versat_base,VersatRegister_Control);
    if(val){
      break;
    }
  } 
  PRINT("Accelerator reached upperbound\n");
}

static inline void DebugRunAcceleratorOnce(int times,int upperbound){ // times inside value amount
   PRINT("Gonna start accelerator: (%x,%d,%d)\n",versat_base, VersatRegister_Control, times);
   MEMSET(versat_base,VersatRegister_Control,times);
   DebugEndAccelerator(upperbound);

   PRINT("Ended accelerator\n");
}

void DebugRunAccelerator(int times, int maxCycles){
  // Accelerator was previously stuck
  if(!MEMGET(versat_base, VersatRegister_Control)){
    PRINT("Versat accel was previously stuck, before current call to DebugRunAccelerator\n");
    return;
  }

  for(; times > 0xffff; times -= 0xffff){
    DebugRunAcceleratorOnce(0xffff,maxCycles);
  } 

  DebugRunAcceleratorOnce(times,maxCycles);
}

VersatDebugState VersatDebugGetState(){
  VersatDebugState res = {};

  int debugState = MEMGET(versat_base,VersatRegister_Debug); 
  res.databusIsActive = (debugState & 0x1);

  return res;
}

@{profileStuff}
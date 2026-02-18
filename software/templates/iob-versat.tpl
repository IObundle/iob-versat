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

static int versat_empty_printf(const char* format,...){return 0;}

iptr versat_base;
VersatPrintf versat_printf = versat_empty_printf;

// NOTE: Use PRINT instead of versat_printf, since versat_printf is not required to be set.
// NOTE: This is mostly a debug tool that should not be built upon. We want to use pc-emul to report errors to user, we do not want to polute embedded with error checkings that waste precious cpu cycles.
#define PRINT(...) if(versat_printf){versat_printf(__VA_ARGS__);}

static bool enableDMA;

// TODO: Need to dephase typeof, it is a gnu extension, not valid C until version C23 which is too recent to support
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
  versat_printf = function;
  PRINT("Versat successfully set print function\n");
}

void EndAccelerator(){
  //PRINT("End accelerator\n");
  while(1){
    volatile int val = MEMGET(versat_base,VersatRegister_Control);
    if(val){
      return;
    }
  } 
}

void StartAccelerator(){
  EndAccelerator();
  MEMSET(versat_base,VersatRegister_Control,1);
}

void ResetAccelerator(){
  MEMSET(versat_base,VersatRegister_Control,0x80000000); // Soft reset
  VersatLoadDelay(delayBuffer);
}

void RunAccelerator(int times){
  for(int i = 0; i < times; i++){
    StartAccelerator();
  } 
  EndAccelerator();
}

void SignalLoop(){
  MEMSET(versat_base,VersatRegister_Control,0x40000000);
}

void VersatMemoryCopy(volatile void* dest,volatile const void* data,int size){
  if(size <= 0){
    return;
  }

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

@{dmaStuff}
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

@{debugStuff}

@{profileStuff}
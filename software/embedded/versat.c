#include "versat_accel.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#define TIME_IT(...) ((void)0)
  
#define MEMSET(base, location, value) (*((volatile int*) (base + (sizeof(int)) * location)) = (int) value)
#define MEMGET(base, location)        (*((volatile int*) (base + (sizeof(int)) * location)))

#include "iob-timer.h"

#include "printf.h"

#ifdef __cplusplus
#include <cstdint>
#else
#include "stdint.h"
#include "stdbool.h"
#endif

typedef uint64_t uint64;

// There should be a shared header for common structures, but do not share code.
// It does not work as well and keeps giving compile and linker errors. It's not worth it.
typedef struct{
  uint64 microSeconds;
  uint64 seconds;
} Time;

Time GetTime(){
  Time res = {};
  res.seconds = (uint64) timer_time_s();

  res.microSeconds = (uint64) timer_time_us();
  res.microSeconds -= (res.seconds * 1000000);
  
  return res;
}

int versat_base;

volatile AcceleratorConfig* accelConfig = 0;
volatile AcceleratorState*  accelState  = 0;

void versat_init(int base){
  versat_base = base;

  printf("Embedded Versat\n");

  MEMSET(versat_base,0x0,0x80000000); // Soft reset

  accelConfig = (volatile AcceleratorConfig*) (versat_base + configStart);
  accelState  = (volatile AcceleratorState*)  (versat_base + stateStart);

  volatile int* delayBase = (volatile int*) (versat_base + delayStart);
  volatile int* staticBase = (volatile int*) (versat_base + staticStart);

  for(int i = 0; i < ARRAY_SIZE(delayBuffer); i++){  // Hackish, for now
      delayBase[i] = delayBuffer[i];
  }
  for(int i = 0; i < ARRAY_SIZE(staticBuffer); i++){ // Hackish, for now
      staticBase[i] = staticBuffer[i];
  }
}

void StartAccelerator(){
  //printf("Start accelerator\n");
  MEMSET(versat_base,0x0,1);
}

int timesWaiting = 0;

void ClearCache(void*);

void EndAccelerator(){
  //printf("End accelerator\n");
  bool seenWaiting = false;
  while(1){
    volatile int val = MEMGET(versat_base,0x0);
    if(val){
      break;
    }
    if(!seenWaiting){
      timesWaiting += 1;
      seenWaiting = true;
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

void VersatMemoryCopy(iptr* dest,iptr* data,int size){
  if(size <= 0){
    return;
  }

  TIME_IT("Memory copy");

  int destInt = (int) dest;
  int dataInt = (int) data;

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
      printf("Warning, Versat currently cannot DMA between two memory regions inside itself\n");
    } else {
      printf("Warning, Versat cannot use its DMA when both regions are outside its address space\n");
    }
    printf("Using a simple copy loop for now\n");
  }

  if(acceleratorSupportsDMA && (dataInsideVersat != destInsideVersat)){
    MEMSET(versat_base,0x1,dest); // Dest inside 
    MEMSET(versat_base,0x2,data); // Memory address
    MEMSET(versat_base,0x3,size); // Byte size
    MEMSET(versat_base,0x4,0x1); // Start DMA

    while(1){
      int val = MEMGET(versat_base,0x1);
      if(val) break;
    }
  } else {
    for(int i = 0; i < size; i++){
      dest[i] = data[i];
    }
  }
}

void VersatUnitWrite(int baseaddr,int index,int val){
  int* ptr = (int*) (baseaddr + index * sizeof(int));
  *ptr = val;
}

int VersatUnitRead(int base,int index){
  int* ptr = (int*) (base + index * sizeof(int));
  return *ptr;
}

float VersatUnitReadFloat(int base,int index){
  float* ptr = (float*) (base + index * sizeof(float));
  return *ptr;
}

// Implementation of common functionality

static unsigned int randomSeed = 1;
void SeedRandomNumber(unsigned int val){
  if(val == 0){
    randomSeed = 1;
  } else {
    randomSeed = val;
  }
}

unsigned int GetRandomNumber(){
  // Xorshift
  randomSeed ^= randomSeed << 13;
  randomSeed ^= randomSeed >> 17;
  randomSeed ^= randomSeed << 5;
  return randomSeed;
}

static int Abs(int val){
  int res = val;
  if(val < 0){
    res = -val;
  }
  return res;
}

int RandomNumberBetween(int minimum,int maximum){
  int randomValue = GetRandomNumber();
  int delta = maximum - minimum;

  if(delta <= 0){
    return minimum;
  }

  int res = minimum + Abs(randomValue % delta);
  return res;
}


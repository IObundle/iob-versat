#include "versat_accel.h"

#ifdef __cplusplus
#include "utils.hpp"
#else
#undef  ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#define TIME_IT(...) 
#endif
  
#define MEMSET(base, location, value) (*((volatile int*) (base + (sizeof(int)) * location)) = (int) value)
#define MEMGET(base, location)        (*((volatile int*) (base + (sizeof(int)) * location)))

#include "iob-timer.h"

#include "printf.h"

#ifndef __cplusplus
#include "stdint.h"
typedef uint64_t uint64;

// Care, this structure must be the same as the one from utils.
// TODO: Find a way of sharing these even if embedded
typedef struct{
   uint64 seconds;
   uint64 nanoSeconds;
} NanoSecond;
#endif

#if 1
Time GetTime(){
   Time res = {};
   res.seconds = (uint64) timer_time_s();
   res.nanoSeconds = (uint64) timer_time_us() * 1000;
   return res;
}
#endif

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
   MEMSET(versat_base,0x0,1);
}

int timesWaiting = 0;

void EndAccelerator(){
   bool seenWaiting = false;
   while(1){
      int val = MEMGET(versat_base,0x0);
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

   if(acceleratorSupportsDMA){
      MEMSET(versat_base,0x1,dest);
      MEMSET(versat_base,0x2,data);
      MEMSET(versat_base,0x3,size - 1); // AXI size
      MEMSET(versat_base,0x4,0x1); // Start DMA

      while(1){
         int val = MEMGET(versat_base,0x1);

         if(val)
            break;
      }
   } else {
      for(int i = 0; i < size; i++){
         dest[i] = data[i];
      }
   }
}

void VersatUnitWrite(int addr,int val){
   int* ptr = (int*) addr;
   *ptr = val;
}

int VersatUnitRead(int base,int index){
   int* ptr = (int*) base + index;
   return *ptr;
}

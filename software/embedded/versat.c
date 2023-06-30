#include "versat_accel.hpp"

#include "printf.h"
#define MEMSET(base, location, value) (*((volatile int*) (base + (sizeof(int)) * location)) = (int) value)
#define MEMGET(base, location)        (*((volatile int*) (base + (sizeof(int)) * location)))

#include "utilsCore.hpp"

extern "C"{
#include "iob-timer.h"

NanoSecond GetTime(){
   NanoSecond res = {};
   res.time = (uint64) timer_time_us() * 1000;
   return res;
}

int versat_base;

volatile AcceleratorConfig* accelConfig = nullptr;
volatile AcceleratorState*  accelState  = nullptr;

void versat_init(int base){
   versat_base = base;

   printf("Embedded Versat\n");

   MEMSET(versat_base,0x0,0x80000000);
   MEMSET(versat_base,0x0,0);

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

void RunAccelerator(int times){
   MEMSET(versat_base,0x0,times);
   while(1){
      int val = MEMGET(versat_base,0x0);
      if(val){ // We wait until accelerator finishes before returning. Not mandatory, but less error prone and no need to squeeze a lot of performance for now (In the future, the concept of returning immediatly and having the driver tell when to wait will be implemented)  
         break;
      }
   }
}

void VersatMemoryCopy(volatile int* dest,int* data,int size){
   if(size <= 0){
      return;
   }

   TIME_IT("Memory copy");

   MEMSET(versat_base,0x1,dest);
   MEMSET(versat_base,0x2,data);
   MEMSET(versat_base,0x3,size - 1); // AXI size
   MEMSET(versat_base,0x4,0x1); // Start DMA

   while(1){
      int val = MEMGET(versat_base,0x1);

      if(val)
         break;
   }
}

void VersatUnitWrite(int addr,int val){
   int* ptr = (int*) addr;
   *ptr = val;
}

}


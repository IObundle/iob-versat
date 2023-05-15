#include "versat.hpp"
#include "printf.h"

#include "versat_data.inc"

#define MEMSET(base, location, value) (*((volatile int*) (base + (sizeof(int)) * location)) = (int) value)
#define MEMGET(base, location)        (*((volatile int*) (base + (sizeof(int)) * location)))

#if 0
#define DEBUG
#endif

#undef TIME_IT
#define TIME_IT(...) ((void)0)

static int versat_base;

extern int* number_versat_configurations;
extern int  versat_configurations;

Versat* InitVersat(int base,int numberConfigurations){
   static Versat versat = {};

   printf("Embedded Versat\n");

   #ifdef DEBUG
   printf("E\n");
   #endif

   versat_base = base;

   #ifdef DEBUG
   printf("BASE:%x\n",base);
   #endif

   MEMSET(versat_base,0x0,0x80000000);
   MEMSET(versat_base,0x0,0);

   return &versat;
}

// Accelerator functions
void AcceleratorRun(Accelerator* accel,int times){
   static bool init = false;

   if(!init){

      TIME_IT("Init accel");
      // Set delay and static values
      for(int i = 0; i < ARRAY_SIZE(delayBuffer); i++){
         delayBase[i] = delayBuffer[i];
      }
      for(int i = 0; i < ARRAY_SIZE(staticBuffer); i++){ // Hackish, for now
         staticBase[i] = staticBuffer[i];
      }
      init = true;
   }

   #ifdef DEBUG
   printf("B\n");
   #endif

   TIME_IT("Accel run");

   MEMSET(versat_base,0x0,times);

   while(1){
      int val = MEMGET(versat_base,0x0);

      if(val)
         break;
   }
   #ifdef DEBUG
   printf("A\n");
   #endif
}

void VersatUnitWrite(FUInstance* instance,int address, int value){
   instance->memMapped[address] = value;

   #ifdef DEBUG
   printf("W:%x-%x\n",address,value);
   #endif
}

int32_t VersatUnitRead(FUInstance* instance,int address){
   int32_t res = instance->memMapped[address];

   #ifdef DEBUG
   printf("R:%x-%x\n",address,res);
   #endif

   return res;
}

void VersatMemoryCopy(FUInstance* instance,volatile int* dest,int* data,int size){
   if(size <= 0){
      return;
   }

   TIME_IT("Memory copy");

   MEMSET(versat_base,0x1,dest);
   MEMSET(versat_base,0x2,data);
   MEMSET(versat_base,0x3,size - 1); // AXI size
   MEMSET(versat_base,0x4,0x1);

   while(1){
      int val = MEMGET(versat_base,0x1);

      if(val)
         break;
   }
}

void VersatMemoryCopy(FUInstance* instance,volatile int* dest,Array<int> data){
   VersatMemoryCopy(instance,dest,data.data,data.size);
}

static FUInstance* vGetInstanceByName_(int startIndex,int argc, va_list args){
   int index = startIndex;
   for (int i = 0; i < argc; i++){
      char* str = va_arg(args, char*);

      int length = strlen(str);
      int arguments = 0;
      for(int ii = 0; ii < length; ii++){
         if(str[ii] == '%'){
            arguments += 1;
         }
      }

      char buffer[1024];
      if(arguments){
         vsnprintf(buffer,1024,str,args);
         i += arguments;
         for(int ii = 0; ii < arguments; ii++){
            va_arg(args, int); // Need to consume something
         }
      } else {
         strcpy(buffer,str);
      }

      //printf("B:%s\n",buffer);

      for(; index < ARRAY_SIZE(instancesBuffer);){
         //printf("%d %s ",index,instancesBuffer[index].name);
         if(strcmp(buffer,instancesBuffer[index].name) == 0){
            index += 1;
            break;
         } else {
            index += instancesBuffer[index].numberChilds + 1;
         }
         //printf("%d\n",index);
      }

      //printf("I:%d\n",index);
   }

   FUInstance* res = &instancesBuffer[index - 1];

   #ifdef DEBUG
   printf("C:%x\n",res->config);
   printf("S:%x\n",res->state);
   printf("M:%x\n",res->memMapped);
   #endif

   return res;
}

FUInstance* GetInstanceByName_(Accelerator* accel,int argc, ...){
   va_list args;
   va_start(args,argc);

   TIME_IT("Get Instance");

   FUInstance* res = vGetInstanceByName_(0,argc,args);

   va_end(args);

   return res;
}

FUInstance* GetSubInstanceByName_(Accelerator* topLevel,FUInstance* inst,int argc, ...){
   va_list args;
   va_start(args,argc);

   TIME_IT("Get SubInstance");

   int index = inst - instancesBuffer;
   FUInstance* res = vGetInstanceByName_(index + 1,argc,args);

   va_end(args);

   return res;
}

FUInstance* CreateFUInstance(Accelerator* accel,FUDeclaration* type,String entityName){
   FUInstance* res = GetInstanceByName_(accel,1,entityName.data);

   return res;
}

NanoSecond GetTime(){
   NanoSecond res = {};
   res.time = (uint64) timer_time_us() * 1000;
   return res;
}

void VersatSimDebug(Versat* versat){
   MEMSET(versat_base,0x5,0);
}

void Hook(Versat* versat,Accelerator* accel,FUInstance* inst){

}

#include "versatExtra.hpp"

bool InitSimpleAccelerator(SimpleAccelerator* simple,Versat* versat,const char* declarationName){
   bool res = simple->init;
   simple->inst = &instancesBuffer[0];
   for(int i = 0; i < simpleInputs; i++){
      simple->inputs[i] = &instancesBuffer[inputStart + i];
   }
   for(int i = 0; i < simpleOutputs; i++){
      simple->outputs[i] = &instancesBuffer[outputStart + i];
   }
   simple->numberInputs = simpleInputs;
   simple->numberOutputs = simpleOutputs;
   simple->init = true;

   return res;
}

int* vRunSimpleAccelerator(SimpleAccelerator* simple,va_list args){
   static int out[99];

   for(unsigned int i = 0; i < simple->numberInputs; i++){
      int val = va_arg(args,int);
      simple->inputs[i]->config[0] = val;
   }

   AcceleratorRun(simple->accel);

   for(unsigned int i = 0; i < simple->numberOutputs; i++){
      out[i] = simple->outputs[i]->state[0];
   }

   return out;
}

int* RunSimpleAccelerator(SimpleAccelerator* simple, ...){
   va_list args;
   va_start(args,simple);

   int* out = vRunSimpleAccelerator(simple,args);

   va_end(args);

   return out;
}

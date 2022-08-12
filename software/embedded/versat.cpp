#include "versat.hpp"
#include "printf.h"

#include "versat_data.inc"

#define MEMSET(base, location, value) (*((volatile int*) (base + (sizeof(int)) * location)) = value)
#define MEMGET(base, location)        (*((volatile int*) (base + (sizeof(int)) * location)))

#if 0
#define DEBUG
#endif

static int versat_base;

extern int* number_versat_configurations;
extern int  versat_configurations;

Versat* InitVersat(int base,int numberConfigurations){
   static Versat versat = {};

   #ifdef DEBUG
   printf("E\n");
   #endif

   versat_base = base;

   #ifdef DEBUG
   printf("BASE:%x\n",base);
   #endif

   MEMSET(versat_base,0x0,2);
   MEMSET(versat_base,0x0,0);

   return &versat;
}

// Accelerator functions
void AcceleratorRun(Accelerator* accel){
   #ifdef DEBUG
   printf("B\n");
   #endif

   // Set delay and static values
   for(int i = 0; i < ARRAY_SIZE(delayBuffer); i++){
      delayBase[i] = delayBuffer[i];
   }
   for(int i = 0; i < ARRAY_SIZE(staticBuffer); i++){ // Hackish, for now
      staticBase[i] = staticBuffer[i];
   }

   MEMSET(versat_base,0x0,1);

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

FUInstance* GetInstanceByName_(Accelerator* accel,int argc, ...){
   static char buffer[1024];

   va_list args;
   va_start(args,argc);

   bool first = true;
   char* ptr = buffer;
   for (int i = 0; i < argc; i++){
      char* str = va_arg(args, char*);

      if(!first){
         *(ptr++) = '_';
      }
      first = false;

      int length = strlen(str);
      int arguments = 0;
      for(int ii = 0; ii < length; ii++){
         if(str[ii] == '%'){
            arguments += 1;
         }
      }

      if(arguments){
         ptr += vsnprintf(ptr,1024 - (ptr - buffer),str,args);
         i += arguments;
         for(int ii = 0; ii < arguments; ii++){
            va_arg(args, int); // Need to consume something
         }
      } else {
         strcpy(ptr,str);
         ptr += length;
      }
   }

   *ptr = '\0';
   FUInstance* res = nullptr;

   unsigned int hash = 0;
   for(int i = 0; buffer[i] != '\0'; i++){
      hash *= 17;
      hash += buffer[i];
   }

   int hashTableSize = ARRAY_SIZE(instanceHashmap);
   for(int counter = 0; counter < hashTableSize; counter++){
      HashKey data = instanceHashmap[(hash + counter) % hashTableSize];

      if(strcmp(data.key,buffer) == 0){
         res = &instancesBuffer[data.index];
         break;
      } else if(data.index == -1){
         printf("ERROR ON HASH: %d\n",(hash + counter) % hashTableSize);
         printf("STR: %s\n",buffer);
      }

      //printf("%d %d %s\n",i,ARRAY_SIZE(instancesBuffer),buffer);
   }

   if(res == nullptr){
      printf("Didn't find: %s\n",buffer);
   }

   va_end(args);

   #ifdef DEBUG
   printf("C:%x\n",res->config);
   printf("S:%x\n",res->state);
   printf("M:%x\n",res->memMapped);
   #endif

   return res;
}

FUInstance* CreateFUInstance(Accelerator* accel,FUDeclaration* type,SizedString entityName){
   FUInstance* res = GetInstanceByName_(accel,1,entityName.str);

   return res;
}

void Hook(Versat* versat,Accelerator* accel,FUInstance* inst){

}

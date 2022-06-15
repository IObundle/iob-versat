#include "versat.hpp"
#include "printf.h"

#include "versat_data.inc"

#define MEMSET(base, location, value) (*((volatile int*) (base + (sizeof(int)) * location)) = value)
#define MEMGET(base, location)        (*((volatile int*) (base + (sizeof(int)) * location)))

static int versat_base;

extern int* number_versat_configurations;
extern int  versat_configurations;

void InitVersat(Versat* versat,int base,int numberConfigurations){
   printf("Embedded Versat\n");

   versat_base = base;

   MEMSET(versat_base,0x0,2);
   MEMSET(versat_base,0x0,0);
}

FUDeclaration* RegisterFU(Versat* versat,FUDeclaration decl){
   static FUDeclaration decls[100];
   static int index;

   FUDeclaration* res = &decls[index];
   decls[index++] = decl;

   return res;
}

// Accelerator functions
Accelerator* CreateAccelerator(Versat* versat){
   return nullptr;
}

FUInstance* CreateFUInstance(Accelerator* accel,FUDeclaration* decl){
   static int instancesCreated = 0;

   FUInstance* inst = &instancesBuffer[instancesCreated++];

   return inst;
}

void AcceleratorRun(Accelerator* accel){
   MEMSET(versat_base,0x0,1);

   while(1){
      int val = MEMGET(versat_base,0x0);

      if(val)
         break;
   }
}

void VersatUnitWrite(FUInstance* instance,int address, int value){
   instance->memMapped[address] = value;
}

int32_t VersatUnitRead(FUInstance* instance,int address){
   int32_t res = instance->memMapped[address];

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
         ptr += vsnprintf(ptr,ptr - buffer,str,args);
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
   //printf("%s\n",buffer);
   FUInstance* res = nullptr;

   for(int i = 0; i < ARRAY_SIZE(instancesBuffer); i++){
      printf("%d %d %s\n",i,ARRAY_SIZE(instancesBuffer),buffer);
      if(strcmp(instancesBuffer[i].name,buffer) == 0){
         res = &instancesBuffer[i];
         break;
      }
   }

   if(res == nullptr){
      printf("Didn't find: %s\n",buffer);
   }

   va_end(args);

   return res;
}

// In versat space, simple extract delays from configuration data
void CalculateDelay(Versat* versat,Accelerator* accel){
   for(int i = 0; i < ARRAY_SIZE(delayBuffer); i++){
      delayBase[i] = delayBuffer[i];
   }
}

SizedString MakeSizedString(const char* str, size_t size){
   SizedString s = {};

   if(size == 0){
      size = strlen(str);
   }

   s.str = str;
   s.size = size;
   return s;
}

FUInstance* CreateNamedFUInstance(Accelerator* accel,FUDeclaration* type,SizedString entityName){
   FUInstance* res = GetInstanceByName_(accel,1,entityName.str);

   return res;
}

void RegisterTypes(){
}

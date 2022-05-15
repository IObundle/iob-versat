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
      if(strcmp(instancesBuffer[i].name.str,buffer) == 0){
         res = &instancesBuffer[i];
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
   for(int i = 0; i < ARRAY_SIZE(instancesBuffer); i++){
      FUInstance* inst = &instancesBuffer[i];

      printf("%d %x\n",i,(int)inst->delay);

      inst->delay[0] = inst->baseDelay;
   }
}

// From this point on, empty functions so the code still compiles
void OutputVersatSource(Versat* versat,Accelerator* accel,const char* declarationFilepath,const char* sourceFilepath,const char* configurationFilepath){

}

void OutputMemoryMap(Versat* versat){

}

void SaveConfiguration(Accelerator* accel,int configuration){

}

void LoadConfiguration(Accelerator* accel,int configuration){

}

FUDeclaration* RegisterAdd(Versat* versat){
   return nullptr;
}
FUDeclaration* RegisterReg(Versat* versat){
   return nullptr;
}
FUDeclaration* RegisterVRead(Versat* versat){
   return nullptr;
}
FUDeclaration* RegisterVWrite(Versat* versat){
   return nullptr;
}
FUDeclaration* RegisterMem(Versat* versat,int n){
   return nullptr;
}
FUDeclaration* RegisterDebug(Versat* versat){
   return nullptr;
}
FUDeclaration* RegisterConst(Versat* versat){
   return nullptr;
}

void ConnectUnits(FUInstance* out,int outIndex,FUInstance* in,int inIndex){
}

Accelerator* Flatten(Versat* versat,Accelerator* accel,int times){
   return nullptr;
}

void OutputVersatSource(Versat* versat,Accelerator* accel,const char* sourceFilepath){
}

void RemoveFUInstance(Accelerator* accel,FUInstance* inst){
}

FUDeclaration* GetTypeByName(Versat* versat,SizedString str){
   return nullptr;
}

int GetPageSize(){
   return 4096;
}

void* AllocatePages(int pages){
   return nullptr;
}

void DeallocatePages(void* ptr,int pages){
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

void FixedStringCpy(char* dest,SizedString src){
}

int32_t GetInputValue(FUInstance* instance,int port){
   return 0;
}

int mini(int a1,int a2){
   return 0;
}

int maxi(int a1,int a2){
   return 0;
}

int RoundUpDiv(int dividend,int divisor){
   return 1;
}

int AlignNextPower2(int val){
   return 1;
}

void FlushStdout(){
}

FUInstance* CreateNamedFUInstance(Accelerator* accel,FUDeclaration* type,SizedString entityName,HierarchyName* hierarchyParent){
   FUInstance* res = GetInstanceByName_(accel,1,entityName.str);

   return res;
}

void RegisterTypes(){
}

/*

Embedded

Disabled (empty):

   Anything related to type info can be disabled (but it can be enabled, no problem, nothing really depends on it though)
   Register functions can be disabled , I think
   GetTypeByName - we do not need to get types, I think
   ConnectUnits - do not need

Enabled:

   InitVersat - memory address needed only.
   CreateAccelerator - must return some data that separates all the following instance functions from different accelerators (an array of instance info)
   CreateInstance - only needs to returns instance. Either gets info by name or by order of creation (but in that case order of creation must be kept constant)
   GetInstanceByName - absolutely needed
   VersatUnitWrite - memory write
   VersatUnitRead - memory read
   CalculateDelay - In embedded only need to set delay (write to registers). Delay data should be pre computed (handle only 1 configuration for now)
   AcceleratorRun - write to register, start accelerator









*/


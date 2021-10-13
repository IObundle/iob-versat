#include "versat.h"

void InitVersat(Versat* versat){

}

FU_Type RegisterFU(Versat* versat,const char* declarationName,int extraBytes,int configBytes,int memoryMapBytes,bool doesIO,FUFunction startFunction,FUFunction updateFunction){
   FU_Type type={};

   return type;
}

void OutputVersatSource(Versat* versat,const char* declarationFilepath,const char* sourceFilepath){

}

// Accelerator functions
Accelerator* CreateAccelerator(Versat* versat){
   return (Accelerator*) 0;
}

FUInstance* CreateFUInstance(Accelerator* accel,FU_Type type){
   return (FUInstance*) 0;
}

void AcceleratorRun(Versat* versat,Accelerator* accel,FUInstance* endRoot,FUFunction terminateFunction){

}
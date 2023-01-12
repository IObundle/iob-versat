#include "versatExtra.hpp"

#include "versatPrivate.hpp"

static const char* constIn[] = {"constIn0","constIn1","constIn2","constIn3","constIn4","constIn5","constIn6","constIn7","constIn8","constIn9","constIn10","constIn11","constIn12","constIn13","constIn14","constIn15","constIn16",
                       "constIn17","constIn18","constIn19","constIn20","constIn21","constIn22","constIn23","constIn24","constIn25","constIn26","constIn27","constIn28","constIn29","constIn30","constIn31","constIn32",};
static const char* regOut[] = {"regOut0","regOut1","regOut2","regOut3","regOut4","regOut5","regOut6","regOut7","regOut8","regOut9","regOut10","regOut11","regOut12","regOut13","regOut14","regOut15","regOut16","regOut17"};

bool InitSimpleAccelerator(SimpleAccelerator* simple,Versat* versat,const char* declarationName){
   if(simple->init){
      return false;
   }

   FUDeclaration* type = GetTypeByName(versat,MakeSizedString(declarationName));

   simple->accel = CreateAccelerator(versat);
   simple->inst = CreateFUInstance(simple->accel,type,MakeSizedString("Test"));

   simple->numberInputs = type->inputDelays.size;
   simple->numberOutputs = type->outputLatencies.size;

   FUDeclaration* constType = GetTypeByName(versat,MakeSizedString("Const"));
   FUDeclaration* regType = GetTypeByName(versat,MakeSizedString("Reg"));

   for(unsigned int i = 0; i < simple->numberInputs; i++){
      Assert(i < ARRAY_SIZE(constIn));

      SizedString name = MakeSizedString(constIn[i]);
      simple->inputs[i] = CreateFUInstance(simple->accel,constType,name);
      ConnectUnits(simple->inputs[i],0,simple->inst,i);
   }

   for(unsigned int i = 0; i < simple->numberOutputs; i++){
      Assert(i < ARRAY_SIZE(regOut));

      SizedString name = MakeSizedString(regOut[i]);
      simple->outputs[i] = CreateFUInstance(simple->accel,regType,name);

      ConnectUnits(simple->inst,i,simple->outputs[i],0);
   }

   return true;
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

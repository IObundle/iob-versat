#include "versatExtra.hpp"

#include "templateEngine.hpp"
#include "versatPrivate.hpp"
#include "type.hpp"

static const char* constIn[] = {"constIn0","constIn1","constIn2","constIn3","constIn4","constIn5","constIn6","constIn7","constIn8","constIn9","constIn10","constIn11","constIn12","constIn13","constIn14","constIn15","constIn16",
                       "constIn17","constIn18","constIn19","constIn20","constIn21","constIn22","constIn23","constIn24","constIn25","constIn26","constIn27","constIn28","constIn29","constIn30","constIn31","constIn32",};
static const char* regOut[] = {"regOut0","regOut1","regOut2","regOut3","regOut4","regOut5","regOut6","regOut7","regOut8","regOut9","regOut10","regOut11","regOut12","regOut13","regOut14","regOut15","regOut16","regOut17"};

bool InitSimpleAccelerator(SimpleAccelerator* simple,Versat* versat,const char* declarationName){
   if(simple->init){
      return false;
   }

   FUDeclaration* type = GetTypeByName(versat,STRING(declarationName));

   simple->decl = type;
   simple->accel = CreateAccelerator(versat);
   simple->inst = CreateFUInstance(simple->accel,type,STRING("Test"));

   simple->numberInputs = type->inputDelays.size;
   simple->numberOutputs = type->outputLatencies.size;

   FUDeclaration* constType = GetTypeByName(versat,STRING("Const"));
   FUDeclaration* regType = GetTypeByName(versat,STRING("Reg"));

   for(unsigned int i = 0; i < simple->numberInputs; i++){
      Assert(i < ARRAY_SIZE(constIn));

      String name = STRING(constIn[i]);
      simple->inputs[i] = CreateFUInstance(simple->accel,constType,name);
      ConnectUnits(simple->inputs[i],0,simple->inst,i);
   }

   for(unsigned int i = 0; i < simple->numberOutputs; i++){
      Assert(i < ARRAY_SIZE(regOut));

      String name = STRING(regOut[i]);
      simple->outputs[i] = CreateFUInstance(simple->accel,regType,name);

      ConnectUnits(simple->inst,i,simple->outputs[i],0);
   }

   return true;
}

void RemapSimpleAccelerator(SimpleAccelerator* simple,Versat* versat){
   for(unsigned int i = 0; i < simple->numberInputs; i++){
      Assert(i < ARRAY_SIZE(constIn));

      String name = STRING(constIn[i]);
      simple->inputs[i] = GetInstanceByName(simple->accel,name);
   }

   for(unsigned int i = 0; i < simple->numberOutputs; i++){
      Assert(i < ARRAY_SIZE(regOut));

      String name = STRING(regOut[i]);
      simple->outputs[i] = GetInstanceByName(simple->accel,name);
   }
}

void OutputVersatSource(Versat* versat,SimpleAccelerator* simpleAccel,const char* sourceFilepath,const char* constantsFilepath,const char* dataFilepath){
   if(!versat->debug.outputVersat){
      return;
   }

   Accelerator* accel = simpleAccel->accel;

   Arena* arena = &versat->temp;
   ArenaMarker marker(arena);

   //AcceleratorView view = CreateAcceleratorView(accel,arena);
   //view.CalculateVersatData(arena);

   SetDelayRecursive(accel);

   #if 1
   // No need for templating, small file
   FILE* c = OpenFileAndCreateDirectories(constantsFilepath,"w");

   if(!c){
      printf("Error creating file, check if filepath is correct: %s\n",constantsFilepath);
      return;
   }

   VersatComputedValues val = ComputeVersatValues(versat,accel);

   #if 1
   std::vector<ComplexFUInstance*> accum;
   AcceleratorIterator iter = {};
   for(InstanceNode* node = iter.Start(accel,&versat->temp); node; node = iter.Next()){
      ComplexFUInstance* inst = node->inst;
      if(!inst->declaration->isOperation && inst->declaration->type != FUDeclaration::SPECIAL){
         accum.push_back(inst);
      }
   };
   #endif

   fprintf(c,"`define NUMBER_UNITS %d\n",Size(accel->allocated));
   fprintf(c,"`define CONFIG_W %d\n",val.configurationBits);
   fprintf(c,"`define STATE_W %d\n",val.stateBits);
   fprintf(c,"`define MAPPED_UNITS %d\n",val.unitsMapped);
   fprintf(c,"`define MAPPED_BIT %d\n",val.memoryConfigDecisionBit);
   fprintf(c,"`define nIO %d\n",val.nUnitsIO);

   if(val.nUnitsIO){
      fprintf(c,"`define VERSAT_IO\n");
   }

   fclose(c);

   FILE* s = OpenFileAndCreateDirectories(sourceFilepath,"w");

   if(!s){
      printf("Error creating file, check if filepath is correct: %s\n",sourceFilepath);
      return;
   }

   FILE* d = OpenFileAndCreateDirectories(dataFilepath,"w");

   if(!d){
      fclose(s);
      printf("Error creating file, check if filepath is correct: %s\n",dataFilepath);
      return;
   }

   TemplateSetNumber("numberUnits",Size(accel->allocated));
   TemplateSetCustom("versatValues",&val,"VersatComputedValues");
   TemplateSetCustom("versat",versat,"Versat");
   TemplateSetCustom("accel",accel,"Accelerator");
   TemplateSetNumber("nStatics",val.nStatics);
   TemplateSetNumber("nDelays",val.nDelays);
   TemplateSetNumber("delayStart",val.delayBitsStart);

   // Output configuration file
   int size = Size(accel->allocated);
   Array<InstanceNode*> nodes = PushArray<InstanceNode*>(arena,size);
   int i = 0;
   FOREACH_LIST_INDEXED(ptr,accel->ordered,i){
      nodes[i] = ptr->node;
   }

   TemplateSetCustom("instances",&nodes,"Array<InstanceNode*>");

   TemplateSetNumber("versatBase",versat->base);
   TemplateSetNumber("memoryAddressBits",val.memoryAddressBits);
   TemplateSetNumber("unitsMapped",val.unitsMapped);
   TemplateSetNumber("memoryConfigDecisionBit",val.memoryConfigDecisionBit);
   TemplateSetNumber("configurationBits",val.configurationBits);
   TemplateSetNumber("memoryMappedBase",1 << val.memoryConfigDecisionBit);

   ProcessTemplate(s,BasicTemplates::topAcceleratorTemplate,&versat->temp);

   TemplateSetBool("IsSimple",true);
   TemplateSetNumber("simpleInputs",simpleAccel->numberInputs);
   TemplateSetNumber("simpleOutputs",simpleAccel->numberOutputs);

   int inputStart = CountNonOperationChilds(simpleAccel->inst->declaration->fixedDelayCircuit);
   TemplateSetNumber("inputStart",inputStart + 1);
   TemplateSetNumber("outputStart",inputStart + simpleAccel->numberInputs + 1);

   TemplateSetNumber("configurationSize",accel->configAlloc.size);
   TemplateSetArray("configurationData","int",accel->configAlloc.ptr,accel->configAlloc.size);

   TemplateSet("config",accel->configAlloc.ptr);
   TemplateSet("state",accel->stateAlloc.ptr);
   TemplateSet("memMapped",nullptr);
   TemplateSet("static",accel->staticAlloc.ptr);
   TemplateSet("staticEnd",&accel->staticAlloc.ptr[accel->staticAlloc.reserved]);

   TemplateSetArray("delay","int",accel->delayAlloc.ptr,accel->delayAlloc.size);
   TemplateSetArray("staticBuffer","int",accel->staticAlloc.ptr,accel->staticAlloc.size);

   TemplateSetNumber("memoryAddressBits",val.memoryAddressBits);
   TemplateSetNumber("memoryConfigDecisionBit",val.memoryConfigDecisionBit);
   TemplateSetNumber("versatBase",versat->base);
   TemplateSetNumber("memoryMappedBase",1 << val.memoryConfigDecisionBit);
   TemplateSetNumber("nConfigs",val.nConfigs);
   TemplateSetNumber("nDelays",val.nDelays);
   TemplateSetNumber("nStatics",val.nStatics);
   TemplateSetCustom("instances",&accum,"std::vector<ComplexFUInstance*>");
   TemplateSetNumber("numberUnits",accum.size());
   ProcessTemplate(d,BasicTemplates::dataTemplate,&versat->temp);

   //PopMark(&versat->temp,mark);

   fclose(s);
   fclose(d);
   #endif
}

int* vRunSimpleAccelerator(SimpleAccelerator* simple,bool debug,va_list args){
   static int out[99];

   for(unsigned int i = 0; i < simple->numberInputs; i++){
      int val = va_arg(args,int);
      simple->inputs[i]->config[0] = val;
   }

   if(debug){
      //AcceleratorRunDebug(simple->accel);
   } else {
      AcceleratorRun(simple->accel);
   }

   for(unsigned int i = 0; i < simple->numberOutputs; i++){
      out[i] = simple->outputs[i]->state[0];
   }

   return out;
}

int* RunSimpleAccelerator(SimpleAccelerator* simple, ...){
   va_list args;
   va_start(args,simple);

   int* out = vRunSimpleAccelerator(simple,false,args);

   va_end(args);

   return out;
}

int* RunSimpleAcceleratorDebug(SimpleAccelerator* simple, ...){
   va_list args;
   va_start(args,simple);

   int* out = vRunSimpleAccelerator(simple,true,args);

   va_end(args);

   return out;
}

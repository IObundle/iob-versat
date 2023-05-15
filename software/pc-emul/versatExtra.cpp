#include "versatExtra.hpp"

#include "templateEngine.hpp"
#include "versatPrivate.hpp"
#include "type.hpp"

static const char* constIn[] = {"constIn0","constIn1","constIn2","constIn3","constIn4","constIn5","constIn6","constIn7","constIn8","constIn9","constIn10","constIn11","constIn12","constIn13","constIn14","constIn15","constIn16",
                       "constIn17","constIn18","constIn19","constIn20","constIn21","constIn22","constIn23","constIn24","constIn25","constIn26","constIn27","constIn28","constIn29","constIn30","constIn31","constIn32","constIn33",
                       "constIn34","constIn35","constIn36","constIn37","constIn38","constIn39","constIn40","constIn41","constIn42","constIn43","constIn44"};
static const char* regOut[] = {"regOut0","regOut1","regOut2","regOut3","regOut4","regOut5","regOut6","regOut7","regOut8","regOut9","regOut10","regOut11","regOut12","regOut13","regOut14","regOut15","regOut16","regOut17","regOut18",
                       "regOut19","regOut20","regOut21","regOut22","regOut23","regOut24","regOut25","regOut26","regOut27","regOut28","regOut29","regOut30","regOut31","regOut32","regOut33","regOut34","regOut35","regOut36","regOut37",
                       "regOut38","regOut39","regOut40","regOut41","regOut42","regOut43","regOut44"};

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

   FUDeclaration* constType = GetTypeByName(versat,STRING("TestConst")); // TODO: Used to be TestConst, but using Const while testing Iterative units
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

#if 1
void OutputVersatSource(Versat* versat,SimpleAccelerator* simpleAccel,const char* directoryPath){
   if(!versat->debug.outputVersat){
      return;
   }

   Accelerator* accel = simpleAccel->accel;

   Arena* arena = &versat->temp;
   ArenaMarker marker(arena);

   SetDelayRecursive(accel,arena);

   #if 1
   // No need for templating, small file
   FILE* c = OpenFileAndCreateDirectories(StaticFormat("%s/versat_defs.vh",directoryPath),"w");

   if(!c){
      printf("Error creating file, check if filepath is correct: %s\n",directoryPath);
      return;
   }

   VersatComputedValues val = ComputeVersatValues(versat,accel);

   #if 1
   std::vector<ComplexFUInstance> accum;
   AcceleratorIterator iter = {};
   for(InstanceNode* node = iter.Start(accel,arena,true); node; node = iter.Next()){
      ComplexFUInstance* inst = node->inst;
      if(!inst->declaration->isOperation && inst->declaration->type != FUDeclaration::SPECIAL){
         accum.push_back(*inst);
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

   if(val.externalMemoryInterfaces){
      fprintf(c,"`define VERSAT_EXTERNAL_MEMORY\n");
   }

   fclose(c);

   FILE* s = OpenFileAndCreateDirectories(StaticFormat("%s/versat_instance.v",directoryPath),"w");

   if(!s){
      printf("Error creating file, check if filepath is correct: %s\n",directoryPath);
      return;
   }

   FILE* d = OpenFileAndCreateDirectories(StaticFormat("%s/versat_data.inc",directoryPath),"w");

   if(!d){
      fclose(s);
      printf("Error creating file, check if filepath is correct: %s\n",directoryPath);
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
   FOREACH_LIST_INDEXED(ptr,accel->allocated,i){
      nodes[i] = ptr;
   }

   ComputedData computedData = CalculateVersatComputedData(accel->allocated,val,arena);

   TemplateSetCustom("versatData",&computedData.data,"Array<VersatComputedData>");
   TemplateSetCustom("external",&computedData.external,"Array<ExternalMemoryInterface>");
   TemplateSetCustom("instances",&nodes,"Array<InstanceNode*>");

   int staticStart = 0;
   FOREACH_LIST(ptr,accel->allocated){
      FUDeclaration* decl = ptr->inst->declaration;
      for(Wire& wire : decl->configs){
         staticStart += wire.bitsize;
      }
   }

   Hashmap<StaticId,StaticData>* staticUnits = PushHashmap<StaticId,StaticData>(arena,accel->staticUnits.size());
   for(InstanceNode* node = iter.Start(accel,arena,true); node; node = iter.Next()){
      if(node->inst->isStatic){
         InstanceNode* parent = iter.ParentInstance();

         StaticId id = {};
         id.parent = parent ? parent->inst->declaration : nullptr;
         id.name = node->inst->name;

         StaticData data = {};
         data.configs = node->inst->declaration->configs;
         data.offset = node->inst->config - accel->staticAlloc.ptr;

         staticUnits->Insert(id,data);
      }
   }
   TemplateSetCustom("staticUnits",staticUnits,"Hashmap<StaticId,StaticData>");

   TemplateSetNumber("staticStart",staticStart);
   TemplateSetNumber("versatBase",versat->base);
   TemplateSetNumber("memoryAddressBits",val.memoryAddressBits);
   TemplateSetNumber("unitsMapped",val.unitsMapped);
   TemplateSetNumber("memoryConfigDecisionBit",val.memoryConfigDecisionBit);
   TemplateSetNumber("configurationBits",val.configurationBits);
   TemplateSetNumber("memoryMappedBase",1 << val.memoryConfigDecisionBit);
   TemplateSetNumber("versatConfig",val.versatConfigs);
   TemplateSetNumber("versatState",val.versatStates);

   ProcessTemplate(s,BasicTemplates::topAcceleratorTemplate,&versat->temp);

   TemplateSetNumber("configurationSize",accel->configAlloc.size);
   TemplateSetArray("configurationData","int",accel->configAlloc.ptr,accel->configAlloc.size);

   TemplateSetBool("IsSimple",true);
   TemplateSetNumber("simpleInputs",simpleAccel->numberInputs);
   TemplateSetNumber("simpleOutputs",simpleAccel->numberOutputs);

   int inputStart = CountNonOperationChilds(simpleAccel->inst->declaration->fixedDelayCircuit);
   TemplateSetNumber("inputStart",inputStart + 1);
   TemplateSetNumber("outputStart",inputStart + simpleAccel->numberInputs + 1);

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
   TemplateSetCustom("instances",&accum,"std::vector<ComplexFUInstance>");
   TemplateSetNumber("numberUnits",accum.size());
   TemplateSetNumber("versatConfig",val.versatConfigs);
   TemplateSetNumber("versatState",val.versatStates);
   ProcessTemplate(d,BasicTemplates::dataTemplate,&versat->temp);

   {
   FILE* f = OpenFileAndCreateDirectories(StaticFormat("%s/versat_external_memory_inst.vh",directoryPath),"w");
   TemplateSetCustom("external",&computedData.external,"Array<ExternalMemoryInterface>");
   ProcessTemplate(f,BasicTemplates::externalInstTemplate,&versat->temp);
   fclose(f);
   }

   {
   FILE* f = OpenFileAndCreateDirectories(StaticFormat("%s/versat_external_memory_port.vh",directoryPath),"w");
   TemplateSetCustom("external",&computedData.external,"Array<ExternalMemoryInterface>");
   ProcessTemplate(f,BasicTemplates::externalPortTemplate,&versat->temp);
   fclose(f);
   }

   {
   FILE* f = OpenFileAndCreateDirectories(StaticFormat("%s/versat_external_memory_portmap.vh",directoryPath),"w");
   TemplateSetCustom("external",&computedData.external,"Array<ExternalMemoryInterface>");
   ProcessTemplate(f,BasicTemplates::externalPortmapTemplate,&versat->temp);
   fclose(f);
   }
   //PopMark(&versat->temp,mark);

   fclose(s);
   fclose(d);
   #endif
}

#endif

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

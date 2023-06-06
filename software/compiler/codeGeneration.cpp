#include "versatPrivate.hpp"

#include "templateEngine.hpp"

#include "debug.hpp"
#include "debugGUI.hpp"

void TemplateSetDefaults(Versat* versat){
   TemplateSetCustom("versat",versat,"Versat");
   TemplateSetCustom("inputDecl",BasicDeclaration::input,"FUDeclaration");
   TemplateSetCustom("outputDecl",BasicDeclaration::output,"FUDeclaration");
}

void OutputCircuitSource(Versat* versat,FUDeclaration* decl,Accelerator* accel,FILE* file){
   Assert(versat->debug.outputAccelerator); // Because FILE is created outside, code should not call this function if flag is set

   Arena* arena = &versat->temp;
   ArenaMarker marker(arena);

   VersatComputedValues val = ComputeVersatValues(versat,accel);

   // Output configuration file
   int nonSpecialUnits = CountNonSpecialChilds(accel->allocated);

   int size = Size(accel->allocated);
   Array<InstanceNode*> nodes = ListToArray(accel->allocated,size,arena);

   Array<InstanceNode*> ordered = PushArray<InstanceNode*>(arena,size);
   int i = 0;
   FOREACH_LIST_INDEXED(ptr,accel->ordered,i){
      ordered[i] = ptr->node;
   }

   ComputedData computedData = CalculateVersatComputedData(accel->allocated,val,arena);

   TemplateSetDefaults(versat);
   TemplateSetCustom("accel",decl,"FUDeclaration");
   TemplateSetCustom("versatValues",&val,"VersatComputedValues");
   TemplateSetCustom("staticUnits",decl->staticUnits,"Hashmap<StaticId,StaticData>");
   TemplateSetCustom("versatData",&computedData.data,"Array<VersatComputedData>");
   TemplateSetCustom("external",&computedData.external,"Array<ExternalMemoryInterface>");
   TemplateSetCustom("instances",&nodes,"Array<InstanceNode*>");
   TemplateSetCustom("ordered",&ordered,"Array<InstanceNode*>");
   TemplateSetNumber("numberUnits",Size(accel->allocated));
   TemplateSetNumber("nonSpecialUnits",nonSpecialUnits);
   TemplateSetNumber("unitsMapped",val.unitsMapped);
   TemplateSetNumber("configurationBits",val.configurationBits);
   TemplateSetNumber("memoryAddressBits",val.memoryAddressBits);
   TemplateSetNumber("memoryConfigDecisionBit",val.memoryConfigDecisionBit);

   ProcessTemplate(file,BasicTemplates::acceleratorTemplate,&versat->temp);
}

void OutputVersatSource(Versat* versat,Accelerator* accel,const char* directoryPath,String accelName,bool isSimple){
   if(!versat->debug.outputVersat){
      return;
   }

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

   UnitValues unit = CalculateAcceleratorValues(versat,accel);

   fprintf(c,"`define NUMBER_UNITS %d\n",Size(accel->allocated));
   fprintf(c,"`define CONFIG_W %d\n",val.configurationBits);
   fprintf(c,"`define STATE_W %d\n",val.stateBits);
   fprintf(c,"`define MAPPED_UNITS %d\n",val.unitsMapped);
   fprintf(c,"`define MAPPED_BIT %d\n",val.memoryConfigDecisionBit);
   fprintf(c,"`define nIO %d\n",val.nUnitsIO);

   if(unit.inputs || unit.outputs){
      fprintf(c,"`define EXTERNAL_PORTS\n");
   }

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

   // Output configuration file
   int size = Size(accel->allocated);
   Array<InstanceNode*> nodes = ListToArray(accel->allocated,size,arena);

   ReorganizeAccelerator(accel,arena);
   Array<InstanceNode*> ordered = PushArray<InstanceNode*>(arena,size);
   int i = 0;
   FOREACH_LIST_INDEXED(ptr,accel->ordered,i){
      ordered[i] = ptr->node;
   }

   ComputedData computedData = CalculateVersatComputedData(accel->allocated,val,arena);

   TemplateSetDefaults(versat);

   TemplateSetNumber("nInputs",unit.inputs);
   TemplateSetNumber("nOutputs",unit.outputs);
   TemplateSetCustom("ordered",&ordered,"Array<InstanceNode*>");
   TemplateSetCustom("versatValues",&val,"VersatComputedValues");
   TemplateSetNumber("delayStart",val.delayBitsStart);
   TemplateSetCustom("versatData",&computedData.data,"Array<VersatComputedData>");
   TemplateSetCustom("external",&computedData.external,"Array<ExternalMemoryInterface>");
   TemplateSetCustom("instances",&nodes,"Array<InstanceNode*>");
   TemplateSetNumber("nIO",val.nUnitsIO);

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
   TemplateSetNumber("unitsMapped",val.unitsMapped);
   TemplateSetNumber("memoryConfigDecisionBit",val.memoryConfigDecisionBit);
   TemplateSetNumber("configurationBits",val.configurationBits);
   TemplateSetNumber("versatConfig",val.versatConfigs);
   TemplateSetNumber("versatState",val.versatStates);

   ProcessTemplate(s,BasicTemplates::topAcceleratorTemplate,&versat->temp);

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
   TemplateSetNumber("versatConfig",val.versatConfigs);
   TemplateSetNumber("versatState",val.versatStates);
   TemplateSetNumber("nStatics",val.nStatics);
   TemplateSetCustom("instances",&accum,"std::vector<ComplexFUInstance>");
   TemplateSetNumber("numberUnits",accum.size());
   TemplateSetBool("IsSimple",false);
   ProcessTemplate(d,BasicTemplates::dataTemplate,&versat->temp);

   {
   Hashmap<String,SizedConfig>* namedConfigs = ExtractNamedSingleConfigs(accel,arena);
   Hashmap<String,SizedConfig>* namedStates = ExtractNamedSingleStates(accel,arena);
   Hashmap<String,SizedConfig>* namedMem = ExtractNamedSingleMem(accel,arena);

   TemplateSetCustom("namedConfigs",namedConfigs,"Hashmap<String,SizedConfig>");
   TemplateSetCustom("namedStates",namedStates,"Hashmap<String,SizedConfig>");
   TemplateSetCustom("namedMem",namedMem,"Hashmap<String,SizedConfig>");
   TemplateSetString("accelType",accelName);
   TemplateSetBool("isSimple",isSimple);

   TemplateSetArray("delay","int",accel->delayAlloc.ptr,accel->delayAlloc.size);
   TemplateSetArray("staticBuffer","int",accel->staticAlloc.ptr,accel->staticAlloc.size);

   TemplateSetNumber("versatBase",versat->base);
   TemplateSetNumber("nStatics",val.nStatics);
   TemplateSetNumber("nConfigs",val.nConfigs);
   TemplateSetNumber("nDelays",val.nDelays);
   TemplateSetNumber("versatConfig",val.versatConfigs);
   TemplateSetNumber("versatState",val.versatStates);
   TemplateSetNumber("memoryMappedBase",1 << val.memoryConfigDecisionBit);

   FILE* f = OpenFileAndCreateDirectories(StaticFormat("%s/accel.hpp",directoryPath),"w");
   ProcessTemplate(f,BasicTemplates::acceleratorHeaderTemplate,&versat->temp);
   fclose(f);
   }

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

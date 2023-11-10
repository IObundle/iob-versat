#include "configurations.hpp"
#include "declaration.hpp"
#include "memory.hpp"
#include "type.hpp"
#include "utilsCore.hpp"
#include "versat.hpp"

#include "templateEngine.hpp"

#include "debug.hpp"
#include "debugGUI.hpp"

void TemplateSetDefaults(Versat* versat){
   TemplateSetCustom("versat",versat,"Versat");
   TemplateSetCustom("inputDecl",BasicDeclaration::input,"FUDeclaration");
   TemplateSetCustom("outputDecl",BasicDeclaration::output,"FUDeclaration");
   TemplateSetCustom("arch",&versat->opts,"Options");
}

void OutputCircuitSource(Versat* versat,FUDeclaration* decl,Accelerator* accel,FILE* file){
   Assert(versat->debug.outputAccelerator); // Because FILE is created outside, code should not call this function if flag is set

   Arena* arena = &versat->temp;

   BLOCK_REGION(arena);

   VersatComputedValues val = ComputeVersatValues(versat,accel);

   // Output configuration file
   int nonSpecialUnits = CountNonSpecialChilds(accel->allocated);

   int size = Size(accel->allocated);
   Array<InstanceNode*> nodes = ListToArray(accel->allocated,size,arena);

   Array<InstanceNode*> ordered = PushArray<InstanceNode*>(arena,size);
   int i = 0;
   FOREACH_LIST_INDEXED(OrderedInstance*,ptr,accel->ordered,i){
      ordered[i] = ptr->node;

      FUInstance* inst = ptr->node->inst;
      if(inst->declaration->nIOs){
         inst->parameters = STRING("#(.AXI_ADDR_W(AXI_ADDR_W),.AXI_DATA_W(AXI_DATA_W),.LEN_W(LEN_W))"); // TODO: placeholder hack.
      }
   }

   {
      iptr memoryPos = 0;
      FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
         FUInstance* inst = ptr->inst;
         FUDeclaration* decl = inst->declaration;

         if(decl->isMemoryMapped) {
            memoryPos = AlignBitBoundary(memoryPos,decl->memoryMapBits);
            inst->memMapped = (int*) memoryPos;
            memoryPos += 1 << decl->memoryMapBits;
         }
      }
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

Array<FUDeclaration*> SortTypesByDependency(Array<FUDeclaration*> types,Arena* out,Arena* temp){
  BLOCK_REGION(temp);

  int size = types.size;

  int stored = 0;
  Array<FUDeclaration*> result = PushArray<FUDeclaration*>(out,size);

  Hashmap<FUDeclaration*,bool>* seen = PushHashmap<FUDeclaration*,bool>(temp,size);
  Array<Array<FUDeclaration*>> subTypes = PushArray<Array<FUDeclaration*>>(temp,size);
  Memset(subTypes,{});

  for(int i = 0; i < size; i++){
    subTypes[i] = SubTypes(types[i],temp,out);
    seen->Insert(types[i],false);
  }

  for(int iter = 0; iter < size; iter++){
    bool breakEarly = true;
    for(int i = 0; i < size; i++){
      FUDeclaration* type = types[i];
      Array<FUDeclaration*> sub = subTypes[i];

      bool* seenType = seen->Get(type);

      if(seenType && *seenType){
        continue;
      }

      bool allSeen = true;
      for(FUDeclaration* subIter : sub){
        bool* res = seen->Get(subIter);

        if(res && !*res){
          allSeen = false;
          break;
        }
      }

      if(allSeen){
        *seenType = true;
        result[stored++] = types[i];
        breakEarly = false;
      }
    }

    if(breakEarly){
      break;
    }
  }

  return result;
}

Array<TypeStructInfoElement> GenerateStructFromType(FUDeclaration* decl,Arena* arena){
   if(decl->type == FUDeclaration::SPECIAL){
     return {};
   }

   if(decl->type == FUDeclaration::SINGLE){
     int size = decl->configInfo.configs.size;
     Array<TypeStructInfoElement> entries = PushArray<TypeStructInfoElement>(arena,size);

     for(int i = 0; i < size; i++){
       entries[i].type = STRING("iptr");
       entries[i].name = decl->configInfo.configs[i].name;
     }
     return entries;
   }

   int configSize = 0;
   FOREACH_LIST(InstanceNode*,node,decl->baseCircuit->allocated){
     FUDeclaration* decl = node->inst->declaration;

     if(!(ContainsConfigs(decl) || ContainsStatics(decl))){
       continue;
     }

     configSize += 1;
#if 0
     if(decl->type == FUDeclaration::SINGLE){
       configSize += decl->configInfo.configs.size;
     } else {
       configSize += 1;
     }
#endif
   }

   CalculatedOffsets& offsets = decl->configInfo.configOffsets;
   Array<TypeStructInfoElement> entries = PushArray<TypeStructInfoElement>(arena,configSize);

   int i = 0;
   int index = 0;
   FOREACH_LIST_INDEXED(InstanceNode*,node,decl->baseCircuit->allocated,i){
     FUDeclaration* decl = node->inst->declaration;

     if(!(ContainsConfigs(decl) || ContainsStatics(decl))){
       continue;
     }

#if 0
     if(decl->type == FUDeclaration::SINGLE){
       for(int i = 0; i < decl->configInfo.configs.size; i++){
         entries[index].type = STRING("iptr");
         entries[index].name = PushString(arena,"%.*s_%.*s",UNPACK_SS(node->inst->name),UNPACK_SS(decl->configInfo.configs[i].name));
         index += 1;
       }
     } else {
#endif       
       entries[index].type = PushString(arena,"%.*sConfig",UNPACK_SS(decl->name));
       entries[index].name = node->inst->name;
       index += 1;
#if 0
     }
#endif
     }

   return entries;
}

void OutputVersatSource(Versat* versat,Accelerator* accel,const char* hardwarePath,const char* softwarePath,String accelName,bool isSimple){
   if(!versat->debug.outputVersat){
      return;
   }

   Arena* arena = &versat->permanent;
   Arena* temp = &versat->temp;

   BLOCK_REGION(arena);
   BLOCK_REGION(temp);

   SetDelayRecursive(accel,arena);

   // No need for templating, small file
   FILE* c = OpenFileAndCreateDirectories(StaticFormat("%s/versat_defs.vh",hardwarePath),"w");

   if(!c){
      printf("Error creating file, check if filepath is correct: %s\n",hardwarePath);
      return;
   }

   VersatComputedValues val = ComputeVersatValues(versat,accel);

   //DebugValue(MakeValue(&val));
  
   std::vector<FUInstance> accum;
   AcceleratorIterator iter = {};
   for(InstanceNode* node = iter.Start(accel,arena,true); node; node = iter.Next()){
      FUInstance* inst = node->inst;
      if(!inst->declaration->isOperation && inst->declaration->type != FUDeclaration::SPECIAL){
         accum.push_back(*inst);
      }
   };

   UnitValues unit = CalculateAcceleratorValues(versat,accel);

   fprintf(c,"`define NUMBER_UNITS %d\n",Size(accel->allocated));
   fprintf(c,"`define CONFIG_W %d\n",val.configurationBits);
   fprintf(c,"`define STATE_W %d\n",val.stateBits);
   fprintf(c,"`define MAPPED_UNITS %d\n",val.unitsMapped);
   fprintf(c,"`define MAPPED_BIT %d\n",val.memoryConfigDecisionBit);
   fprintf(c,"`define nIO %d\n",val.nUnitsIO);
   fprintf(c,"`define LEN_W %d\n",20);

   if(versat->opts.architectureHasDatabus){
      fprintf(c,"`define VERSAT_ARCH_HAS_IO 1\n");
   }

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

   FILE* s = OpenFileAndCreateDirectories(StaticFormat("%s/versat_instance.v",hardwarePath),"w");

   if(!s){
      printf("Error creating file, check if filepath is correct: %s\n",hardwarePath);
      return;
   }

   // Output configuration file
   int size = Size(accel->allocated);
   Array<InstanceNode*> nodes = ListToArray(accel->allocated,size,arena);

   ReorganizeAccelerator(accel,arena);
   Array<InstanceNode*> ordered = PushArray<InstanceNode*>(arena,size);
   int i = 0;
   FOREACH_LIST_INDEXED(OrderedInstance*,ptr,accel->ordered,i){
      ordered[i] = ptr->node;

      FUInstance* inst = ptr->node->inst;
      if(inst->declaration->nIOs){
         inst->parameters = STRING("#(.AXI_ADDR_W(AXI_ADDR_W),.AXI_DATA_W(AXI_DATA_W),.LEN_W(LEN_W))"); // TODO: placeholder hack.
      }
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
   TemplateSetBool("isSimple",isSimple);

   int staticStart = 0;
   FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
      FUDeclaration* decl = ptr->inst->declaration;
      for(Wire& wire : decl->configInfo.configs){
         staticStart += wire.bitSize;
      }
   }

   Hashmap<StaticId,StaticData>* staticUnits = accel->calculatedStaticPos;

   TemplateSetCustom("staticUnits",staticUnits,"Hashmap<StaticId,StaticData>");
   TemplateSetArray("staticBuffer","iptr",accel->configAlloc.ptr + accel->startOfStatic,accel->staticSize);

   OrderedConfigurations orderedConfigs = ExtractOrderedConfigurationNames(versat,accel,&versat->permanent,&versat->temp);

   TemplateSetNumber("staticStart",staticStart);
   TemplateSetNumber("versatBase",versat->base);
   TemplateSetNumber("unitsMapped",val.unitsMapped);
   TemplateSetNumber("memoryConfigDecisionBit",val.memoryConfigDecisionBit);
   TemplateSetNumber("configurationBits",val.configurationBits);
   TemplateSetNumber("versatConfig",val.versatConfigs);
   TemplateSetNumber("versatState",val.versatStates);
   TemplateSetBool("useDMA",accel->useDMA);
   versat->opts.shadowRegister = true;
   TemplateSetCustom("opts",&versat->opts,"Options");

   ProcessTemplate(s,BasicTemplates::topAcceleratorTemplate,&versat->temp);

   TemplateSetNumber("configurationSize",accel->configAlloc.size);
   TemplateSetArray("configurationData","iptr",accel->configAlloc.ptr,accel->configAlloc.size);

   TemplateSet("static",accel->configAlloc.ptr + accel->startOfStatic);
   TemplateSetArray("delay","iptr",accel->configAlloc.ptr + accel->startOfDelay,val.nDelays);

   TemplateSet("config",accel->configAlloc.ptr);
   TemplateSet("state",accel->stateAlloc.ptr);
   TemplateSet("memMapped",nullptr);

   TemplateSetNumber("memoryAddressBits",val.memoryAddressBits);
   TemplateSetNumber("memoryMappedBase",1 << val.memoryConfigDecisionBit);
   TemplateSetNumber("nConfigs",val.nConfigs);
   TemplateSetNumber("nDelays",val.nDelays);
   TemplateSetNumber("nStatics",val.nStatics);
   TemplateSetCustom("instances",&accum,"std::vector<FUInstance>");
   TemplateSetNumber("numberUnits",accum.size());
   TemplateSetBool("IsSimple",false);

   Array<FUDeclaration*> typesUsed = {};
   region(temp){
     Set<FUDeclaration*>* maps = PushSet<FUDeclaration*>(temp,99);
     AcceleratorIterator iter = {};
     for(InstanceNode* node = iter.Start(accel,arena,false); node; node = iter.Next()){
       FUInstance* inst = node->inst;
       FUDeclaration* decl = inst->declaration;

       if(/*decl->type != FUDeclaration::SPECIAL && */ (ContainsConfigs(decl) || ContainsStatics(decl))){
         maps->Insert(decl);
       }
     }

     typesUsed = PushArrayFromSet(arena,maps);
     typesUsed = SortTypesByDependency(typesUsed,arena,temp);
   }

   Array<TypeStructInfo> structures = PushArray<TypeStructInfo>(arena,typesUsed.size);
   int index = 0;
   for(auto& decl : typesUsed){
     Array<TypeStructInfoElement> val = GenerateStructFromType(decl,arena);
     structures[index].name = decl->name;
     structures[index].entries = val;
     index += 1;
   }

   //DebugValue(MakeValue(&structures));
   TemplateSetCustom("structures",&structures,"Array<TypeStructInfo>");

   {
   Hashmap<String,SizedConfig>* namedConfigs = ExtractNamedSingleConfigs(accel,arena);
   Hashmap<String,SizedConfig>* namedStates = ExtractNamedSingleStates(accel,arena);
   Hashmap<String,SizedConfig>* namedMem = ExtractNamedSingleMem(accel,arena);

   TemplateSetBool("doingMerged",false);
#if 0
   {// TEMPORARY_MARK
   FUInstance* merged = nullptr;
   AcceleratorIterator iter = {};
   region(arena){
     for(InstanceNode* node = iter.Start(accel,arena,false); node; node = iter.Next()){
       FUInstance* inst = node->inst;
       FUDeclaration* decl = inst->declaration;
       if(decl->type == FUDeclaration::COMPOSITE){
         merged = inst;
         break;
       }
     }
   }

   /* What I have: I have the indexes where the given configuration values should reside inside an array.
      Basically for each unit inside the accelerator a map from unit to the position inside the struct where the
      values will reside.

      I also have the default configurations for the normal accelerators inside the configs.

    */

   if(merged){
     Array<Array<String>> mergedConfigs = PushArray<Array<String>>(arena,2); // TODO: Only using 2 for now.

     for(int i = 0; i < mergedConfigs.size; i++){
       MergeInfo& info = merged->declaration->mergeInfo[i];
       mergedConfigs[i] = PushArray<String>(arena,info.config.configOffsets.max);

       Memset(mergedConfigs[i],{});

       int index = 0;
       AcceleratorIterator iter = {};
       for(InstanceNode* node = iter.Start(info.baseType->baseCircuit,arena,false); node; node = iter.Skip()){
         FUInstance* inst = node->inst;
         FUDeclaration* decl = inst->declaration;
         int storingIndex = info.config.configOffsets.offsets[index];

         String name = iter.GetFullName(arena,"_");
         for(int ii = 0; ii < inst->declaration->configInfo.configs.size; ii++){
           String fullName = PushString(arena,"%.*s_%.*s",UNPACK_SS(name),UNPACK_SS(decl->configInfo.configs[ii].name));
           mergedConfigs[i][storingIndex + ii] = fullName; // MARK
         }

         index += 1;
       }
     }

     TemplateSetBool("doingMerged",true);
     TemplateSetCustom("mergedConfigs",&mergedConfigs,"Array<Array<String>>");

//     DebugValue(MakeValue(&mergedConfigs));
   }

   }// TEMPORARY_MARK
#endif

   TemplateSetCustom("namedConfigs",namedConfigs,"Hashmap<String,SizedConfig>");
   TemplateSetCustom("namedStates",namedStates,"Hashmap<String,SizedConfig>");
   TemplateSetCustom("namedMem",namedMem,"Hashmap<String,SizedConfig>");
   TemplateSetString("accelType",accelName);

   TemplateSetCustom("orderedConfigs",&orderedConfigs,"OrderedConfigurations");

   FILE* f = OpenFileAndCreateDirectories(StaticFormat("%s/versat_accel.h",softwarePath),"w");
   ProcessTemplate(f,BasicTemplates::acceleratorHeaderTemplate,&versat->temp);
   fclose(f);
   }

   {
   FILE* f = OpenFileAndCreateDirectories(StaticFormat("%s/versat_external_memory_inst.vh",hardwarePath),"w");
   TemplateSetCustom("external",&computedData.external,"Array<ExternalMemoryInterface>");
   ProcessTemplate(f,BasicTemplates::externalInstTemplate,&versat->temp);
   fclose(f);
   }

   {
   FILE* f = OpenFileAndCreateDirectories(StaticFormat("%s/versat_external_memory_port.vh",hardwarePath),"w");
   ProcessTemplate(f,BasicTemplates::externalPortTemplate,&versat->temp);
   fclose(f);
   }

   {
   FILE* f = OpenFileAndCreateDirectories(StaticFormat("%s/versat_external_memory_portmap.vh",hardwarePath),"w");
   ProcessTemplate(f,BasicTemplates::externalPortmapTemplate,&versat->temp);
   fclose(f);
   }

   fclose(s);

   ClearTemplateEngine();
}

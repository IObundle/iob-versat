#include "versatPrivate.hpp"

#include "templateEngine.hpp"

VersatComputedValues ComputeVersatValues(Versat* versat,Accelerator* accel){
   #if 0
   ArenaMarker marker(&versat->temp);
   AcceleratorView view = CreateAcceleratorView(accel,&versat->temp);
   view.CalculateDAGOrdering(&versat->temp);
   #endif

   VersatComputedValues res = {};

   FOREACH_LIST(ptr,accel->allocated){
      ComplexFUInstance* inst = ptr->inst;
      FUDeclaration* decl = inst->declaration;

      res.numberConnections += Size(ptr->allOutputs);

      if(decl->isMemoryMapped){
         res.maxMemoryMapDWords = std::max(res.maxMemoryMapDWords,1 << decl->memoryMapBits);
         res.memoryMappedBytes += (1 << (decl->memoryMapBits + 2));
         res.unitsMapped += 1;
      }

      res.nConfigs += decl->configs.size;
      for(Wire& wire : decl->configs){
         res.configBits += wire.bitsize;
      }

      res.nStates += decl->states.size;
      for(Wire& wire : decl->states){
         res.stateBits += wire.bitsize;
      }

      res.nDelays += decl->nDelays;
      res.delayBits += decl->nDelays * 32;

      res.nUnitsIO += decl->nIOs;
   }

   for(StaticInfo* unit : accel->staticInfo){
      res.nStatics += unit->data.configs.size;

      for(Wire& wire : unit->data.configs){
         res.staticBits += wire.bitsize;
      }
   }

   res.staticBitsStart = res.configBits;
   res.delayBitsStart = res.staticBitsStart + res.staticBits;

   // Versat specific registers are treated as a special maping (all 0's) of 1 configuration and 1 state register
   res.nConfigs += 1;
   res.nStates += 1;

   res.nConfigurations = res.nConfigs + res.nStatics + res.nDelays;
   res.configurationBits = res.configBits + res.staticBits + res.delayBits;

   res.memoryAddressBits = log2i(res.memoryMappedBytes / 4);

   res.memoryMappingAddressBits = res.memoryAddressBits;
   res.configurationAddressBits = log2i(res.nConfigurations);
   res.stateAddressBits = log2i(res.nStates);
   res.stateConfigurationAddressBits = std::max(res.configurationAddressBits,res.stateAddressBits);

   res.lowerAddressSize = std::max(res.stateConfigurationAddressBits,res.memoryMappingAddressBits);

   res.memoryConfigDecisionBit = res.lowerAddressSize;

   return res;
}

#include "debug.hpp"

void OutputCircuitSource(Versat* versat,FUDeclaration* decl,Accelerator* accel,FILE* file){
   if(!versat->debug.outputAccelerator){
      return;
   }

   Arena* arena = &versat->temp;
   ArenaMarker marker(arena);

   TemplateSetCustom("accel",decl,"FUDeclaration");

   VersatComputedValues val = ComputeVersatValues(versat,accel);

   // Output configuration file
   TemplateSetCustom("versatValues",&val,"VersatComputedValues");
   TemplateSetNumber("numberUnits",Size(accel->allocated));

   int nonSpecialUnits = CountNonSpecialChilds(accel->allocated);

   #if 0
   int nonSpecialUnits = 0;
   FOREACH_LIST(inst,accel->instances){
      if(inst->declaration->type != FUDeclaration::SPECIAL && !inst->declaration->isOperation){
         nonSpecialUnits += 1;
      }
   }
   #endif

   int size = Size(accel->allocated);
   Array<InstanceNode*> nodes = PushArray<InstanceNode*>(arena,size);
   int i = 0;
   FOREACH_LIST_INDEXED(ptr,accel->allocated,i){
      nodes[i] = ptr;
   }

   TemplateSetCustom("instances",&nodes,"Array<InstanceNode*>");

   TemplateSetNumber("nonSpecialUnits",nonSpecialUnits);

   TemplateSetNumber("unitsMapped",val.unitsMapped);
   TemplateSetNumber("configurationBits",val.configurationBits);

   TemplateSetNumber("memoryConfigDecisionBit",val.memoryConfigDecisionBit);
   TemplateSetCustom("versat",versat,"Versat");

   TemplateSetCustom("inputDecl",BasicDeclaration::input,"FUDeclaration");
   TemplateSetCustom("outputDecl",BasicDeclaration::output,"FUDeclaration");

   if(!BasicTemplates::acceleratorTemplate){
      BasicTemplates::acceleratorTemplate = CompileTemplate("../../submodules/VERSAT/software/templates/versat_accelerator_template.tpl",&versat->permanent);
   }
   ProcessTemplate(file,BasicTemplates::acceleratorTemplate,&versat->temp);

   //ProcessTemplate(file,"../../submodules/VERSAT/software/templates/versat_accelerator_template.tpl",&versat->temp);
}

void OutputVersatSource(Versat* versat,Accelerator* accel,const char* sourceFilepath,const char* constantsFilepath,const char* dataFilepath){
   if(!versat->debug.outputVersat){
      return;
   }

   Arena* arena = &versat->temp;
   ArenaMarker marker(arena);

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
   FOREACH_LIST_INDEXED(ptr,accel->allocated,i){
      nodes[i] = ptr;
   }

   TemplateSetCustom("instances",&nodes,"Array<InstanceNode*>");

   TemplateSetNumber("versatBase",versat->base);
   TemplateSetNumber("memoryAddressBits",val.memoryAddressBits);
   TemplateSetNumber("unitsMapped",val.unitsMapped);
   TemplateSetNumber("memoryConfigDecisionBit",val.memoryConfigDecisionBit);
   TemplateSetNumber("configurationBits",val.configurationBits);
   TemplateSetNumber("memoryMappedBase",1 << val.memoryConfigDecisionBit);

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
   TemplateSetNumber("nStatics",val.nStatics);
   TemplateSetCustom("instances",&accum,"std::vector<ComplexFUInstance>");
   TemplateSetNumber("numberUnits",accum.size());
   TemplateSetBool("IsSimple",false);
   ProcessTemplate(d,BasicTemplates::dataTemplate,&versat->temp);

   //PopMark(&versat->temp,mark);

   fclose(s);
   fclose(d);
   #endif
}

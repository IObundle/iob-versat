#include "codeGeneration.hpp"

#include "versatPrivate.hpp"
#include "templateEngine.hpp"

VersatComputedValues ComputeVersatValues(Versat* versat,Accelerator* accel){
   LockAccelerator(accel,Accelerator::Locked::GRAPH);

   VersatComputedValues res = {};

   for(ComplexFUInstance* inst : accel->instances){
      FUDeclaration* decl = inst->declaration;

      res.numberConnections += inst->tempData->numberOutputs;

      if(decl->isMemoryMapped){
         res.maxMemoryMapDWords = maxi(res.maxMemoryMapDWords,1 << decl->memoryMapBits);
         res.memoryMappedBytes += (1 << (decl->memoryMapBits + 2));
         res.unitsMapped += 1;
      }

      res.nConfigs += decl->nConfigs;
      for(int i = 0; i < decl->nConfigs; i++){
         res.configBits += decl->configWires[i].bitsize;
      }

      res.nStates += decl->nStates;
      for(int i = 0; i < decl->nStates; i++){
         res.stateBits += decl->stateWires[i].bitsize;
      }

      res.nDelays += decl->nDelays;
      res.delayBits += decl->nDelays * 32;

      res.nUnitsIO += decl->nIOs;
   }

   for(StaticInfo* unit : accel->staticInfo){
      res.nStatics += unit->nConfigs;

      for(int i = 0; i < unit->nConfigs; i++){
         res.staticBits += unit->wires[i].bitsize;
      }
   }

   // Versat specific registers are treated as a special maping (all 0's) of 1 configuration and 1 state register
   res.nConfigs += 1;
   res.nStates += 1;

   res.nConfigurations = res.nConfigs + res.nStatics + res.nDelays;
   res.configurationBits = res.configBits + res.staticBits + res.delayBits;

   res.memoryAddressBits = log2i(res.memoryMappedBytes / 4);

   res.memoryMappingAddressBits = res.memoryAddressBits;
   res.configurationAddressBits = log2i(res.nConfigurations);
   res.stateAddressBits = log2i(res.nStates);
   res.stateConfigurationAddressBits = maxi(res.configurationAddressBits,res.stateAddressBits);

   res.lowerAddressSize = maxi(res.stateConfigurationAddressBits,res.memoryMappingAddressBits);

   res.memoryConfigDecisionBit = res.lowerAddressSize;

   return res;
}

void OutputCircuitSource(Versat* versat,FUDeclaration* decl,Accelerator* accel,FILE* file){
   if(!versat->debug.outputAccelerator){
      return;
   }

   #if 1
   LockAccelerator(accel,Accelerator::Locked::FIXED);

   TemplateSetCustom("accel",decl,"FUDeclaration");

   VersatComputedValues val = ComputeVersatValues(versat,accel);

   // Output configuration file
   TemplateSetCustom("versatValues",&val,"VersatComputedValues");
   TemplateSetCustom("instances",&accel->instances,"Pool<ComplexFUInstance>");
   TemplateSetNumber("numberUnits",accel->instances.Size());

   int nonSpecialUnits = 0;
   for(ComplexFUInstance* inst : accel->instances){
      if(inst->declaration->type != FUDeclaration::SPECIAL && !inst->declaration->isOperation){
         nonSpecialUnits += 1;
      }
   }

   TemplateSetNumber("nonSpecialUnits",nonSpecialUnits);

   TemplateSetNumber("unitsMapped",val.unitsMapped);
   TemplateSetNumber("configurationBits",val.configurationBits);

   TemplateSetNumber("memoryConfigDecisionBit",val.memoryConfigDecisionBit);
   TemplateSetCustom("versat",versat,"Versat");

   ProcessTemplate(file,"../../submodules/VERSAT/software/templates/versat_accelerator_template.tpl",&versat->temp);
   #endif
}

void OutputVersatSource(Versat* versat,Accelerator* accel,const char* sourceFilepath,const char* constantsFilepath,const char* dataFilepath){
   if(!versat->debug.outputVersat){
      return;
   }

   LockAccelerator(accel,Accelerator::Locked::FIXED);

   #if 1
   // No need for templating, small file
   FILE* c = fopen(constantsFilepath,"w");

   if(!c){
      printf("Error creating file, check if filepath is correct: %s\n",constantsFilepath);
      return;
   }

   VersatComputedValues val = ComputeVersatValues(versat,accel);

   #if 1
   std::vector<ComplexFUInstance*> accum;
   AcceleratorIterator iter = {};
   for(ComplexFUInstance* inst = iter.Start(accel); inst; inst = iter.Next()){
      if(!inst->declaration->isOperation && inst->declaration->type != FUDeclaration::SPECIAL){
         accum.push_back(inst);
      }
   };
   #endif

   fprintf(c,"`define NUMBER_UNITS %d\n",accel->instances.Size());
   fprintf(c,"`define CONFIG_W %d\n",val.configurationBits);
   fprintf(c,"`define STATE_W %d\n",val.stateBits);
   fprintf(c,"`define MAPPED_UNITS %d\n",val.unitsMapped);
   fprintf(c,"`define MAPPED_BIT %d\n",val.memoryConfigDecisionBit);
   fprintf(c,"`define nIO %d\n",val.nUnitsIO);

   if(val.nUnitsIO){
      fprintf(c,"`define VERSAT_IO\n");
   }

   fclose(c);

   FILE* s = fopen(sourceFilepath,"w");

   if(!s){
      printf("Error creating file, check if filepath is correct: %s\n",sourceFilepath);
      return;
   }

   FILE* d = fopen(dataFilepath,"w");

   if(!d){
      fclose(s);
      printf("Error creating file, check if filepath is correct: %s\n",dataFilepath);
      return;
   }

   TemplateSetNumber("numberUnits",accel->instances.Size());
   TemplateSetCustom("versatValues",&val,"VersatComputedValues");
   TemplateSetCustom("versat",versat,"Versat");
   TemplateSetCustom("accel",accel,"Accelerator");
   TemplateSetNumber("nStatics",val.nStatics);
   TemplateSetNumber("nDelays",val.nDelays);

   // Output configuration file
   TemplateSetCustom("instances",&accel->instances,"Pool<ComplexFUInstance>");

   TemplateSetNumber("versatBase",versat->base);
   TemplateSetNumber("memoryAddressBits",val.memoryAddressBits);
   TemplateSetNumber("unitsMapped",val.unitsMapped);
   TemplateSetNumber("memoryConfigDecisionBit",val.memoryConfigDecisionBit);
   TemplateSetNumber("configurationBits",val.configurationBits);
   TemplateSetNumber("memoryMappedBase",1 << val.memoryConfigDecisionBit);

   ProcessTemplate(s,"../../submodules/VERSAT/software/templates/versat_top_instance_template.tpl",&versat->temp);



   /*
   unsigned int hashTableSize = 2048;
   Assert(accum.size() < (hashTableSize / 2));
   HashKey hashMap[hashTableSize];
   for(size_t i = 0; i < hashTableSize; i++){
      hashMap[i].data = -1;
      hashMap[i].key = (SizedString){nullptr,0};
   }

   Byte* mark = MarkArena(&versat->temp);
   for(size_t i = 0; i < accum.size(); i++){
      char* name = GetHierarchyNameRepr(accum[i]->name);

      SizedString ss = PushString(&versat->temp,MakeSizedString(name));

      unsigned int hash = 0;
      for(int ii = 0; ii < ss.size; ii++){
         hash *= 17;
         hash += ss.str[ii];
      }

      if(hashMap[hash % hashTableSize].data == -1){
         hashMap[hash % hashTableSize].data = i;
         hashMap[hash % hashTableSize].key = ss;
      } else {
         unsigned int counter;
         for(counter = 0; counter < hashTableSize; counter++){
            if(hashMap[(hash + counter) % hashTableSize].data == -1){
               hashMap[(hash + counter) % hashTableSize].data = i;
               hashMap[(hash + counter) % hashTableSize].key = ss;
               break;
            }
         }
         Assert(counter != hashTableSize);
      }
   }


   TemplateSetNumber("hashTableSize",hashTableSize);
   TemplateSetArray("instanceHashmap","HashKey",hashMap,hashTableSize);
   */

   TemplateSetNumber("configurationSize",accel->configAlloc.size);
   TemplateSetArray("configurationData","int",accel->configAlloc.ptr,accel->configAlloc.size);

   TemplateSetNumber("config",(int)accel->configAlloc.ptr);
   TemplateSetNumber("state",(int)accel->stateAlloc.ptr);
   TemplateSetNumber("memMapped",(int)0);
   TemplateSetNumber("static",(int)accel->staticAlloc.ptr);
   TemplateSetNumber("staticEnd",(int)&accel->staticAlloc.ptr[accel->staticAlloc.reserved]);

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
   ProcessTemplate(d,"../../submodules/VERSAT/software/templates/embedData.tpl",&versat->temp);

   //PopMark(&versat->temp,mark);

   fclose(s);
   fclose(d);
   #endif
}

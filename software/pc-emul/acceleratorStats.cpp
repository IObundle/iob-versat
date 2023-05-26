#include "acceleratorStats.hpp"

int MemorySize(Array<ExternalMemoryInterface> interfaces){
   int size = 0;

   for(ExternalMemoryInterface& inter : interfaces){
      size = AlignBitBoundary(size,inter.bitsize);
      size += 1 << inter.bitsize;
   }

   return size;
}

int NumberUnits(Accelerator* accel){
   STACK_ARENA(temp,Kilobyte(1));

   int count = 0;
   AcceleratorIterator iter = {};
   for(InstanceNode* node = iter.Start(accel,&temp,false); node; node = iter.Next()){
      count += 1;
   }
   return count;
}

VersatComputedValues ComputeVersatValues(Versat* versat,Accelerator* accel){
   VersatComputedValues res = {};

   int memoryMappedDWords = 0;
   FOREACH_LIST(ptr,accel->allocated){
      ComplexFUInstance* inst = ptr->inst;
      FUDeclaration* decl = inst->declaration;

      res.numberConnections += Size(ptr->allOutputs);

      if(decl->isMemoryMapped){
         memoryMappedDWords = AlignBitBoundary(memoryMappedDWords,decl->memoryMapBits);
         memoryMappedDWords += 1 << decl->memoryMapBits;

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

      res.externalMemoryInterfaces += decl->externalMemory.size;
   }

   for(auto pair : accel->staticUnits){
      StaticData data = pair.second;
      res.nStatics += data.configs.size;

      for(Wire& wire : data.configs){
         res.staticBits += wire.bitsize;
      }
   }

   res.staticBitsStart = res.configBits;
   res.delayBitsStart = res.staticBitsStart + res.staticBits;

   // Versat specific registers are treated as a special maping (all 0's) of 1 configuration and 1 state register
   res.versatConfigs = 5;
   res.versatStates = 5;
   res.nUnitsIO += 1; // For the DMA

   res.nConfigs += res.versatConfigs;
   res.nStates += res.versatStates;

   res.nConfigurations = res.nConfigs + res.nStatics + res.nDelays;
   res.configurationBits = res.configBits + res.staticBits + res.delayBits;

   res.memoryMappedBytes = memoryMappedDWords * 4;
   res.memoryAddressBits = log2i(memoryMappedDWords);

   res.memoryMappingAddressBits = res.memoryAddressBits;
   res.configurationAddressBits = log2i(res.nConfigurations);
   res.stateAddressBits = log2i(res.nStates);
   res.stateConfigurationAddressBits = std::max(res.configurationAddressBits,res.stateAddressBits);

   res.lowerAddressSize = std::max(res.stateConfigurationAddressBits,res.memoryMappingAddressBits);

   res.memoryConfigDecisionBit = res.lowerAddressSize;

   return res;
}

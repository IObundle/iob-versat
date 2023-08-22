#include "acceleratorStats.hpp"

int ExternalMemoryByteSize(ExternalMemoryInterface* inter){
  int size = 0;

  switch(inter->type){
  case ExternalMemoryType::TWO_P:{
	size = std::max((1 << inter->tp.bitSizeIn)  * (inter->tp.dataSizeIn / 8),
                  (1 << inter->tp.bitSizeOut) * (inter->tp.dataSizeOut / 8));
  }break;
  case ExternalMemoryType::DP:{
	size = std::max((1 << inter->dp[0].bitSize) * std::max(inter->dp[0].dataSizeIn,inter->dp[0].dataSizeOut),
                  (1 << inter->dp[1].bitSize) * std::max(inter->dp[1].dataSizeIn,inter->dp[1].dataSizeOut));
  }break;
  default: NOT_IMPLEMENTED;
  }

  return size;
}

int ExternalMemoryByteSize(Array<ExternalMemoryInterface> interfaces){
  int size = 0;

  // Not to sure about this logic. So far we do not allow different DATA_W values for the same port memories, but technically this should work
  for(ExternalMemoryInterface& inter : interfaces){
    int max = ExternalMemoryByteSize(&inter);

    // Aligns
    int nextPower2 = AlignNextPower2(max);
    int boundary = log2i(nextPower2);

    int aligned = AlignBitBoundary(size,boundary);
    size = aligned + max;
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
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
  FUInstance* inst = ptr->inst;
  FUDeclaration* decl = inst->declaration;

  res.numberConnections += Size(ptr->allOutputs);

  if(decl->isMemoryMapped){
    memoryMappedDWords = AlignBitBoundary(memoryMappedDWords,decl->memoryMapBits);
    memoryMappedDWords += 1 << decl->memoryMapBits;

    res.unitsMapped += 1;
  }

  res.nConfigs += decl->configs.size;
  for(Wire& wire : decl->configs){
    res.configBits += wire.bitSize;
  }

  res.nStates += decl->states.size;
  for(Wire& wire : decl->states){
    res.stateBits += wire.bitSize;
  }

  res.nDelays += decl->delayOffsets.max;
  res.delayBits += decl->delayOffsets.max * 32;

  res.nUnitsIO += decl->nIOs;

  res.externalMemoryInterfaces += decl->externalMemory.size;
  res.signalLoop |= decl->signalLoop;
}

  for(auto pair : accel->staticUnits){
    StaticData data = pair.second;
    res.nStatics += data.configs.size;

    for(Wire& wire : data.configs){
      res.staticBits += wire.bitSize;
    }
  }

  res.staticBitsStart = res.configBits;
  res.delayBitsStart = res.staticBitsStart + res.staticBits;

  // Versat specific registers are treated as a special maping (all 0's) of 1 configuration and 1 state register
  res.versatConfigs = 1;
  res.versatStates = 1;

  if(accel->useDMA){
    res.versatConfigs += 4;
    res.versatStates += 4;

    res.nUnitsIO += 1; // For the DMA
  }

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




#include "acceleratorStats.hpp"
#include "utilsCore.hpp"

// Checks wether the external memory conforms to the expected interface or not (has valid values)
bool VerifyExternalMemory(ExternalMemoryInterface* inter){
  bool res;

  switch(inter->type){
  case ExternalMemoryType::TWO_P:{
    res = (inter->tp.bitSizeIn == inter->tp.bitSizeOut);
  }break;
  case ExternalMemoryType::DP:{
    res = (inter->dp[0].bitSize == inter->dp[1].bitSize);
  }break;
  default: NOT_IMPLEMENTED;
  }

  return res;
}

// Function that calculates byte offset from size of data_w
int DataWidthToByteOffset(int dataWidth){
  // 0-8 : 0,
  // 9-16 : 1
  // 17 - 32 : 2,
  // 33 - 64 : 3

  // 0 : 0
  // 8 : 0

  if(dataWidth == 0){
    return 0;
  }

  int res = log2i((dataWidth - 1) / 8);
  return res;
}

int ExternalMemoryByteSize(ExternalMemoryInterface* inter){
  Assert(VerifyExternalMemory(inter));

  // TODO: The memories size and address is still error prone. We should just store the total size and the address width in the structures (and then we can derive the data width from the given address width and the total size). Storing data width and address size at the same time gives more degrees of freedom than need. The parser is responsible in ensuring that the data is given in the correct format and this code should never have to worry about having to deal with values out of phase
  int addressBitSize = 0;
  int byteOffset = 0;
  switch(inter->type){
  case ExternalMemoryType::TWO_P:{
    addressBitSize = inter->tp.bitSizeIn;
    byteOffset = std::min(DataWidthToByteOffset(inter->tp.dataSizeIn),DataWidthToByteOffset(inter->tp.dataSizeOut));
  }break;
  case ExternalMemoryType::DP:{
    addressBitSize = inter->dp[0].bitSize;
    byteOffset = std::min(DataWidthToByteOffset(inter->dp[0].dataSizeIn),DataWidthToByteOffset(inter->dp[1].dataSizeIn));
  }break;
  default: NOT_IMPLEMENTED;
  }

  int byteSize = (1 << (addressBitSize + byteOffset));

  return byteSize;
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

VersatComputedValues ComputeVersatValues(Versat* versat,Accelerator* accel,bool useDMA){
  VersatComputedValues res = {};

  int memoryMappedDWords = 0;
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    FUInstance* inst = ptr->inst;
    FUDeclaration* decl = inst->declaration;

    res.numberConnections += Size(ptr->allOutputs);

    if(decl->memoryMapBits.has_value()){
      memoryMappedDWords = AlignBitBoundary(memoryMappedDWords,decl->memoryMapBits.value());
      memoryMappedDWords += 1 << decl->memoryMapBits.value();

      res.unitsMapped += 1;
    }

    res.nConfigs += decl->configInfo.configs.size;
    for(Wire& wire : decl->configInfo.configs){
      res.configBits += wire.bitSize;
    }

    res.nStates += decl->configInfo.states.size;
    for(Wire& wire : decl->configInfo.states){
      res.stateBits += wire.bitSize;
    }

    res.nDelays += decl->configInfo.delayOffsets.max;
    res.delayBits += decl->configInfo.delayOffsets.max * 32;

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

  if(useDMA){
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

  //res.lowerAddressSize = std::max(res.stateConfigurationAddressBits,res.memoryMappingAddressBits);
  //res.memoryConfigDecisionBit = res.lowerAddressSize;
  res.memoryConfigDecisionBit = std::max(res.stateConfigurationAddressBits,res.memoryMappingAddressBits) + 2;
  
  return res;
}




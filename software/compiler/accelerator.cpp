#include "accelerator.hpp"

#include "declaration.hpp"
#include "embeddedData.hpp"
#include "globals.hpp"
#include "utils.hpp"
#include "versat.hpp"

#include "symbolic.hpp"
#include "userConfigs.hpp"

#define TAG_TEMPORARY 1
#define TAG_PERMANENT 2

static Pool<Accelerator> accelerators;

// ======================================
// Accelerator creation

Accelerator* CreateAccelerator(String name,AcceleratorPurpose purpose){
  static int globalID = 0;
  Accelerator* accel = accelerators.Alloc();
  
  accel->accelMemory = CreateDynamicArena(1);
  accel->id = globalID++;
  
  accel->name = PushString(accel->accelMemory,name);
  accel->purpose = purpose;
  PushString(accel->accelMemory,{"",1}); // TODO: What?
  
  return accel;
}

bool NameExists(Accelerator* accel,String name){
  FUInstance* inst = GetUnit(accel,name);
  bool res = (inst != nullptr);
  return res;
}

String GenerateNewValidName(Accelerator* accel,String base,Arena* out){
  TEMP_REGION(temp,out);
  
  for(int i = 0; true; i++){
    String possible = PushString(temp,"%.*s_%d",UN(base),i);
    if(!NameExists(accel,possible)){
      return PushString(out,possible);
    }
  }
}

int GetFreeShareIndex(Accelerator* accel){
  int attempt = 0; 
  while(1){
    bool found = true;
    for(FUInstance* inst : accel->allocated){
      if(inst->sharedEnable && inst->sharedIndex == attempt){
        attempt += 1;
        found = false;
        break;
      }
    }

    if(found){
      return attempt;
    }
  }
}

void ShareInstanceConfig(FUInstance* inst, int shareBlockIndex){
  inst->sharedIndex = shareBlockIndex;
  inst->sharedEnable = true;
  Memset(inst->isSpecificConfigShared,true);
}

void SetStatic(FUInstance* inst){
  inst->isStatic = true;
}

FUInstance* CreateFUInstance(Accelerator* accel,FUDeclaration* type,String name){
  static int globalID = 0;
  String storedName = PushString(accel->accelMemory,name);

  Assert(CheckValidName(storedName));
  Assert(!NameExists(accel,storedName));
  
  FUInstance* inst = accel->allocated.Alloc();

  int parameterSize = type->parameters.size;
  inst->parameterValues = PushArray<ParameterValue>(accel->accelMemory,parameterSize); 
  Memset(inst->parameterValues,{});
  
  inst->inputs = PushArray<PortInstance>(accel->accelMemory,type->NumberInputs());
  inst->outputs = PushArray<bool>(accel->accelMemory,type->NumberOutputs());
  
  inst->name = storedName;
  inst->accel = accel;
  inst->declaration = type;
  inst->id = globalID++;

  inst->isSpecificConfigShared = PushArray<bool>(globalPermanent,type->configs.size);
  Memset(inst->isSpecificConfigShared,false);
  
  return inst;
}

Accelerator* CopyAccelerator(Accelerator* accel,AcceleratorPurpose purpose,bool preserveIds){
  TEMP_REGION(temp,nullptr);
  
  Accelerator* newAccel = CreateAccelerator(accel->name,purpose);

  TrieMap<FUInstance*,FUInstance*>* map = PushTrieMap<FUInstance*,FUInstance*>(temp);

  // Copy of instances
  for(FUInstance* ptr : accel->allocated){
    FUInstance* inst = ptr;
    FUInstance* newInst = CopyInstance(newAccel,inst,preserveIds,inst->name);
    
    map->Insert(inst,newInst);
  }

  // Flat copy of edges
  for(EdgeIterator iter = IterateEdges(accel); iter.IsValid(); iter.Next()){
    Edge edge = iter.Value();
    FUInstance* out = map->GetOrFail(edge.units[0].inst);
    int outPort = edge.units[0].port;
    FUInstance* in = map->GetOrFail(edge.units[1].inst);
    int inPort = edge.units[1].port;
    
    ConnectUnits(out,outPort,in,inPort,edge.delay);
  }
  
  return newAccel;
}

Pair<Accelerator*,AcceleratorMapping*> CopyAcceleratorWithMapping(Accelerator* accel,AcceleratorPurpose purpose,bool preserveIds,Arena* out){
  Accelerator* newAccel = CreateAccelerator(accel->name,purpose);

  AcceleratorMapping* map = MappingSimple(accel,newAccel,out);
  
  // Copy of instances
  for(FUInstance* inst : accel->allocated){
    FUInstance* newInst = CopyInstance(newAccel,inst,preserveIds,inst->name);
    
    MappingInsertEqualNode(map,inst,newInst);
  }

  // Flat copy of edges
  for(EdgeIterator iter = IterateEdges(accel); iter.IsValid(); iter.Next()){
    Edge edge = iter.Value();

    PortInstance out = MappingMapOutput(map,edge.out);
    PortInstance in = MappingMapInput(map,edge.in);
    
    ConnectUnits(out.inst,out.port,in.inst,in.port,edge.delay);
  }
  
  return {newAccel,map};
}

FUInstance* CopyInstance(Accelerator* accel,FUInstance* oldInstance,bool preserveIds,String newName){
  FUInstance* newInst = CreateFUInstance(accel,oldInstance->declaration,newName);

  newInst->parameterValues = PushArray<ParameterValue>(accel->accelMemory,oldInstance->parameterValues.size);
  for(int i = 0; i < newInst->parameterValues.size; i++){
    newInst->parameterValues[i] = oldInstance->parameterValues[i];
  }
  
  // Any of the union values is copied here.
  newInst->portIndex = oldInstance->portIndex;

  newInst->sharedEnable = oldInstance->sharedEnable;
  newInst->sharedIndex = oldInstance->sharedIndex;

  for(int i = 0; i < oldInstance->isSpecificConfigShared.size; i++){
    newInst->isSpecificConfigShared[i] = oldInstance->isSpecificConfigShared[i];
  }
  
  if(oldInstance->isStatic){
    newInst->isStatic = oldInstance->isStatic;
  }
  newInst->isMergeMultiplexer = oldInstance->isMergeMultiplexer;
  
  if(preserveIds){
    newInst->id = oldInstance->id;
  }
  newInst->isMergeMultiplexer = oldInstance->isMergeMultiplexer;
  newInst->addressGenUsed = oldInstance->addressGenUsed;
  newInst->debug = oldInstance->debug;
  
  return newInst;
}

FUInstance* GetUnit(Accelerator* accel,String name){
  for(FUInstance* inst : accel->allocated){
    if(inst->name == name){
      return inst;
    }
  }
  return nullptr;
}

Array<int> ExtractInputDelays(Accelerator* accel,CalculateDelayResult delays,int mimimumAmount,Arena* out){
  TEMP_REGION(temp,out);
  Array<FUInstance*> inputNodes = PushArray<FUInstance*>(temp,99);
  Memset(inputNodes,{});
  
  bool seenOneInput = false;
  int maxInput = 0;
  for(FUInstance* ptr : accel->allocated){
    if(ptr->declaration == BasicDeclaration::input){
      maxInput = MAX(maxInput,ptr->portIndex);
      inputNodes[ptr->portIndex] = ptr;
      seenOneInput = true;
    }
  }
  if(seenOneInput) maxInput += 1;
  inputNodes.size = MAX(maxInput,mimimumAmount);
  
  Array<int> inputDelay = PushArray<int>(out,inputNodes.size);

  for(int i = 0; i < inputDelay.size; i++){
    FUInstance* node = inputNodes[i];
    
    if(node){
      inputDelay[i] = delays.nodeDelay->GetOrFail(node).value;
    } else {
      inputDelay[i] = 0;
    }
  }

  return inputDelay;
}

Array<int> ExtractOutputLatencies(Accelerator* accel,CalculateDelayResult delays,Arena* out){
  TEMP_REGION(temp,out);
  FUInstance* outputNode = nullptr;
  int maxOutput = 0;
  for(FUInstance* ptr : accel->allocated){
    if(ptr->declaration == BasicDeclaration::output){
      Assert(outputNode == nullptr);
      outputNode = ptr;
    }
  }

  if(outputNode){
    FOREACH_LIST(ConnectionNode*,ptr,outputNode->allInputs){
      maxOutput = MAX(maxOutput,ptr->port);
    }
    maxOutput += 1;
  } else {
    return {};
  }
  
  Array<int> outputLatencies = PushArray<int>(out,maxOutput);
  Memset(outputLatencies,0);

  if(outputNode){
    FOREACH_LIST(ConnectionNode*,ptr,outputNode->allInputs){
      int port = ptr->port;
      PortInstance end = MakePortIn(outputNode,port);

      outputLatencies[port] = delays.portDelay->GetOrFail(end).value;
    }
  }

  return outputLatencies;
}

// TODO: This functions should have actual error handling and reporting. Instead of just Asserting.
static int Visit(Array<FUInstance*> ordering,int& nodesFound,FUInstance* node,Hashmap<FUInstance*,int>* tags){
  int* tag = tags->Get(node);
  Assert(tag);

  if(*tag == TAG_PERMANENT){
    return 0;
  }
  if(*tag == TAG_TEMPORARY){
    Assert(0);
  }

  FUInstance* inst = node;

  if(node->type == NodeType_SINK ||
     (node->type == NodeType_SOURCE_AND_SINK && CHECK_DELAY(inst,DelayType::DelayType_SINK_DELAY))){
    return 0;
  }

  *tag = TAG_TEMPORARY;

  int count = 0;
  if(node->type == NodeType_COMPUTE){
    FOREACH_LIST(ConnectionNode*,ptr,node->allInputs){
      count += Visit(ordering,nodesFound,ptr->instConnectedTo.inst,tags);
    }
  }

  *tag = TAG_PERMANENT;

  if(node->type == NodeType_COMPUTE){
    ordering[nodesFound++] = node;
    count += 1;
  }

  return count;
}

DAGOrderNodes CalculateDAGOrder(Accelerator* accel,Arena* out){
  Pool<FUInstance>* instances = &accel->allocated;
  int size = instances->Size();

  DAGOrderNodes res = {};
  
  res.size = 0;
  res.instances = PushArray<FUInstance*>(out,size);
  res.order = PushArray<int>(out,size);

  int nodesFound = 0;
  
  // NOTE: Could replace Hashmap with an array and add a Pool function to get the index from a ptr.
  Hashmap<FUInstance*,int>* tags = PushHashmap<FUInstance*,int>(out,size);

  for(FUInstance* ptr : *instances){
    res.size += 1;
    tags->Insert(ptr,0);
  }

  // Add source units, guaranteed to come first
  for(FUInstance* ptr : *instances){
    FUInstance* inst = ptr;
    if(ptr->type == NodeType_SOURCE || (ptr->type == NodeType_SOURCE_AND_SINK && CHECK_DELAY(inst,DelayType::DelayType_SOURCE_DELAY))){
      res.instances[nodesFound++] = ptr;

      tags->Insert(ptr,TAG_PERMANENT);
    }
  }
  res.sources = {};
  res.sources.data = res.instances.data;
  res.sources.size = nodesFound;

  int computeIndexStart = nodesFound;
  FUInstance** computeStart = res.sources.data + res.sources.size;
  // Add compute units
  for(FUInstance* ptr : *instances){
    if(ptr->type == NodeType_UNCONNECTED){
      res.instances[nodesFound++] = ptr;
      tags->Insert(ptr,TAG_PERMANENT);
    } else {
      int tag = tags->GetOrFail(ptr);
      if(tag == 0 && ptr->type == NodeType_COMPUTE){
        Visit(res.instances,nodesFound,ptr,tags);
      }
    }
  }
  res.computeUnits.data = computeStart;
  res.computeUnits.size = nodesFound - computeIndexStart;
  
  int sinkIndexStart = nodesFound;
  FUInstance** sinkStart = res.computeUnits.data + res.computeUnits.size;
  // Add sink units
  for(FUInstance* ptr : *instances){
    FUInstance* inst = ptr;
    if(ptr->type == NodeType_SINK || (ptr->type == NodeType_SOURCE_AND_SINK && CHECK_DELAY(inst,DelayType::DelayType_SINK_DELAY))){
      res.instances[nodesFound++] = ptr;

      int* tag = tags->Get(ptr);
      Assert(*tag == 0);

      *tag = TAG_PERMANENT;
    }
  }
  res.sinks.data = sinkStart;
  res.sinks.size = nodesFound - sinkIndexStart;

  for(FUInstance* ptr : *instances){
    int tag = tags->GetOrFail(ptr);
    Assert(tag == TAG_PERMANENT);
  }

  Assert(res.sources.size + res.computeUnits.size + res.sinks.size == size);

  // Reuse tags hashmap
  Hashmap<FUInstance*,int>* orderIndex = tags;
  orderIndex->Clear();

  res.maxOrder = 0;
  for(int i = 0; i < size; i++){
    FUInstance* node = res.instances[i];

    int order = 0;
    FOREACH_LIST(ConnectionNode*,ptr,node->allInputs){
      FUInstance* other = ptr->instConnectedTo.inst;

      if(other->type == NodeType_SOURCE_AND_SINK){
        continue;
      }

      int index = orderIndex->GetOrFail(other);

      order = MAX(order,res.order[index]);
    }

    orderIndex->Insert(node,i); // Only insert after to detect any potential error.
    res.order[i] = order;
    res.maxOrder = MAX(res.maxOrder,order);
  }

  return res;
}

bool IsUnitCombinatorial(FUInstance* instance){
  NOT_IMPLEMENTED("Implement if needed after AccelInfo change");
  return {};
#if 0
  FUDeclaration* type = instance->declaration;

  // TODO: Could check if it's a multiple type situation and check each one to make sure.
  for(int i = 0; i < type->NumberOutputs(); i++){
    if(type->baseConfig.outputLatencies[i] != 0){
      return false;
    }
  }

  return true;
#endif
}

void FixDelays(Accelerator* accel,Hashmap<Edge,DelayInfo>* edgeDelays){
  TEMP_REGION(temp,nullptr);

  int buffersInserted = 0;
  for(auto edgePair : edgeDelays){
    Edge edge = edgePair.first;
    int delay = edgePair.second->value;

    if(delay == 0){
      continue;
    }

    if(edge.units[1].inst->declaration == BasicDeclaration::output){
      continue;
    }
    
    FUInstance* output = edge.units[0].inst;
    
    if(output->declaration == BasicDeclaration::variableBuffer){
      edgePair.second = 0;
      continue;
    }
    Assert(delay > 0); // Cannot deal with negative delays at this stage.

    FUInstance* buffer = nullptr;
    if(globalOptions.useFixedBuffers){
      String bufferName = PushString(globalPermanent,"fixedBuffer%d",buffersInserted);

      buffer = CreateFUInstance(accel,BasicDeclaration::fixedBuffer,bufferName);
      buffer->bufferAmount = delay - BasicDeclaration::fixedBuffer->info.infos[0].outputLatencies[0];

      SetParameter(buffer,"AMOUNT",PushLiteral(globalPermanent,buffer->bufferAmount));
    } else {
      String bufferName = PushString(globalPermanent,"buffer%d",buffersInserted);

      buffer = CreateFUInstance(accel,BasicDeclaration::variableBuffer,bufferName);
      buffer->bufferAmount = delay - BasicDeclaration::variableBuffer->info.infos[0].outputLatencies[0];
      Assert(buffer->bufferAmount >= 0);
      SetStatic(buffer);
    }

    InsertUnit(accel,edge.units[0],edge.units[1],MakePortOut(buffer,0),MakePortIn(buffer,0));

    //OutputDebugDotGraph(accel,SF("fixDelay_%d.dot",buffersInserted),buffer);

    buffersInserted += 1;
  }
}

FUInstance* GetInputInstance(Pool<FUInstance>* nodes,int inputIndex){
  for(FUInstance* ptr : *nodes){
    FUInstance* inst = ptr;
    if(inst->declaration == BasicDeclaration::input && inst->portIndex == inputIndex){
      return inst;
    }
  }
  return nullptr;
}

FUInstance* GetOutputInstance(Pool<FUInstance>* nodes){
  for(FUInstance* ptr : *nodes){
    FUInstance* inst = ptr;
    if(inst->declaration == BasicDeclaration::output){
      return inst;
    }
  }

  return nullptr;
}

SymbolicExpression* GetParameterValue(FUInstance* inst,String name){
  SymbolicExpression* val = {};
  for(int i = 0; i < inst->declaration->parameters.size; i++){
    Parameter param = inst->declaration->parameters[i];
    SymbolicExpression* paramVal = inst->parameterValues[i].val;

    if(CompareString(param.name,name)){
      if(paramVal){
        val = paramVal;
      } else {
        val = param.valueExpr;
      }
    }
  }

  return val;
}

PortInstance GetAssociatedOutputPortInstance(FUInstance* unit,int portIndex){
  PortInstance inPort = unit->inputs[portIndex];
  FUInstance* outInst = inPort.inst;

  for(ConnectionNode* ptr = outInst->allOutputs; ptr; ptr = ptr->next){
    if(ptr->instConnectedTo.inst == unit && ptr->instConnectedTo.port == portIndex && ptr->port == inPort.port){
      PortInstance res = MakePortOut(outInst,ptr->port);
      return res;
    }
  }
  Assert(false); // This function is not supposed to fail
  return {};
}

bool IsCombinatorial(Accelerator* accel){
  for(FUInstance* ptr : accel->allocated){
    if(ptr->declaration->fixedDelayCircuit == nullptr){
      continue;
    }
    
    bool isComb = IsCombinatorial(ptr->declaration->fixedDelayCircuit);
    if(!isComb){
      return false;
    }
  }
  return true;
}

Array<FUDeclaration*> MemSubTypes(AccelInfo* info,Arena* out){
  TEMP_REGION(temp,out);
  
  Set<FUDeclaration*>* maps = PushSet<FUDeclaration*>(temp,99);

  Array<InstanceInfo> test = info->infos[0].info;
  for(InstanceInfo& info : test){
    if(info.memMappedSize.has_value()){
      maps->Insert(GetTypeByName(info.typeName));
    }
  }
  
  Array<FUDeclaration*> subTypes = PushArray(out,maps);
  return subTypes;
}

Hashmap<StaticId,StaticData>* CollectStaticUnits(AccelInfo* info,Arena* out){
  Hashmap<StaticId,StaticData>* staticUnits = PushHashmap<StaticId,StaticData>(out,999);

  for(AccelInfoIterator iter = StartIteration(info); iter.IsValid(); iter = iter.Step()){
    InstanceInfo* info = iter.CurrentUnit();
    if(info->isStatic){
      StaticId id = {};
      id.name = info->name;
      id.parent = GetTypeByName(info->parentTypeName);

      StaticData data = {};
      data.decl = GetTypeByName(info->typeName);
      data.configs = info->configs;
      staticUnits->InsertIfNotExist(id,data);
    }
  }

  return staticUnits;
}

// Checks wether the external memory conforms to the expected interface or not (has valid values)
bool VerifyExternalMemory(ExternalMemoryInterface* inter){
  bool res = false;

  switch(inter->type){
  case ExternalMemoryType::ExternalMemoryType_2P:{
    res = (inter->tp.bitSizeIn == inter->tp.bitSizeOut);
  }break;
  case ExternalMemoryType::ExternalMemoryType_DP:{
    res = (inter->dp[0].bitSize == inter->dp[1].bitSize);
  }break;
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
  case ExternalMemoryType::ExternalMemoryType_2P:{
    addressBitSize = inter->tp.bitSizeIn;
    byteOffset = MIN(DataWidthToByteOffset(inter->tp.dataSizeIn),DataWidthToByteOffset(inter->tp.dataSizeOut));
  }break;
  case ExternalMemoryType::ExternalMemoryType_DP:{
    addressBitSize = inter->dp[0].bitSize;
    byteOffset = MIN(DataWidthToByteOffset(inter->dp[0].dataSizeIn),DataWidthToByteOffset(inter->dp[1].dataSizeIn));
  }break;
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

VersatComputedValues ComputeVersatValues(Accelerator* graph,AccelInfo* info,Arena* out){
  TEMP_REGION(temp,out);
  VersatComputedValues res = {};

  int memoryMappedDWords = 0;
  int delayBits = 0;
  int configBits = 0;
  int numberUnits = 0;
  int numberDones = 0;

  SymbolicExpression* configExpr = PushLiteral(temp,0);
  
  auto builder = StartArray<ExternalMemoryInterface>(temp);

  int defaultDelaySize = 7;
  int externalMemoryInterfaces = 0; 
  
  for(AccelInfoIterator iter = StartIteration(info); iter.IsValid(); iter = iter.Next()){
    InstanceInfo* unit = iter.CurrentUnit();
    
    if(unit->memMapBits.has_value()){
      memoryMappedDWords = AlignBitBoundary(memoryMappedDWords,unit->memMapBits.value());
      memoryMappedDWords += 1 << unit->memMapBits.value();

      res.unitsMapped += 1;
    }

    res.nConfigs += unit->configSize;
    for(Wire& wire : unit->configs){
      configBits += wire.bitSize;

      configExpr = Normalize(SymbolicAdd(configExpr,wire.sizeExpr,temp),temp);
    }

    res.nStates += unit->stateSize;
    for(Wire& wire : unit->states){
      res.stateBits += wire.bitSize;
    }

    res.nDelays += unit->numberDelays;
    delayBits += unit->numberDelays * defaultDelaySize;

    externalMemoryInterfaces += unit->externalMemory.size;

    if(unit->externalMemory.size){
      builder.PushArray(unit->externalMemory);
    }

    if(unit->singleInterfaces & SingleInterfaces_DONE){
      numberDones += 1;
    }
  }

  for(AccelInfoIterator iter = StartIteration(info); iter.IsValid(); iter = iter.Step()){
    InstanceInfo* unit = iter.CurrentUnit();
    if(!unit->isComposite){
      numberUnits += 1;
    }
  }    

  SymbolicExpression* staticSize = ParseSymbolicExpression(info->staticExpr,temp);
  SymbolicExpression* delayStart = Normalize(SymbolicAdd(configExpr,staticSize,temp),out);

  res.configSizeExpr = Normalize(configExpr,out);
  res.delayStart = delayStart;
  
  res.nDones = numberDones;
  
  Array<ExternalMemoryInterface> allExternalMemories = EndArray(builder);
  res.totalExternalMemory = ExternalMemoryByteSize(allExternalMemories);
  
  int staticBits = 0;
  res.nStatics = info->statics;
  staticBits = info->staticBits;

  res.nUnits = numberUnits;
  
  int staticBitsStart = configBits;
  res.delayBitsStart = staticBitsStart + staticBits;

  SymbolicExpression* total = SymbolicAdd(configExpr,staticSize,temp);
  total = SymbolicAdd(total,SymbolicMult(PushLiteral(temp,res.nDelays),PushVariable(temp,"DELAY_W"),temp),temp);
  
  res.configurationBitsExpr = Normalize(total,out);

  // Versat specific registers are treated as a special maping (all 0's) of 1 configuration and 1 state register
  auto registerList = PushList<VersatRegister>(temp);

  auto AddRegister = [&res,&registerList](VersatRegister reg){
    res.versatConfigs += 1;
    res.versatStates += 1;
    *registerList->PushElem() = reg;
  };

  AddRegister(VersatRegister_Control);
  
  res.nUnitsIO = info->nIOs;
  if(globalOptions.useDMA){
    AddRegister(VersatRegister_DmaInternalAddress);
    AddRegister(VersatRegister_DmaExternalAddress);
    AddRegister(VersatRegister_DmaTransferLength);
    AddRegister(VersatRegister_DmaControl);
    
    res.nUnitsIO += 1; // For the DMA
  }

  if(globalOptions.insertDebugRegisters){
    AddRegister(VersatRegister_Debug);
  }

  if(globalOptions.insertProfilingRegisters){
    for(ProfilingVersatRegisters_GenType reg : ProfilingVersatRegisters){
      AddRegister(reg.t);
    }
  }

  res.nConfigs += res.versatConfigs;
  res.nStates += res.versatStates;
  res.registers = PushArray(out,registerList);
  
  int nConfigurations = res.nConfigs + res.nStatics + res.nDelays;
  res.configurationBits = configBits + staticBits + delayBits;

  res.memoryMappedBytes = memoryMappedDWords * 4;
  res.memoryAddressBits = log2i(memoryMappedDWords);

  res.configurationAddressBits = log2i(nConfigurations);
  res.stateAddressBits = log2i(res.nStates);

  int memoryMappingAddressBits = res.memoryAddressBits;
  int stateConfigurationAddressBits = MAX(res.configurationAddressBits,res.stateAddressBits);

  res.memoryConfigDecisionBit = MAX(stateConfigurationAddressBits,memoryMappingAddressBits) + 2;
  
  res.numberConnections = info->numberConnections;

  Array<ExternalMemoryInterface> external = PushArray<ExternalMemoryInterface>(out,externalMemoryInterfaces);
  int externalIndex = 0;
  for(InstanceInfo& in : info->infos[0].info){
    if(!in.isComposite){
      for(ExternalMemoryInterface& inter : in.externalMemory){
        external[externalIndex++] = inter;
      }
    }
  }
  res.externalMemoryInterfaces = external;
  res.info = info;

  Pool<FUInstance> instances = graph->allocated;
  Hashmap<StaticId,StaticData>* staticUnits = CollectStaticUnits(info,out);

  Array<WireInformation> wireInfo = CalculateWireInformation(instances,staticUnits,res.versatConfigs,out);

  res.staticUnits = staticUnits;
  res.allWiresInfo = wireInfo;
  
  return res;
}

bool SetParameter(FUInstance* inst,String parameterName,SymbolicExpression* val){
  FUDeclaration* decl = inst->declaration;
  int paramSize = decl->parameters.size;
  
  for(int i = 0; i < paramSize; i++){
    if(CompareString(decl->parameters[i].name,parameterName)){
      inst->parameterValues[i].val = val;
      return true;
    }
  }

  return false;
}

static void CalculateNodeType(FUInstance* node){
  bool hasInput = (node->allInputs != nullptr);
  bool hasOutput = (node->allOutputs != nullptr);

  // If the unit is both capable of acting as a sink or as a source of data
  if(hasInput && hasOutput){
    if(CHECK_DELAY(node,DelayType_SINK_DELAY) || CHECK_DELAY(node,DelayType_SOURCE_DELAY)){
      node->type = NodeType_SOURCE_AND_SINK;
    }  else {
      node->type = NodeType_COMPUTE;
    }
  } else if(hasInput){
    node->type = NodeType_SINK;
  } else if(hasOutput){
    node->type = NodeType_SOURCE;
  } else {
    node->type = NodeType_UNCONNECTED;
  }
}

static void FixInputs(FUInstance* node){
  Memset(node->inputs,{});
  node->multipleSamePortInputs = false;

  FOREACH_LIST(ConnectionNode*,ptr,node->allInputs){
    int port = ptr->port;

    if(node->inputs[port].inst){
      node->multipleSamePortInputs = true;
    }

    node->inputs[port] = ptr->instConnectedTo;
  }
}

void FixOutputs(FUInstance* node){
  Memset(node->outputs,false);

  FOREACH_LIST(ConnectionNode*,ptr,node->allOutputs){
    int port = ptr->port;
    node->outputs[port] = true;
  }
}

Array<Edge> GetAllEdges(Accelerator* accel,Arena* out){
  auto arr = StartArray<Edge>(out);
  for(FUInstance* ptr : accel->allocated){
    FOREACH_LIST(ConnectionNode*,con,ptr->allOutputs){
      Edge edge = MakeEdge(ptr,con->port,con->instConnectedTo.inst,con->instConnectedTo.port,con->edgeDelay);
      *arr.PushElem() = edge;
    }
  }

  Array<Edge> result = EndArray(arr);
  
  return result;
}

static void AdvanceUntilValid(EdgeIterator* iter){
  if(iter->currentNode != iter->end && iter->currentPort){
    return;
  }

  if(iter->currentNode == iter->end){
    Assert(iter->currentPort == nullptr);
    return;
  }
  
  while(iter->currentNode != iter->end){
    if(iter->currentPort == nullptr){
      ++iter->currentNode;
      
      if(iter->currentNode != iter->end){
        iter->currentPort = (*iter->currentNode)->allOutputs;
        if(iter->currentPort){
          break;
        }
      }
    }
  }
}

bool EdgeIterator::IsValid(){
  if(!(currentNode != end)){
    Assert(currentPort == nullptr);
    return false;
  } else {
    return true;
  }
}

void EdgeIterator::Next(){
  currentPort = currentPort->next;
  AdvanceUntilValid(this);
}

Edge EdgeIterator::Value(){
  Edge edge = MakeEdge(*currentNode,currentPort->port,currentPort->instConnectedTo.inst,currentPort->instConnectedTo.port,currentPort->edgeDelay);
  return edge;
}

EdgeIterator IterateEdges(Accelerator* accel){
  EdgeIterator iter = {};

  if(!accel->allocated.Size()){
    return iter;
  }

  iter.currentNode = accel->allocated.begin();
  iter.end = accel->allocated.end();
  iter.currentPort = (*iter.currentNode)->allOutputs;
  AdvanceUntilValid(&iter);
  
  return iter;
}

bool EdgeEqualNoDelay(const Edge& e0,const Edge& e1){
  bool res = (e0.in  == e1.in &&
              e0.out == e1.out);
  
  return res;
}

Opt<Edge> FindEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay){
  FUDeclaration* inDecl = in->declaration;
  FUDeclaration* outDecl = out->declaration;
  
  Assert(out->accel == in->accel);
  Assert(inIndex < inDecl->NumberInputs());
  Assert(outIndex < outDecl->NumberOutputs());

  Accelerator* accel = out->accel;

  for(EdgeIterator iter = IterateEdges(accel); iter.IsValid(); iter.Next()){
    Edge edge = iter.Value();

    if(edge.units[0].inst == out &&
       edge.units[0].port == outIndex &&
       edge.units[1].inst == in &&
       edge.units[1].port == inIndex &&
       edge.delay == delay) {
      return edge;
    }
  }

  return {};
}

Opt<Edge> FindEdge(PortInstance out,PortInstance in,int delay){
  return FindEdge(out.inst,out.port,in.inst,in.port,delay);
}

void ConnectUnitsGetEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay){
  ConnectUnitsGetEdge(out,outIndex,in,inIndex,delay,nullptr);
}

void ConnectUnits(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay){
  ConnectUnitsGetEdge(out,outIndex,in,inIndex,delay);
}

void ConnectUnitsIfNotConnected(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay){
  Accelerator* accel = out->accel;

  for(EdgeIterator iter = IterateEdges(accel); iter.IsValid(); iter.Next()){
    Edge edge = iter.Value();

    if(edge.units[0].inst == out && edge.units[0].port == outIndex &&
       edge.units[1].inst == in  && edge.units[1].port == inIndex &&
       delay == edge.delay){
      return;
    }
  }

  ConnectUnitsGetEdge(out,outIndex,in,inIndex,delay);
}

void ConnectUnits(PortInstance out,PortInstance in,int delay){
  FUDeclaration* inDecl = in.inst->declaration;
  FUDeclaration* outDecl = out.inst->declaration;

  Assert(out.inst->accel == in.inst->accel);
  Assert(in.port < inDecl->NumberInputs());
  Assert(out.port < outDecl->NumberOutputs());
  Assert(out.dir == Direction_OUTPUT);
  Assert(in.dir == Direction_INPUT);
  
  Accelerator* accel = out.inst->accel;

  // Update graph data.
  FUInstance* inputNode = in.inst;
  FUInstance* outputNode = out.inst;

  // Add info to outputNode
  // Update all outputs
  {
    ConnectionNode* con = PushStruct<ConnectionNode>(accel->accelMemory);
    con->edgeDelay = delay;
    con->port = out.port;
    con->instConnectedTo = MakePortIn(inputNode,in.port);
    
    outputNode->allOutputs = ListInsert(outputNode->allOutputs,con);
    outputNode->outputs[out.port] = true;
  }

  // Add info to inputNode
  {
    ConnectionNode* con = PushStruct<ConnectionNode>(accel->accelMemory);
    con->edgeDelay = delay;
    con->port = in.port;
    con->instConnectedTo = MakePortOut(outputNode,out.port);

    inputNode->allInputs = ListInsert(inputNode->allInputs,con);

    if(inputNode->inputs[in.port].inst){
      inputNode->multipleSamePortInputs = true;
    }

    inputNode->inputs[in.port] = MakePortOut(outputNode,out.port);
  }

  CalculateNodeType(inputNode);
  CalculateNodeType(outputNode);
}

String GlobalStaticWireName(StaticId id,Wire w,Arena* out){
  String res = PushString(out,"%.*s_%.*s_%.*s",UN(id.parent->name),UN(id.name),UN(w.name));
  return res;
}

// Connects out -> in
void ConnectUnitsGetEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay,Edge* previous){
  FUDeclaration* inDecl = in->declaration;
  FUDeclaration* outDecl = out->declaration;

  Assert(out->accel == in->accel);
  Assert(inIndex < inDecl->NumberInputs());
  Assert(outIndex < outDecl->NumberOutputs());

  Assert(!FindEdge(out,outIndex,in,inIndex,delay));
  Accelerator* accel = out->accel;

  // Update graph data.
  FUInstance* inputNode = in;
  FUInstance* outputNode = out;

  // Add info to outputNode
  // Update all outputs
  {
    ConnectionNode* con = PushStruct<ConnectionNode>(accel->accelMemory);
    con->edgeDelay = delay;
    con->port = outIndex;
    con->instConnectedTo = MakePortIn(inputNode,inIndex);

    outputNode->allOutputs = ListInsert(outputNode->allOutputs,con);
    outputNode->outputs[outIndex] = true;
  }

  // Add info to inputNode
  {
    ConnectionNode* con = PushStruct<ConnectionNode>(accel->accelMemory);
    con->edgeDelay = delay;
    con->port = inIndex;
    con->instConnectedTo = MakePortOut(outputNode,outIndex);

    inputNode->allInputs = ListInsert(inputNode->allInputs,con);

    if(inputNode->inputs[inIndex].inst){
      inputNode->multipleSamePortInputs = true;
    }

    inputNode->inputs[inIndex] = MakePortOut(outputNode,outIndex);
  }

  CalculateNodeType(inputNode);
  CalculateNodeType(outputNode);
}

Array<int> GetNumberOfInputConnections(FUInstance* node,Arena* out){
  Array<int> res = PushArray<int>(out,node->inputs.size);
  Memset(res,0);

  FOREACH_LIST(ConnectionNode*,ptr,node->allInputs){
    int port = ptr->port;
    res[port] += 1;
  }

  return res;
}

Array<Array<PortInstance>> GetAllInputs(FUInstance* node,Arena* out){
  Array<int> connections = GetNumberOfInputConnections(node,out); // Wastes a bit of memory because not deallocated after, its irrelevant

  Array<Array<PortInstance>> res = PushArray<Array<PortInstance>>(out,node->inputs.size);
  Memset(res,{});

  for(int i = 0; i < res.size; i++){
    res[i] = PushArray<PortInstance>(out,connections[i]);
  }

  FOREACH_LIST(ConnectionNode*,ptr,node->allInputs){
    int port = ptr->port;

    Array<PortInstance> array = res[port];
    int index = connections[port] - 1;
    connections[port] -= 1;
    array[index] = ptr->instConnectedTo;
  }

  return res;
}
  
void RemoveConnection(Accelerator* accel,FUInstance* out,int outPort,FUInstance* in,int inPort){
  // TODO: This is currently leaking. Need to add a free list to Accelerator.

  out->allOutputs = ListRemoveAll(out->allOutputs,[&](ConnectionNode* n){
    bool res = (n->port == outPort && n->instConnectedTo.inst == in && n->instConnectedTo.port == inPort);
    return res;
  });

  FixOutputs(out);

  in->allInputs = ListRemoveAll(in->allInputs,[&](ConnectionNode* n){
    bool res = (n->port == inPort && n->instConnectedTo.inst == out && n->instConnectedTo.port == outPort);
    return res;
  });
  FixInputs(in);
}

void RemoveAllDirectedConnections(FUInstance* out,FUInstance* in){
  out->allOutputs = ListRemoveAll(out->allOutputs,[&](ConnectionNode* n){
    return (n->instConnectedTo.inst == in);
  });
  //out->outputs = Size(out->allOutputs);
  FixOutputs(out);

  in->allInputs = ListRemoveAll(in->allInputs,[&](ConnectionNode* n){
    return (n->instConnectedTo.inst == out);
  });

  FixInputs(in);
}

void RemoveAllConnections(FUInstance* n1,FUInstance* n2){
  RemoveAllDirectedConnections(n1,n2);
  RemoveAllDirectedConnections(n2,n1);
}

void RemoveFUInstance(Accelerator* accel,FUInstance* toRemove){
  accel->allocated.Remove(toRemove);

  for(FUInstance* inst : accel->allocated){
    RemoveAllConnections(inst,toRemove);
    RemoveAllConnections(toRemove,inst);
  }
}

void InsertUnit(Accelerator* accel,PortInstance before,PortInstance after,PortInstance newUnitOutput,PortInstance newUnitInput,int delay){
  RemoveConnection(accel,before.inst,before.port,after.inst,after.port);

  // NOTE: The delay is only on the input because this is mostly used by Merge.
  ConnectUnits(newUnitOutput,after,0);
  ConnectUnits(before,newUnitInput,delay);
}

ConnectionNode* GetConnectionNode(ConnectionNode* head,int port,PortInstance other){
  FOREACH_LIST(ConnectionNode*,con,head){
    if(con->port != port){
      continue;
    }
    if(con->instConnectedTo == other){
      return con;
    }
  }

  return nullptr;
}

void MappingCheck(AcceleratorMapping* map){
  int idToCheckFirst = map->firstId;
  int idToCheckSecond = map->secondId;
  
  for(auto p : map->instanceMap){
    Assert(idToCheckFirst == p.first->accel->id);
    Assert(idToCheckSecond == p.second->accel->id);
  }
  for(auto p : map->inputMap){
    PortInstance f = p.first;
    PortInstance s = p.second;

    Assert(f.dir == Direction_INPUT);
    Assert(s.dir == Direction_INPUT);
    
    Assert(idToCheckFirst == f.inst->accel->id);
    Assert(idToCheckSecond == s.inst->accel->id);
  }

  for(auto p : map->outputMap){
    PortInstance f = p.first;
    PortInstance s = p.second;

    Assert(f.dir == Direction_OUTPUT);
    Assert(s.dir == Direction_OUTPUT);

    Assert(idToCheckFirst == f.inst->accel->id);
    Assert(idToCheckSecond == s.inst->accel->id);
  }
}

void MappingCheck(AcceleratorMapping* map,Accelerator* first,Accelerator* second){
  int firstId = first->id;
  int secondId = second->id;

  Assert(map->firstId == firstId);
  Assert(map->secondId == secondId);
}

// TODO: Need to add list to arena in order to this be good. 
static Arena mappingArenaInst = {};
static Arena* mappingArena = nullptr;

AcceleratorMapping* MappingSimple(Accelerator* first,Accelerator* second,int size,Arena* out){
  if(!mappingArena){
    mappingArenaInst = InitArena(Megabyte(64));
    mappingArena = &mappingArenaInst; 
  }
  
  AcceleratorMapping* mapping = PushStruct<AcceleratorMapping>(out);
  mapping->instanceMap = PushTrieMap<FUInstance*,FUInstance*>(mappingArena);
  mapping->inputMap = PushTrieMap<PortInstance,PortInstance>(mappingArena);
  mapping->outputMap = PushTrieMap<PortInstance,PortInstance>(mappingArena);
  mapping->firstId = first->id;   
  mapping->secondId = second->id;

  return mapping;
}

AcceleratorMapping* MappingSimple(Accelerator* first,Accelerator* second,Arena* out){
  int size = first->allocated.Size();

  return MappingSimple(first,second,size,out);
}

AcceleratorMapping* MappingInvert(AcceleratorMapping* toReverse,Arena* out){
  MappingCheck(toReverse);

  AcceleratorMapping* result = PushStruct<AcceleratorMapping>(out);
  result->instanceMap = PushTrieMap<FUInstance*,FUInstance*>(mappingArena);
  result->inputMap = PushTrieMap<PortInstance,PortInstance>(mappingArena);
  result->outputMap = PushTrieMap<PortInstance,PortInstance>(mappingArena);

  for(auto p : toReverse->instanceMap){
    result->instanceMap->Insert(p.second,p.first);
  }
  for(auto p : toReverse->inputMap){
    result->inputMap->Insert(p.second,p.first);
  }
  for(auto p : toReverse->outputMap){
    result->outputMap->Insert(p.second,p.first);
  }
  
  result->firstId = toReverse->secondId;
  result->secondId = toReverse->firstId;

  MappingCheck(result);
  
  return result;
}

AcceleratorMapping* MappingCombine(AcceleratorMapping* first,AcceleratorMapping* second,Arena* out){
  MappingCheck(first);
  MappingCheck(second);

  Assert(first->secondId == second->firstId);
  
  AcceleratorMapping* result = PushStruct<AcceleratorMapping>(out);
  result->instanceMap = PushTrieMap<FUInstance*,FUInstance*>(mappingArena);
  result->inputMap = PushTrieMap<PortInstance,PortInstance>(mappingArena);
  result->outputMap = PushTrieMap<PortInstance,PortInstance>(mappingArena);

  for(auto p : first->instanceMap){
    FUInstance* start = p.first;
    FUInstance** end = second->instanceMap->Get(p.second);
    
    if(end){
      result->instanceMap->Insert(start,*end);
    }
  }

  for(auto p : first->inputMap){
    PortInstance start = p.first;
    PortInstance* end = second->inputMap->Get(p.second);

    if(end){
      result->inputMap->Insert(start,*end);
    }
  }

  for(auto p : first->outputMap){
    PortInstance start = p.first;
    PortInstance* end = second->outputMap->Get(p.second);

    if(end){
      result->outputMap->Insert(start,*end);
    }
  }

  result->firstId = first->firstId;
  result->secondId = second->secondId;
  
  return result;
}

PortInstance MappingMapInput(AcceleratorMapping* mapping,PortInstance toMap){
  int id = toMap.inst->accel->id;

  Assert(mapping->firstId == id);

  PortInstance* ptr = mapping->inputMap->Get(toMap);
  if(!ptr){
    return {nullptr,-1};
  }
  
  return *ptr;
}

PortInstance MappingMapOutput(AcceleratorMapping* mapping,PortInstance toMap){
  int id = toMap.inst->accel->id;

  Assert(mapping->firstId == id);

  PortInstance* ptr = mapping->outputMap->Get(toMap);
  if(!ptr){
    return {nullptr,-1};
  }
  
  return *ptr;
}

void MappingInsertEqualNode(AcceleratorMapping* mapping,FUInstance* first,FUInstance* second){
  Assert(mapping->firstId == first->accel->id);
  Assert(mapping->secondId == second->accel->id);

  Assert(first->declaration == second->declaration);
  FUDeclaration* decl = first->declaration;
  int nInputs = decl->NumberInputs();
  int nOutputs = decl->NumberOutputs();
  
  mapping->instanceMap->Insert(first,second);

  for(int i = 0; i < nInputs; i++){
    mapping->inputMap->Insert(MakePortIn(first,i),MakePortIn(second,i));
  }

  for(int i = 0; i < nOutputs; i++){
    mapping->outputMap->Insert(MakePortOut(first,i),MakePortOut(second,i));
  }
}

void MappingInsertInput(AcceleratorMapping* mapping,PortInstance first,PortInstance second){
  Assert(mapping->firstId == first.inst->accel->id);
  Assert(mapping->secondId == second.inst->accel->id);

  Assert(first.dir == Direction_INPUT);
  Assert(second.dir == Direction_INPUT);
  
  mapping->instanceMap->Insert(first.inst,second.inst);
  mapping->inputMap->Insert(first,second);
}

void MappingInsertOutput(AcceleratorMapping* mapping,PortInstance first,PortInstance second){
  Assert(mapping->firstId == first.inst->accel->id);
  Assert(mapping->secondId == second.inst->accel->id);

  Assert(first.dir == Direction_OUTPUT);
  Assert(second.dir == Direction_OUTPUT);

  mapping->instanceMap->Insert(first.inst,second.inst);
  mapping->outputMap->Insert(first,second);
}

void MappingPrintInfo(AcceleratorMapping* map){
  printf("%d -> %d",map->firstId,map->secondId);
}

void MappingPrintAll(AcceleratorMapping* map){
  MappingPrintInfo(map);
  printf("\n");

  printf("Instance map:\n");
  for(Pair<FUInstance*,FUInstance*> p : map->instanceMap){
    printf("%.*s -> %.*s\n",UN(p.first->name),UN(p.second->name));
  }

  printf("Input map:\n");
  for(Pair<PortInstance,PortInstance> p : map->inputMap){
    printf("%.*s:%d(%d) -> %.*s:%d(%d)\n",UN(p.first.inst->name),p.first.port,p.first.inst->id,
                                         UN(p.second.inst->name),p.second.port,p.second.inst->id);
  }

  printf("Output map:\n");
  for(Pair<PortInstance,PortInstance> p : map->outputMap){
    printf("%.*s:%d(%d) -> %.*s:%d(%d)\n",UN(p.first.inst->name),p.first.port,p.first.inst->id,
                                         UN(p.second.inst->name),p.second.port,p.second.inst->id);
  }
}

FUInstance* MappingMapNode(AcceleratorMapping* mapping,FUInstance* inst){
  FUDeclaration* decl = inst->declaration;

  Assert(inst->accel->id == mapping->firstId);
  
  int nInputs = decl->NumberInputs();
  int nOutputs = decl->NumberOutputs();
  
  FUInstance** possibleNode = mapping->instanceMap->Get(inst);
  if(possibleNode){
    return *possibleNode;
  }
  
  FUInstance* toCheck = nullptr;
  for(int i = 0; i < nInputs; i++){
    PortInstance* optPort = mapping->inputMap->Get(MakePortIn(inst,i));

    if(optPort == nullptr){
      continue;
    }
    
    if(toCheck == nullptr){
      toCheck = optPort->inst;
    } else {
      Assert(toCheck == optPort->inst);
    }
  }

  for(int i = 0; i < nOutputs; i++){
    PortInstance* optPort = mapping->outputMap->Get(MakePortOut(inst,i));

    if(optPort == nullptr){
      continue;
    }
    
    if(toCheck == nullptr){
      toCheck = optPort->inst;
    } else {
      Assert(toCheck == optPort->inst);
    }
  }
    
  return toCheck;
}

Set<PortInstance>* MappingMapInput(AcceleratorMapping* map,Set<PortInstance>* set,Arena* out){
  Set<PortInstance>* result = PushSet<PortInstance>(out,set->map->nodesUsed);

  for(PortInstance portInst : set){
    result->Insert(MappingMapInput(map,portInst));
  }

  return result;
}

Pair<Accelerator*,SubMap*> Flatten(Accelerator* accel,int times){
  TEMP_REGION(temp,nullptr);

  Pair<Accelerator*,AcceleratorMapping*> pair = CopyAcceleratorWithMapping(accel,AcceleratorPurpose_FLATTEN,true,temp);
  Accelerator* newAccel = pair.first;
  
  Pool<FUInstance*> compositeInstances = {};
  Pool<FUInstance*> toRemove = {};

  TrieMap<StaticId,int>* staticToIndex = PushTrieMap<StaticId,int>(temp);
  SubMap* subMappingDone = PushTrieMap<SubMappingInfo,PortInstance>(globalPermanent);
  
  for(int i = 0; i < times; i++){
    for(FUInstance* instPtr : newAccel->allocated){
      FUInstance* inst = instPtr;

      bool containsStatic = false;
      containsStatic = inst->isStatic || inst->declaration->staticUnits != nullptr;
      // TODO: For now we do not flatten units that are static or contain statics. It messes merge configurations and still do not know how to procceed. Need to beef merge up a bit before handling more complex cases. The problem was that after flattening statics would become shared (unions) and as such would appear in places where they where not supposed to appear.

      // TODO: Maybe static and shared units configurations could be stored in a separated structure.
      if(inst->declaration->baseCircuit && !containsStatic){
        FUInstance** ptr = compositeInstances.Alloc();

        *ptr = instPtr;
      }
    }

    if(compositeInstances.Size() == 0){
      break;
    }

    TrieMap<int,int>* sharedToFirstChildIndex = PushTrieMap<int,int>(temp);

    int freeSharedIndex = GetFreeShareIndex(newAccel); //(maxSharedIndex != -1 ? maxSharedIndex + 1 : 0);
    int count = 0;
    for(FUInstance** instPtr : compositeInstances){
      FUInstance* inst = (*instPtr);

      Assert(inst->declaration->baseCircuit);

      count += 1;
      Accelerator* circuit = inst->declaration->baseCircuit;
      FUInstance* outputInstance = GetOutputInstance(&circuit->allocated);

      int savedSharedIndex = freeSharedIndex;
      int instSharedIndex = freeSharedIndex;
      if(inst->sharedEnable){
        // Flattening a shared unit
        int* val = sharedToFirstChildIndex->Get(inst->sharedIndex);

        if(val){
          sharedToFirstChildIndex->Insert(inst->sharedIndex,freeSharedIndex);
        } else {
          freeSharedIndex = *val;
        }
      }

      TrieMap<int,int>* sharedToShared = PushTrieMap<int,int>(temp);
      //std::unordered_map<int,int> sharedToShared;
      // Create new instance and map then
      AcceleratorMapping* map = MappingSimple(circuit,newAccel,temp); // TODO: Leaking

      for(FUInstance* ptr : circuit->allocated){
        FUInstance* circuitInst = ptr;
        if(circuitInst->declaration->type == FUDeclarationType_SPECIAL){
          continue;
        }

        String newName = PushString(globalPermanent,"%.*s_%.*s",UN(inst->name),UN(circuitInst->name));

        // We are copying the instance but some differences are gonna happen in relation to static and config sharing. 
        FUInstance* newInst = CopyInstance(newAccel,circuitInst,true,newName);
        MappingInsertEqualNode(map,circuitInst,newInst);

        if(circuitInst->isStatic){
          bool found = false;
          int shareIndex = 0;
          for(auto iter : staticToIndex){
            if(iter.first.parent == inst->declaration && CompareString(iter.first.name,circuitInst->name)){
              found = true;
              shareIndex = iter.second;
              break;
            }
          }

          if(!found){
            shareIndex = GetFreeShareIndex(newAccel);

            StaticId id = {};
            id.name = circuitInst->name;
            id.parent = inst->declaration;
            staticToIndex->Insert(id,shareIndex);
          }

          ShareInstanceConfig(newInst,shareIndex);
        } else if(circuitInst->sharedEnable && inst->sharedEnable){
          int* val = sharedToShared->Get(circuitInst->sharedIndex);

          if(val){
            ShareInstanceConfig(newInst,*val);
          } else {
            int newIndex = GetFreeShareIndex(newAccel);

            sharedToShared->Insert(circuitInst->sharedIndex,newIndex);

            ShareInstanceConfig(newInst,newIndex);
          }
        } else if(inst->sharedEnable){ // Currently flattening instance is shared
          ShareInstanceConfig(newInst,instSharedIndex); // Always use the same inst index
        } else if(circuitInst->sharedEnable){
          int* val = sharedToShared->Get(circuitInst->sharedIndex);

          if(val){
            ShareInstanceConfig(newInst,*val);
          } else {
            int newIndex = freeSharedIndex;
            
            sharedToShared->Insert(circuitInst->sharedIndex,newIndex);

            ShareInstanceConfig(newInst,newIndex);
            freeSharedIndex = GetFreeShareIndex(newAccel);
          }
        }
      }

      if(inst->sharedEnable && savedSharedIndex > freeSharedIndex){
        freeSharedIndex = savedSharedIndex;
      }

      // TODO: All these iterations could be converted to a single loop where inside we check if its input,output or inernal edge
      // Add accel edges to output instances
      for(EdgeIterator iter = IterateEdges(newAccel); iter.IsValid(); iter.Next()){
        Edge edge = iter.Value();

        if(edge.units[0].inst == inst){
          for(EdgeIterator iter2 = IterateEdges(circuit); iter2.IsValid(); iter2.Next()){
            Edge circuitEdge = iter2.Value();

            if(circuitEdge.units[1].inst == outputInstance && circuitEdge.units[1].port == edge.units[0].port){
              FUInstance* other = MappingMapOutput(map,circuitEdge.units[0]).inst;

              if(other == nullptr){
                continue;
              }

              FUInstance* out = other;
              FUInstance* in = edge.units[1].inst;
              int outPort = circuitEdge.units[0].port;
              int inPort = edge.units[1].port;
              int delay = edge.delay + circuitEdge.delay;

              SubMappingInfo t = {};

              t.subDeclaration = inst->declaration;
              t.higherName = inst->name;
              t.isInput = false;
              t.subPort = circuitEdge.units[1].port;

              subMappingDone->Insert(t,{in,inPort});
              
              ConnectUnits(out,outPort,in,inPort,delay);
            }
          }
        }
      }

      // Add accel edges to input instances
      for(EdgeIterator iter = IterateEdges(newAccel); iter.IsValid(); iter.Next()){
        Edge edge = iter.Value();

        //Edge* edge = &edgeInst;
        if(edge.units[1].inst == inst){
          FUInstance* circuitInst = GetInputInstance(&circuit->allocated,edge.units[1].port);

          for(EdgeIterator iter2 = IterateEdges(circuit); iter2.IsValid(); iter2.Next()){
            Edge circuitEdge = iter2.Value();

            if(circuitEdge.units[0].inst == circuitInst){
              FUInstance* other = MappingMapInput(map,circuitEdge.units[1]).inst;

              if(other == nullptr){
                continue;
              }

              FUInstance* out = edge.units[0].inst;
              FUInstance* in = other;
              int outPort = edge.units[0].port;
              int inPort = circuitEdge.units[1].port;
              int delay = edge.delay + circuitEdge.delay;

              SubMappingInfo t = {};

              t.subDeclaration = inst->declaration;
              t.higherName = inst->name;
              t.isInput = true;
              t.subPort = edge.units[1].port;
              
              subMappingDone->Insert(t,{out,outPort});
              
              ConnectUnits(out,outPort,in,inPort,delay);
            }
          }
        }
      }

      // Add circuit specific edges
      for(EdgeIterator iter = IterateEdges(circuit); iter.IsValid(); iter.Next()){
        Edge edge = iter.Value();

        FUInstance* otherInput = MappingMapOutput(map,edge.units[0]).inst;
        FUInstance* otherOutput = MappingMapInput(map,edge.units[1]).inst;

        if(otherInput == nullptr || otherOutput == nullptr){
          continue;
        }

        FUInstance* out = otherInput;
        FUInstance* in = otherOutput;
        int outPort = edge.units[0].port;
        int inPort = edge.units[1].port;
        int delay = edge.delay;

        ConnectUnits(out,outPort,in,inPort,delay);
      }

      // Add input to output specific edges
      for(EdgeIterator iter = IterateEdges(newAccel); iter.IsValid(); iter.Next()){
        Edge edge1 = iter.Value();

        if(edge1.units[1].inst == inst){
          PortInstance input = edge1.units[0];
          FUInstance* circuitInput = GetInputInstance(&circuit->allocated,edge1.units[1].port);

          for(EdgeIterator iter2 = IterateEdges(newAccel); iter2.IsValid(); iter2.Next()){
            Edge edge2 = iter2.Value();

            if(edge2.units[0].inst == inst){
              PortInstance output = edge2.units[1];
              int outputPort = edge2.units[0].port;

              for(EdgeIterator iter3 = IterateEdges(newAccel); iter3.IsValid(); iter3.Next()){
                Edge circuitEdge = iter3.Value();

                if(circuitEdge.units[0].inst == circuitInput
                   && circuitEdge.units[1].inst == outputInstance
                   && circuitEdge.units[1].port == outputPort){
                  int delay = edge1.delay + circuitEdge.delay + edge2.delay;

                  ConnectUnits(input,output,delay);
                }
              }
            }
          }
        }
      }

      RemoveFUInstance(newAccel,*instPtr);
    }

    toRemove.Clear();
    compositeInstances.Clear();
  }

  toRemove.Clear(true);
  compositeInstances.Clear(true);

  newAccel->name = accel->name;
  
  return {newAccel,subMappingDone};
}

void PrintSubMappingInfo(SubMap* info){
  for(Pair<SubMappingInfo,PortInstance> f : info){
    SubMappingInfo map = f.first;
    PortInstance inst = f.second;
    printf("%.*s",UN(map.subDeclaration->name));
    printf("::%.*s:%d %s\n",UN(map.higherName),map.subPort,map.isInput ? "in" : "out");
    printf("%.*s:%d\n",UN(inst.inst->name),inst.port);
    printf("\n");
  }
}

#include "configurations.hpp"

#include "debug.hpp"
#include "declaration.hpp"
#include "memory.hpp"
#include "parser.hpp"
#include "utils.hpp"
#include "utilsCore.hpp"
#include "versat.hpp"
#include "textualRepresentation.hpp"
#include <strings.h>

CalculatedOffsets CalculateConfigOffsetsIgnoringStatics(Accelerator* accel,Arena* out){
  int size = Size(accel->allocated);

  Array<int> array = PushArray<int>(out,size);

  BLOCK_REGION(out);

  Hashmap<int,int>* sharedConfigs = PushHashmap<int,int>(out,size);

  int index = 0;
  int offset = 0;
  FOREACH_LIST_INDEXED(InstanceNode*,ptr,accel->allocated,index){
    FUInstance* inst = ptr->inst;
    Assert(!(inst->sharedEnable && inst->isStatic));

    int numberConfigs = inst->declaration->baseConfig.configs.size;
    if(numberConfigs == 0){
      array[index] = -1;
      continue;
    }
    
    if(inst->sharedEnable){
      int sharedIndex = inst->sharedIndex;
      GetOrAllocateResult<int> possibleAlreadyShared = sharedConfigs->GetOrAllocate(sharedIndex);

      if(possibleAlreadyShared.alreadyExisted){
        array[index] = *possibleAlreadyShared.data;
        continue;
      } else {
        *possibleAlreadyShared.data = offset;
      }
    }
    
    if(inst->isStatic){
      array[index] = -1; // TODO: Changed this, make sure no bug occurs from this
      //array[index] = 0x40000000;
    } else {
      array[index] = offset;
      offset += numberConfigs;
    }
  }

  CalculatedOffsets res = {};
  res.offsets = array;
  res.max = offset;

  return res;
}

int GetConfigurationSize(FUDeclaration* decl,MemType type){
  int size = 0;

  switch(type){
  case MemType::CONFIG:{
    size = decl->baseConfig.configs.size;
  }break;
  case MemType::DELAY:{
    size = decl->baseConfig.delayOffsets.max;
  }break;
  case MemType::STATE:{
    size = decl->baseConfig.states.size;
  }break;
  case MemType::STATIC:{
  }break;
  default: NOT_IMPLEMENTED("Implemented as needed");
  }

  return size;
}

CalculatedOffsets CalculateConfigurationOffset(Accelerator* accel,MemType type,Arena* out){
  if(type == MemType::CONFIG){
    return CalculateConfigOffsetsIgnoringStatics(accel,out);
  }

  Array<int> array = PushArray<int>(out,Size(accel->allocated));

  BLOCK_REGION(out);

  int index = 0;
  int offset = 0;
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    FUInstance* inst = ptr->inst;
    array[index] = offset;

    int size = GetConfigurationSize(inst->declaration,type);

    offset += size;
    index += 1;
  }

  CalculatedOffsets res = {};
  res.offsets = array;
  res.max = offset;

  return res;
}

Array<Wire> ExtractAllConfigs(Array<InstanceInfo> info,Arena* out,Arena* temp){
  BLOCK_REGION(temp);

  ArenaList<Wire>* list = PushArenaList<Wire>(temp);
  Set<StaticId>* seen = PushSet<StaticId>(temp,99);

  // TODO: This implementation of skipLevel is a bit shabby.
  int skipLevel = -1;
  for(InstanceInfo& t : info){
    if(t.level <= skipLevel){
      skipLevel = -1;
    }
    if(skipLevel != -1 && t.level > skipLevel){
      continue;
    }

    if(t.isStatic){
      skipLevel = t.level;
    }

    if(!t.isComposite && !t.isStatic){
      for(int i = 0; i < t.configSize; i++){
        Wire* wire = PushListElement(list);

        *wire = t.decl->baseConfig.configs[i];
        wire->name = PushString(out,"%.*s_%.*s",UNPACK_SS(t.fullName),UNPACK_SS(t.decl->baseConfig.configs[i].name));
      }
    }
  }
  skipLevel = -1;
  for(InstanceInfo& t : info){
    if(t.level <= skipLevel){
      skipLevel = -1;
    }
    if(skipLevel != -1 && t.level > skipLevel){
      continue;
    }
      
    if(t.parent && t.isStatic){
      FUDeclaration* parent = t.parent;
      String parentName = parent->name;

      StaticId id = {};
      id.parent = parent;
      id.name = t.name;
      if(!seen->Exists(id)){
        int skipLevel = t.level;
        seen->Insert(id);
        for(int i = 0; i < t.configSize; i++){
          Wire* wire = PushListElement(list);

          *wire = t.decl->baseConfig.configs[i];
          wire->name = PushString(out,"%.*s_%.*s_%.*s",UNPACK_SS(parentName),UNPACK_SS(t.name),UNPACK_SS(t.decl->baseConfig.configs[i].name));
        }
      }
    }
  }

  for(int i = 0; i < info[0].delaySize; i++){
    Wire* wire = PushListElement(list);

    wire->name = PushString(out,"TOP_Delay%d",i);
    wire->bitSize = 32;
  }
  
  return PushArrayFromList<Wire>(out,list);
}

Array<String> ExtractStates(Array<InstanceInfo> info,Arena* out){
  int count = 0;
  for(InstanceInfo& in : info){
    if(!in.isComposite && in.statePos.has_value()){
      count += in.decl->baseConfig.states.size;
    }
  }

  Array<String> res = PushArray<String>(out,count);
  int index = 0;
  for(InstanceInfo& in : info){
    if(!in.isComposite && in.statePos.has_value()){
      FUDeclaration* decl = in.decl;
      for(Wire& wire : decl->baseConfig.states){
        res[index++] = PushString(out,"%.*s_%.*s",UNPACK_SS(in.fullName),UNPACK_SS(wire.name)); 
      }
    }
  }

  return res;
}

Array<Pair<String,int>> ExtractMem(Array<InstanceInfo> info,Arena* out){
  int count = 0;
  for(InstanceInfo& in : info){
    if(!in.isComposite && in.memMapped.has_value()){
      count += 1;
    }
  }

  Array<Pair<String,int>> res = PushArray<Pair<String,int>>(out,count);
  int index = 0;
  for(InstanceInfo& in : info){
    if(!in.isComposite && in.memMapped.has_value()){
      FUDeclaration* decl = in.decl;
      String name = PushString(out,"%.*s_addr",UNPACK_SS(in.fullName)); 
      res[index++] = {name,(int) in.memMapped.value()};
    }
  }

  return res;
}

void ShareInstanceConfig(FUInstance* inst, int shareBlockIndex){
  inst->sharedIndex = shareBlockIndex;
  inst->sharedEnable = true;
}

void SetStatic(Accelerator* accel,FUInstance* inst){
  inst->isStatic = true;
}

String ReprStaticConfig(StaticId id,Wire* wire,Arena* out){
  String identifier = PushString(out,"%.*s_%.*s_%.*s",UNPACK_SS(id.parent->name),UNPACK_SS(id.name),UNPACK_SS(wire->name));

  return identifier;
}

// TODO: There is probably a better way of simplifying the logic used here.
// NOTE: The logic for static and shared configs is hard to decouple and simplify. I found no other way of doing this, other than printing the result for a known set of circuits and make small changes at atime.

bool Next(Array<Partition> arr){
  for(int i = 0; i < arr.size; i++){
    Partition& par = arr[i];

    if(par.value + 1 >= par.max){
      continue;
    } else {
      arr[i].value = arr[i].value + 1;
      for(int ii = 0; ii < i; ii++){
        arr[ii].value = 0;
      }
      return true;
    }
  }

  return false;
}

Array<Partition> GenerateInitialPartitions(Accelerator* accel,Arena* out){
  DynamicArray<Partition> partitionsArr = StartArray<Partition>(out);
  int mergedPossibility = 0;
  FOREACH_LIST(InstanceNode*,node,accel->allocated){
    FUInstance* subInst = node->inst;
    FUDeclaration* decl = subInst->declaration;

    if(subInst->declaration->configInfo.size > 1){
      mergedPossibility += log2i(decl->configInfo.size);
      *partitionsArr.PushElem() = (Partition){.value = 0,.max = decl->configInfo.size};
    }
  }
  Array<Partition> partitions = EndArray(partitionsArr);
  return partitions;
}

bool IncrementSinglePartition(Partition* part){
  if(part->value + 1 < part->max){
    part->value += 1;
    return true;
  } else {
    part->value = 0;
    return false;
  }
}

void IncrementPartitions(Array<Partition> partitions,int amount){
  for(int i = 0; i < amount; i++){
    for(Partition& part : partitions){
      if(IncrementSinglePartition(&part)){
        break;
      }
    }
  }
}

static InstanceInfo GetInstanceInfo(InstanceNode* node,FUDeclaration* parentDeclaration,InstanceConfigurationOffsets offsets,Arena* out){
  FUInstance* inst = node->inst;
  String topName = inst->name;
  if(offsets.topName.size != 0){
    topName = PushString(out,"%.*s_%.*s",UNPACK_SS(offsets.topName),UNPACK_SS(inst->name));
  }
    
  FUDeclaration* topDecl = inst->declaration;
  InstanceInfo info = {};
  info = {};

  info.name = inst->name;
  info.baseName = offsets.baseName;
  info.fullName = topName;
  info.decl = topDecl;
  info.isStatic = inst->isStatic; 
  info.isShared = inst->sharedEnable;
  info.level = offsets.level;
  info.isComposite = IsTypeHierarchical(topDecl);
  info.parent = parentDeclaration;
  info.baseDelay = offsets.delay;
  info.isMergeMultiplexer = inst->isMergeMultiplexer;
  info.belongs = offsets.belongs;
  info.special = inst->literal;
  info.order = offsets.order;
  info.connectionType = node->type;

  Assert(info.baseDelay >= 0);
  
  if(topDecl->memoryMapBits.has_value()){
    info.memMappedSize = (1 << topDecl->memoryMapBits.value());
    info.memMappedBitSize = topDecl->memoryMapBits.value();
    info.memMapped = AlignBitBoundary(offsets.memOffset,topDecl->memoryMapBits.value());
    info.memMappedMask = BinaryRepr(info.memMapped.value(),32,out);
  }

  info.configSize = topDecl->baseConfig.configs.size;
  info.stateSize = topDecl->baseConfig.states.size;
  info.delaySize = topDecl->baseConfig.delayOffsets.max;

  bool containsConfigs = info.configSize;
  bool configIsStatic = false;
  if(containsConfigs){
    if(inst->isStatic){
      configIsStatic = true;
      StaticId id = {offsets.parent,inst->name};
      if(offsets.staticInfo->Exists(id)){
        info.configPos = offsets.staticInfo->GetOrFail(id);
      } else {
        int config = *offsets.staticConfig;

        offsets.staticInfo->Insert(id,config);
        info.configPos = config;

        *offsets.staticConfig += info.configSize;
      }
    } else if(offsets.configOffset >= 0x40000000){
      configIsStatic = true;
      info.configPos = offsets.configOffset;
    } else {
      info.configPos = offsets.configOffset;
    } 
  }

  if(topDecl->baseConfig.states.size){
    info.statePos = offsets.stateOffset;
  }

  if(topDecl->baseConfig.delayOffsets.max){
    info.delayPos = offsets.delayOffset;
  }

  // ConfigPos being null has meaning and in some portions of the code indicates wether the unit belongs or not.
  // TODO: This part is very confusing. Code should depend directly on belongs.
  if(!offsets.belongs){
    info.configPos = {}; // The whole code is ugly, rewrite after putting everything working
  }
    
  info.isConfigStatic = configIsStatic;
  
  int nDelays = topDecl->baseConfig.delayOffsets.max;
  if(nDelays > 0 && !info.isComposite){
    info.delay = PushArray<int>(out,nDelays);
    Assert(offsets.delay >= 0);
    Memset(info.delay,offsets.delay);
  }
  
  return info;
}

AcceleratorInfo TransformGraphIntoArrayRecurse(InstanceNode* node,FUDeclaration* parentDecl,InstanceConfigurationOffsets offsets,Opt<Partition> partition,Arena* out,Arena* temp){
  // This prevents us from fully using arenas because we are pushing more data than the actual structures that we keep track of. If this was hierarchical, we would't have this problem.
  AcceleratorInfo res = {};

  FUInstance* inst = node->inst;
  InstanceInfo top = GetInstanceInfo(node,parentDecl,offsets,out);
  
  FUDeclaration* topDecl = inst->declaration;
  if(!IsTypeHierarchical(topDecl)){
    Array<InstanceInfo> arr = PushArray<InstanceInfo>(out,1);
    arr[0] = top;
    res.info = arr;
    res.memSize = top.memMappedSize.value_or(0);
    return res;
  }

  ArenaList<InstanceInfo>* instanceList = PushArenaList<InstanceInfo>(out); 
  *PushListElement(instanceList) = top;

  if(top.memMapped.has_value()){
    offsets.memOffset = top.memMapped.value();
  }
  // Composite instance
  int delayIndex = 0;
  int index = 0;

  int part = 0;
  Array<Partition> subPartitions = {};
  if(partition.has_value()){
    part = partition.value().value;
    subPartitions = GenerateInitialPartitions(inst->declaration->fixedDelayCircuit,temp);
    IncrementPartitions(subPartitions,part);
  }
  
  bool changedConfigFromStatic = false;
  int savedConfig = offsets.configOffset;
  int subOffsetsConfig = offsets.configOffset;
  if(inst->isStatic){
    changedConfigFromStatic = true;
    StaticId id = {offsets.parent,inst->name};
    if(offsets.staticInfo->Exists(id)){
      subOffsetsConfig = offsets.staticInfo->GetOrFail(id);
    } else if(offsets.configOffset <= 0x40000000){
      int config = *offsets.staticConfig;
      *offsets.staticConfig += inst->declaration->configInfo[part].configOffsets.offsets[index];

      offsets.staticInfo->Insert(id,config);

      subOffsetsConfig = config;
    } else {
      changedConfigFromStatic = false; // Already inside a static, act as if it was in a non static context
    }
  }

  int partitionIndex = 0;
  FOREACH_LIST_INDEXED(InstanceNode*,subNode,inst->declaration->fixedDelayCircuit->allocated,index){
    FUInstance* subInst = subNode->inst;
    bool containsConfig = subInst->declaration->baseConfig.configs.size; // TODO: When  doing partition might need to put index here instead of 0
    
    InstanceConfigurationOffsets subOffsets = offsets;
    subOffsets.level += 1;
    subOffsets.topName = top.fullName;
    subOffsets.parent = topDecl;
    subOffsets.configOffset = subOffsetsConfig;
    
    // Bunch of static logic
    subOffsets.configOffset += inst->declaration->configInfo[part].configOffsets.offsets[index];
    subOffsets.stateOffset += inst->declaration->configInfo[part].stateOffsets.offsets[index];
    subOffsets.delayOffset += inst->declaration->configInfo[part].delayOffsets.offsets[index];
    subOffsets.belongs = inst->declaration->configInfo[part].unitBelongs[index];
    subOffsets.baseName = inst->declaration->configInfo[part].baseName[index];
    subOffsets.order = inst->declaration->configInfo[part].order[index];
    
    if(subInst->declaration->baseConfig.delayOffsets.max > 0){
      subOffsets.delay = offsets.delay + inst->declaration->configInfo[part].calculatedDelays[index];
    }

    if(subInst->declaration->memoryMapBits.has_value()){
      res.memSize = AlignBitBoundary(res.memSize,subInst->declaration->memoryMapBits.value());
    }

    subOffsets.memOffset = offsets.memOffset + res.memSize;

    Opt<Partition> part = {};
    if(subInst->declaration->configInfo.size > 1){
      part = subPartitions[partitionIndex++];
    }

    AcceleratorInfo info = TransformGraphIntoArrayRecurse(subNode,inst->declaration,subOffsets,part,temp,out);
    
    res.memSize += info.memSize;
    
    for(InstanceInfo& f : info.info){
      *PushListElement(instanceList) = f;
    }
  }

  res.info = PushArrayFromList(out,instanceList);
  res.memSize = top.memMappedSize.value_or(0);

  Assert(part <= inst->declaration->configInfo.size);
  res.name = inst->declaration->configInfo[part].name;
  
  return res;
}

// For now I'm going to make this function return everything

// TODO: This should call all the Calculate* functions and then put everything together.
//       We are complication this more than needed. We already have all the logic needed to do this.
//       The only thing that we actually need to do is to recurse, access data from configInfo (or calculate if at the top) and them put them all together.

// TODO: Offer a recursive option and a non recursive option. 

// NOTE: I could: Have a function that recurses and counts how many nodes there are.
//       Allocates the Array of instance info.
//       We can then fill everything by computing configInfo and stuff at the top and accessing fudeclaration configInfos as we go down.

TestResult CalculateOneInstance(Accelerator* accel,bool recursive,Array<Partition> partitions,Arena* out,Arena* temp){
  ArenaList<InstanceInfo>* infoList = PushArenaList<InstanceInfo>(temp);
  Hashmap<StaticId,int>* staticInfo = PushHashmap<StaticId,int>(temp,99);

  // TODO: This is being recalculated multiple times if we have various partitions. Move out when this function starts stabilizing.
  CalculatedOffsets configOffsets = CalculateConfigOffsetsIgnoringStatics(accel,out);
  CalculatedOffsets delayOffsets = CalculateConfigurationOffset(accel,MemType::DELAY,out);

  DAGOrderNodes order = CalculateDAGOrder(accel->allocated,temp);
  CalculateDelayResult calculatedDelay = CalculateDelay(accel,order,partitions,temp);
  
  Hashmap<InstanceNode*,int>* toOrder = MapElementToIndex(order.instances,temp);

  Array<InstanceNode*> inputNodes = PushArray<InstanceNode*>(temp,99);

  Array<int> inputDelays = ExtractInputDelays(accel,calculatedDelay,0,out,temp);
  Array<int> outputLatencies = ExtractOutputLatencies(accel,calculatedDelay,out,temp);
  
  int partitionIndex = 0;
  int staticConfig = 0x40000000; // TODO: Make this more explicit
  int index = 0;
  InstanceConfigurationOffsets subOffsets = {};
  subOffsets.topName = {};
  subOffsets.staticConfig = &staticConfig;
  subOffsets.staticInfo = staticInfo;
  subOffsets.belongs = true;
  ArenaList<String>* names = PushArenaList<String>(temp);
  FOREACH_LIST_INDEXED(InstanceNode*,node,accel->allocated,index){
    FUInstance* subInst = node->inst;

    Opt<Partition> part = {};
    if(subInst->declaration->configInfo.size > 1){
      part = partitions[partitionIndex];
    }
    
    subOffsets.configOffset = configOffsets.offsets[index];
    subOffsets.delayOffset = delayOffsets.offsets[index];
    subOffsets.delay = calculatedDelay.nodeDelay->GetOrFail(node);
    subOffsets.order = toOrder->GetOrFail(node);
    
    if(recursive){
      AcceleratorInfo info = TransformGraphIntoArrayRecurse(node,nullptr,subOffsets,part,temp,out);

      if(info.name.has_value()){
        String elem = info.name.value();

        if(!IsOnlyWhitespace(elem)){
          *names->PushElem() = elem;
        }
      }

      subOffsets.memOffset += info.memSize;
      subOffsets.memOffset = AlignBitBoundary(subOffsets.memOffset,log2i(info.memSize));
      
      for(InstanceInfo& f : info.info){
        *PushListElement(infoList) = f;
      }
    } else {
      InstanceInfo top = GetInstanceInfo(node,nullptr,subOffsets,out);

      subOffsets.memOffset += top.memMappedSize.value_or(0);
      subOffsets.memOffset = AlignBitBoundary(subOffsets.memOffset,log2i(top.memMappedSize.value_or(1)));
      
      *PushListElement(infoList) = top;
    }

    if(subInst->declaration->configInfo.size > 1){
      partitionIndex += 1;
    }
  }
  Array<InstanceInfo> res = PushArrayFromList<InstanceInfo>(out,infoList);
  TestResult result = {};
  result.info = res;
  result.subOffsets = subOffsets;
  result.inputDelay = inputDelays;
  result.outputLatencies = outputLatencies;
  
  Array<String> nameList = PushArrayFromList(temp,names);
  
  auto mark = StartString(out);
  bool first = true;
  for(String name : nameList){
    if(first){
      first = false;
    } else {
      PushString(out,"_");
    }
    PushString(out,name);
  }
  result.name = EndString(mark);
  

  return result;
}

// TODO: Move this function to a better place
// This function cannot return an array for merge units because we do not have the merged units info in this level.
// This function only works for modules and for recursing the merged info to upper modules.
// The info needed by merge must be stored by the merge function.

AccelInfo CalculateAcceleratorInfo(Accelerator* accel,bool recursive,Arena* out,Arena* temp){
  // TODO: Have this function return everything that needs to be arraified  
  //       Printing a GraphArray struct should tell me everything I need to know about the accelerator
  AccelInfo result = {};

  //BLOCK_REGION(temp); MARKED - Some bug occurs, probably because the function is a mess and we have no idea 
  Array<Partition> partitions = GenerateInitialPartitions(accel,temp);

  int totalMaxBitsize = 0;
  ArenaList<TestResult>* list = PushArenaList<TestResult>(temp);
  Array<InstanceInfo> res = {};
  InstanceConfigurationOffsets subOffsets = {};
  bool first = true;
  int index = 0;
  while(1){
    TestResult res2 = CalculateOneInstance(accel,recursive,partitions,out,temp);
    *PushListElement(list) = res2;

    if(first){
      res = res2.info;
      subOffsets = res2.subOffsets;
    }
  
    int maxMemBitsize = log2i(subOffsets.memOffset);
    if(maxMemBitsize > 0){
      for(InstanceInfo& info : res2.info){
        if(info.memMapped.has_value()){
          String& memMask = info.memMappedMask.value();

          memMask.data += 32 - maxMemBitsize;
          memMask.size = maxMemBitsize;
        }
      }
    }

    if(first){
      totalMaxBitsize = maxMemBitsize;
    }
    
    FUDeclaration* maxConfigDecl = nullptr;
    int maxConfig = 0;
    for(InstanceInfo& inst : res2.info){
      if(inst.configPos.has_value()){
        int val = inst.configPos.value();
        if(val < 0x40000000){
          if(val > maxConfig){
            maxConfig = val;
            maxConfigDecl = inst.decl;
          }
        }
      }
    }

    if(maxConfigDecl){
      int totalConfigSize = maxConfig + maxConfigDecl->baseConfig.configOffsets.max;
      for(InstanceInfo& inst : res2.info){
        if(inst.configPos.has_value()){
          int val = inst.configPos.value();
          if(val >= 0x40000000){
            inst.configPos = val - 0x40000000 + totalConfigSize;
          }
        }
      }
    }

    first = false;
    if(!Next(partitions)){
      break;
    }
  }
  Array<TestResult> final = PushArrayFromList(out,list);

  result.infos = Map(final,out,[](TestResult t){
    return t.info;
  });
  result.names = Map(final,out,[](TestResult t){
    return t.name;
  });
  result.inputDelays = Map(final,out,[](TestResult t){
    return t.inputDelay;
  });
  result.outputDelays = Map(final,out,[](TestResult t){
    return t.outputLatencies;
  });

  // Merge everything back into a single unique view.
  Array<InstanceInfo> unique = result.infos[0];
  result.baseInfo = PushArray<InstanceInfo>(out,unique.size);
  for(int i = 0; i < result.baseInfo.size; i++){
    result.baseInfo[i] = unique[i]; // Copy default values

    result.baseInfo[i].belongs = true; // All units belong to baseInfo
    result.baseInfo[i].baseName = result.baseInfo[i].name;
    
    for(Array<InstanceInfo> infos : result.infos){
      if(infos[i].configPos.has_value()){
        result.baseInfo[i].configPos = infos[i].configPos.value();
        break;
      }
    }
    for(Array<InstanceInfo> infos : result.infos){
      if(infos[i].statePos.has_value()){
        result.baseInfo[i].statePos = infos[i].statePos.value();
        break;
      }
    }
    for(Array<InstanceInfo> infos : result.infos){
      if(infos[i].memMapped.has_value()){
        result.baseInfo[i].memMapped = infos[i].memMapped.value();
        break;
      }
    }
    for(Array<InstanceInfo> infos : result.infos){
      if(infos[i].memMappedSize.has_value()){
        result.baseInfo[i].memMappedSize = infos[i].memMappedSize.value();
        break;
      }
    }
    for(Array<InstanceInfo> infos : result.infos){
      if(infos[i].memMappedBitSize.has_value()){
        result.baseInfo[i].memMappedBitSize = infos[i].memMappedBitSize.value();
        break;
      }
    }
    for(Array<InstanceInfo> infos : result.infos){
      if(infos[i].memMappedMask.has_value()){
        result.baseInfo[i].memMappedMask = infos[i].memMappedMask.value();
        break;
      }
    }
    for(Array<InstanceInfo> infos : result.infos){
      if(infos[i].delayPos.has_value()){
        result.baseInfo[i].delayPos = infos[i].delayPos.value();
        break;
      }
    }
  }
  
  result.memMappedBitsize = totalMaxBitsize;

  std::vector<bool> seenShared;

  Hashmap<StaticId,int>* staticSeen = PushHashmap<StaticId,int>(temp,1000);

  int memoryMappedDWords = 0;

  // Handle non-static information
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    FUInstance* inst = ptr->inst;
    FUDeclaration* type = inst->declaration;

    // Check if shared
    if(inst->sharedEnable){
      if(inst->sharedIndex >= (int) seenShared.size()){
        seenShared.resize(inst->sharedIndex + 1);
      }

      if(!seenShared[inst->sharedIndex]){
        result.configs += type->baseConfig.configs.size;
        result.sharedUnits += 1;
      }

      seenShared[inst->sharedIndex] = true;
    } else if(!inst->isStatic){ // Shared cannot be static
         result.configs += type->baseConfig.configs.size;
    }

    if(type->memoryMapBits.has_value()){
      result.isMemoryMapped = true;

      memoryMappedDWords = AlignBitBoundary(memoryMappedDWords,type->memoryMapBits.value());
      memoryMappedDWords += (1 << type->memoryMapBits.value());
    }

    if(type == BasicDeclaration::input){
      result.inputs += 1;
    }

    result.states += type->baseConfig.states.size;
    result.delays += type->baseConfig.delayOffsets.max;
    result.ios += type->nIOs;

    if(type->externalMemory.size){
      result.externalMemoryInterfaces += type->externalMemory.size;
	  result.externalMemoryByteSize = ExternalMemoryByteSize(type->externalMemory);
    }

    result.signalLoop |= type->signalLoop;
  }

  result.memoryMappedBits = log2i(memoryMappedDWords);

  FUInstance* outputInstance = GetOutputInstance(accel->allocated);
  if(outputInstance){
    FOREACH_LIST(Edge*,edge,accel->edges){
      if(edge->units[0].inst == outputInstance){
        result.outputs = std::max(result.outputs - 1,edge->units[0].port) + 1;
      }
      if(edge->units[1].inst == outputInstance){
        result.outputs = std::max(result.outputs - 1,edge->units[1].port) + 1;
      }
    }
  }

  // Handle static information
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    FUInstance* inst = ptr->inst;
    if(inst->isStatic){
      StaticId id = {};

      id.name = inst->name;

      staticSeen->Insert(id,1);
      result.statics += inst->declaration->baseConfig.configs.size;
    }

    if(IsTypeHierarchical(inst->declaration)){
      for(Pair<StaticId,StaticData> pair : inst->declaration->staticUnits){
        int* possibleFind = staticSeen->Get(pair.first);

        if(!possibleFind){
          staticSeen->Insert(pair.first,1);
          result.statics += pair.second.configs.size;
        }
      }
    }
  }
  
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    FUInstance* inst = ptr->inst;
    result.numberConnections += Size(ptr->allOutputs);
  }
  
  return result;
}

Array<InstanceInfo> ExtractFromInstanceInfoSameLevel(Array<InstanceInfo> instanceInfo,int level,Arena* out){
  DynamicArray<InstanceInfo> arr = StartArray<InstanceInfo>(out);
  for(InstanceInfo& info : instanceInfo){
    if(info.level == level){
      *arr.PushElem() = info;
    }
  }

  return EndArray(arr);
}

void CheckSanity(Array<InstanceInfo> instanceInfo,Arena* temp){
  // TODO: Add more conditions here as bugs appear that break this

  Array<bool> inputsSeen = PushArray<bool>(temp,999);
  Memset(inputsSeen,false);

  bool outputSeen = false;

  BLOCK_REGION(temp);
  for(int i = 0; ; i++){
    Array<InstanceInfo> sameLevel = ExtractFromInstanceInfoSameLevel(instanceInfo,i,temp);
    
    if(sameLevel.size == 0){
      break;
    }

    if(i == 0){
      for(InstanceInfo& info : sameLevel){
        if(info.decl == BasicDeclaration::input){
          Assert(!inputsSeen[info.special]);
          inputsSeen[info.special] = true;
        }
        if(info.decl == BasicDeclaration::output){
          // TODO: For now this does not work because merge joins names together, causing out to become out_out and so on.
          //Assert(CompareString(info.name,"out"));
          Assert(!outputSeen);
          outputSeen = true;
        }
      }
        
    }
    
    // All same level instances must continuously increase mem mapped values 
    int lastMem = -1;
    for(InstanceInfo& info : sameLevel){
      if(info.memMapped.has_value()){
        int mem = info.memMapped.value(); 
        Assert(mem > lastMem);
        lastMem = info.memMapped.value();
      }
    }
  }
}

void PrintConfigurations(FUDeclaration* type,Arena* temp){
  ConfigurationInfo& info = type->configInfo[0];

  printf("Config:\n");
  for(Wire& wire : info.configs){
    BLOCK_REGION(temp);

    String str = Repr(&wire,temp);
    printf("%.*s\n",UNPACK_SS(str));
  }

  printf("State:\n");
  for(Wire& wire : info.states){
    BLOCK_REGION(temp);

    String str = Repr(&wire,temp);
    printf("%.*s\n",UNPACK_SS(str));
  }
  printf("\n");
}


// HACK
// HACK
// HACK
// HACK
// HACK
// HACK


static InstanceInfo GetInstanceInfoNoDelay(InstanceNode* node,FUDeclaration* parentDeclaration,InstanceConfigurationOffsets offsets,Arena* out){
  FUInstance* inst = node->inst;
  String topName = inst->name;
  if(offsets.topName.size != 0){
    topName = PushString(out,"%.*s_%.*s",UNPACK_SS(offsets.topName),UNPACK_SS(inst->name));
  }
    
  FUDeclaration* topDecl = inst->declaration;
  InstanceInfo info = {};
  info = {};

  info.name = inst->name;
  info.baseName = offsets.baseName;
  info.fullName = topName;
  info.decl = topDecl;
  info.isStatic = inst->isStatic; 
  info.isShared = inst->sharedEnable;
  info.level = offsets.level;
  info.isComposite = IsTypeHierarchical(topDecl);
  info.parent = parentDeclaration;
  info.baseDelay = offsets.delay;
  info.isMergeMultiplexer = inst->isMergeMultiplexer;
  info.belongs = offsets.belongs;
  info.special = inst->literal;
  info.connectionType = node->type;

  Assert(info.baseDelay >= 0);
  
  if(topDecl->memoryMapBits.has_value()){
    info.memMappedSize = (1 << topDecl->memoryMapBits.value());
    info.memMappedBitSize = topDecl->memoryMapBits.value();
    info.memMapped = AlignBitBoundary(offsets.memOffset,topDecl->memoryMapBits.value());
    info.memMappedMask = BinaryRepr(info.memMapped.value(),32,out);
  }

  info.configSize = topDecl->baseConfig.configs.size;
  info.stateSize = topDecl->baseConfig.states.size;
  info.delaySize = topDecl->baseConfig.delayOffsets.max;

  bool containsConfigs = info.configSize;
  bool configIsStatic = false;
  if(containsConfigs){
    if(inst->isStatic){
      configIsStatic = true;
      StaticId id = {offsets.parent,inst->name};
      if(offsets.staticInfo->Exists(id)){
        info.configPos = offsets.staticInfo->GetOrFail(id);
      } else {
        int config = *offsets.staticConfig;

        offsets.staticInfo->Insert(id,config);
        info.configPos = config;

        *offsets.staticConfig += info.configSize;
      }
    } else if(offsets.configOffset >= 0x40000000){
      configIsStatic = true;
      info.configPos = offsets.configOffset;
    } else {
      info.configPos = offsets.configOffset;
    } 
  }

  if(topDecl->baseConfig.states.size){
    info.statePos = offsets.stateOffset;
  }

  if(topDecl->baseConfig.delayOffsets.max){
    info.delayPos = offsets.delayOffset;
  }

  // ConfigPos being null has meaning and in some portions of the code indicates wether the unit belongs or not.
  // TODO: This part is very confusing. Code should depend directly on belongs.
  if(!offsets.belongs){
    info.configPos = {}; // The whole code is ugly, rewrite after putting everything working
  }
    
  info.isConfigStatic = configIsStatic;
 
  return info;
}

AcceleratorInfo TransformGraphIntoArrayRecurseNoDelay(InstanceNode* node,FUDeclaration* parentDecl,InstanceConfigurationOffsets offsets,Partition partition,Arena* out,Arena* temp){
  // This prevents us from fully using arenas because we are pushing more data than the actual structures that we keep track of. If this was hierarchical, we would't have this problem.
  AcceleratorInfo res = {};

  FUInstance* inst = node->inst;
  InstanceInfo top = GetInstanceInfoNoDelay(node,parentDecl,offsets,out);
  
  FUDeclaration* topDecl = inst->declaration;
  if(!IsTypeHierarchical(topDecl)){
    Array<InstanceInfo> arr = PushArray<InstanceInfo>(out,1);
    arr[0] = top;
    res.info = arr;
    res.memSize = top.memMappedSize.value_or(0);
    return res;
  }

  ArenaList<InstanceInfo>* instanceList = PushArenaList<InstanceInfo>(out); 
  *PushListElement(instanceList) = top;

  if(top.memMapped.has_value()){
    offsets.memOffset = top.memMapped.value();
  }
  // Composite instance
  int delayIndex = 0;
  int index = 0;

  int part = partition.value;
  
  bool changedConfigFromStatic = false;
  int savedConfig = offsets.configOffset;
  int subOffsetsConfig = offsets.configOffset;
  if(inst->isStatic){
    changedConfigFromStatic = true;
    StaticId id = {offsets.parent,inst->name};
    if(offsets.staticInfo->Exists(id)){
      subOffsetsConfig = offsets.staticInfo->GetOrFail(id);
    } else if(offsets.configOffset <= 0x40000000){
      int config = *offsets.staticConfig;
      *offsets.staticConfig += inst->declaration->configInfo[part].configOffsets.offsets[index];

      offsets.staticInfo->Insert(id,config);

      subOffsetsConfig = config;
    } else {
      changedConfigFromStatic = false; // Already inside a static, act as if it was in a non static context
    }
  }
  
  FOREACH_LIST_INDEXED(InstanceNode*,subNode,inst->declaration->fixedDelayCircuit->allocated,index){
    FUInstance* subInst = subNode->inst;
    bool containsConfig = subInst->declaration->baseConfig.configs.size; // TODO: When  doing partition might need to put index here instead of 0
    
    InstanceConfigurationOffsets subOffsets = offsets;
    subOffsets.level += 1;
    subOffsets.topName = top.fullName;
    subOffsets.parent = topDecl;
    subOffsets.configOffset = subOffsetsConfig;
    
    // Bunch of static logic
    subOffsets.configOffset += inst->declaration->configInfo[part].configOffsets.offsets[index];
    subOffsets.stateOffset += inst->declaration->configInfo[part].stateOffsets.offsets[index];
    subOffsets.delayOffset += inst->declaration->configInfo[part].delayOffsets.offsets[index];
    subOffsets.belongs = inst->declaration->configInfo[part].unitBelongs[index];
    subOffsets.baseName = inst->declaration->configInfo[part].baseName[index];

    if(subInst->declaration->memoryMapBits.has_value()){
      res.memSize = AlignBitBoundary(res.memSize,subInst->declaration->memoryMapBits.value());
    }

    subOffsets.memOffset = offsets.memOffset + res.memSize;

    Partition zero = {}; // For now do not divide partition
    AcceleratorInfo info = TransformGraphIntoArrayRecurseNoDelay(subNode,inst->declaration,subOffsets,zero,temp,out);
    
    res.memSize += info.memSize;
    
    for(InstanceInfo& f : info.info){
      *PushListElement(instanceList) = f;
    }
  }

  res.info = PushArrayFromList(out,instanceList);
  res.memSize = top.memMappedSize.value_or(0);

  Assert(part <= inst->declaration->configInfo.size);
  res.name = inst->declaration->configInfo[part].name;
  
  return res;
}

// For now I'm going to make this function return everything

// TODO: This should call all the Calculate* functions and then put everything together.
//       We are complication this more than needed. We already have all the logic needed to do this.
//       The only thing that we actually need to do is to recurse, access data from configInfo (or calculate if at the top) and them put them all together.

// TODO: Offer a recursive option and a non recursive option. 

// NOTE: I could: Have a function that recurses and counts how many nodes there are.
//       Allocates the Array of instance info.
//       We can then fill everything by computing configInfo and stuff at the top and accessing fudeclaration configInfos as we go down.

TestResult CalculateOneInstanceNoDelay(Accelerator* accel,bool recursive,Array<Partition> partitions,Arena* temp,Arena* out){
  ArenaList<InstanceInfo>* infoList = PushArenaList<InstanceInfo>(temp);
  Hashmap<StaticId,int>* staticInfo = PushHashmap<StaticId,int>(temp,99);

  // TODO: This is being recalculated multiple times if we have various partitions. Move out when this function starts stabilizing.
  CalculatedOffsets configOffsets = CalculateConfigOffsetsIgnoringStatics(accel,out);
  CalculatedOffsets delayOffsets = CalculateConfigurationOffset(accel,MemType::DELAY,out);


  int partitionIndex = 0;
  int staticConfig = 0x40000000; // TODO: Make this more explicit
  int index = 0;
  InstanceConfigurationOffsets subOffsets = {};
  subOffsets.topName = {};
  subOffsets.staticConfig = &staticConfig;
  subOffsets.staticInfo = staticInfo;
  subOffsets.belongs = true;
  ArenaList<String>* names = PushArenaList<String>(temp);
  FOREACH_LIST_INDEXED(InstanceNode*,node,accel->allocated,index){
    FUInstance* subInst = node->inst;

    Partition part = {};
    if(subInst->declaration->configInfo.size > 1){
      part = partitions[partitionIndex];
    }
    
    subOffsets.configOffset = configOffsets.offsets[index];
    subOffsets.delayOffset = delayOffsets.offsets[index];
    
    if(recursive){
      AcceleratorInfo info = TransformGraphIntoArrayRecurseNoDelay(node,nullptr,subOffsets,part,temp,out);

      if(info.name.has_value()){
        *names->PushElem() = info.name.value();
      }
      
      subOffsets.memOffset += info.memSize;
      subOffsets.memOffset = AlignBitBoundary(subOffsets.memOffset,log2i(info.memSize));
      
      for(InstanceInfo& f : info.info){
        *PushListElement(infoList) = f;
      }
    } else {
      InstanceInfo top = GetInstanceInfoNoDelay(node,nullptr,subOffsets,out);

      subOffsets.memOffset += top.memMappedSize.value_or(0);
      subOffsets.memOffset = AlignBitBoundary(subOffsets.memOffset,log2i(top.memMappedSize.value_or(1)));
      
      *PushListElement(infoList) = top;
    }

    if(subInst->declaration->configInfo.size > 1){
      partitionIndex += 1;
    }
  }
  Array<InstanceInfo> res = PushArrayFromList<InstanceInfo>(out,infoList);
  TestResult result = {};
  result.info = res;
  result.subOffsets = subOffsets;

  Array<String> nameList = PushArrayFromList(temp,names);
  
  auto mark = StartString(out);
  bool first = true;
  for(String name : nameList){
    if(first){
      first = false;
    } else {
      PushString(out,"_");
    }
    PushString(out,name);
  }
  result.name = EndString(mark);
  
  return result;
}

// TODO: Move this function to a better place
// This function cannot return an array for merge units because we do not have the merged units info in this level.
// This function only works for modules and for recursing the merged info to upper modules.
// The info needed by merge must be stored by the merge function.

AccelInfo CalculateAcceleratorInfoNoDelay(Accelerator* accel,bool recursive,Arena* out,Arena* temp){
  // TODO: Have this function return everything that needs to be arraified  
  //       Printing a GraphArray struct should tell me everything I need to know about the accelerator
  AccelInfo result = {};

  // MARKED - Some bug occurs, probably because the function is a mess and we have no idea 
  //BLOCK_REGION(temp);
  
  DynamicArray<Partition> partitionsArr = StartArray<Partition>(temp);
  int mergedPossibility = 0;
  FOREACH_LIST(InstanceNode*,node,accel->allocated){
    FUInstance* subInst = node->inst;
    FUDeclaration* decl = subInst->declaration;

    if(subInst->declaration->configInfo.size > 1){
      mergedPossibility += log2i(decl->configInfo.size);
      *partitionsArr.PushElem() = (Partition){.value = 0,.max = decl->configInfo.size};
    }
  }
  Array<Partition> partitions = EndArray(partitionsArr);

  int totalMaxBitsize = 0;
  ArenaList<TestResult>* list = PushArenaList<TestResult>(temp);
  Array<InstanceInfo> res = {};
  InstanceConfigurationOffsets subOffsets = {};
  bool first = true;
  int index = 0;
  while(1){
    TestResult res2 = CalculateOneInstanceNoDelay(accel,recursive,partitions,temp,out);
    *PushListElement(list) = res2;

    if(first){
      res = res2.info;
      subOffsets = res2.subOffsets;
    }
  
    int maxMemBitsize = log2i(subOffsets.memOffset);
    if(maxMemBitsize > 0){
      for(InstanceInfo& info : res2.info){
        if(info.memMapped.has_value()){
          String& memMask = info.memMappedMask.value();

          memMask.data += 32 - maxMemBitsize;
          memMask.size = maxMemBitsize;
        }
      }
    }

    if(first){
      totalMaxBitsize = maxMemBitsize;
    }
    
    FUDeclaration* maxConfigDecl = nullptr;
    int maxConfig = 0;
    for(InstanceInfo& inst : res2.info){
      if(inst.configPos.has_value()){
        int val = inst.configPos.value();
        if(val < 0x40000000){
          if(val > maxConfig){
            maxConfig = val;
            maxConfigDecl = inst.decl;
          }
        }
      }
    }

    if(maxConfigDecl){
      int totalConfigSize = maxConfig + maxConfigDecl->baseConfig.configOffsets.max;
      for(InstanceInfo& inst : res2.info){
        if(inst.configPos.has_value()){
          int val = inst.configPos.value();
          if(val >= 0x40000000){
            inst.configPos = val - 0x40000000 + totalConfigSize;
          }
        }
      }
    }

    first = false;
    if(!Next(partitions)){
      break;
    }
  }
  Array<TestResult> final = PushArrayFromList(out,list);

  result.infos = Map(final,out,[](TestResult t){
    return t.info;
  });
  result.names = Map(final,out,[](TestResult t){
    return t.name;
  });

  // Merge everything back into a single unique view.
  Array<InstanceInfo> unique = result.infos[0];
  result.baseInfo = PushArray<InstanceInfo>(out,unique.size);
  for(int i = 0; i < result.baseInfo.size; i++){
    result.baseInfo[i] = unique[i]; // Copy default values

    result.baseInfo[i].belongs = true; // All units belong to baseInfo
    result.baseInfo[i].baseName = result.baseInfo[i].name;
    
    for(Array<InstanceInfo> infos : result.infos){
      if(infos[i].configPos.has_value()){
        result.baseInfo[i].configPos = infos[i].configPos.value();
        break;
      }
    }
    for(Array<InstanceInfo> infos : result.infos){
      if(infos[i].statePos.has_value()){
        result.baseInfo[i].statePos = infos[i].statePos.value();
        break;
      }
    }
    for(Array<InstanceInfo> infos : result.infos){
      if(infos[i].memMapped.has_value()){
        result.baseInfo[i].memMapped = infos[i].memMapped.value();
        break;
      }
    }
    for(Array<InstanceInfo> infos : result.infos){
      if(infos[i].memMappedSize.has_value()){
        result.baseInfo[i].memMappedSize = infos[i].memMappedSize.value();
        break;
      }
    }
    for(Array<InstanceInfo> infos : result.infos){
      if(infos[i].memMappedBitSize.has_value()){
        result.baseInfo[i].memMappedBitSize = infos[i].memMappedBitSize.value();
        break;
      }
    }
    for(Array<InstanceInfo> infos : result.infos){
      if(infos[i].memMappedMask.has_value()){
        result.baseInfo[i].memMappedMask = infos[i].memMappedMask.value();
        break;
      }
    }
    for(Array<InstanceInfo> infos : result.infos){
      if(infos[i].delayPos.has_value()){
        result.baseInfo[i].delayPos = infos[i].delayPos.value();
        break;
      }
    }
  }
  
  result.memMappedBitsize = totalMaxBitsize;

  std::vector<bool> seenShared;

  Hashmap<StaticId,int>* staticSeen = PushHashmap<StaticId,int>(temp,1000);

  int memoryMappedDWords = 0;

  // Handle non-static information
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    FUInstance* inst = ptr->inst;
    FUDeclaration* type = inst->declaration;

    // Check if shared
    if(inst->sharedEnable){
      if(inst->sharedIndex >= (int) seenShared.size()){
        seenShared.resize(inst->sharedIndex + 1);
      }

      if(!seenShared[inst->sharedIndex]){
        result.configs += type->baseConfig.configs.size;
        result.sharedUnits += 1;
      }

      seenShared[inst->sharedIndex] = true;
    } else if(!inst->isStatic){ // Shared cannot be static
         result.configs += type->baseConfig.configs.size;
    }

    if(type->memoryMapBits.has_value()){
      result.isMemoryMapped = true;

      memoryMappedDWords = AlignBitBoundary(memoryMappedDWords,type->memoryMapBits.value());
      memoryMappedDWords += (1 << type->memoryMapBits.value());
    }

    if(type == BasicDeclaration::input){
      result.inputs += 1;
    }

    result.states += type->baseConfig.states.size;
    result.delays += type->baseConfig.delayOffsets.max;
    result.ios += type->nIOs;

    if(type->externalMemory.size){
      result.externalMemoryInterfaces += type->externalMemory.size;
	  result.externalMemoryByteSize = ExternalMemoryByteSize(type->externalMemory);
    }

    result.signalLoop |= type->signalLoop;
  }

  result.memoryMappedBits = log2i(memoryMappedDWords);

  FUInstance* outputInstance = GetOutputInstance(accel->allocated);
  if(outputInstance){
    FOREACH_LIST(Edge*,edge,accel->edges){
      if(edge->units[0].inst == outputInstance){
        result.outputs = std::max(result.outputs - 1,edge->units[0].port) + 1;
      }
      if(edge->units[1].inst == outputInstance){
        result.outputs = std::max(result.outputs - 1,edge->units[1].port) + 1;
      }
    }
  }

  // Handle static information
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    FUInstance* inst = ptr->inst;
    if(inst->isStatic){
      StaticId id = {};

      id.name = inst->name;

      staticSeen->Insert(id,1);
      result.statics += inst->declaration->baseConfig.configs.size;
    }

    if(IsTypeHierarchical(inst->declaration)){
      for(Pair<StaticId,StaticData> pair : inst->declaration->staticUnits){
        int* possibleFind = staticSeen->Get(pair.first);

        if(!possibleFind){
          staticSeen->Insert(pair.first,1);
          result.statics += pair.second.configs.size;
        }
      }
    }
  }
  
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    FUInstance* inst = ptr->inst;
    result.numberConnections += Size(ptr->allOutputs);
  }
  
  return result;
}




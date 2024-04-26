#include "configurations.hpp"

#include "debug.hpp"
#include "memory.hpp"
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

    int numberConfigs = inst->declaration->configInfo.configs.size;
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
      array[index] = 0x40000000;
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
    size = decl->configInfo.configs.size;
  }break;
  case MemType::DELAY:{
    size = decl->configInfo.delayOffsets.max;
  }break;
  case MemType::EXTRA:{
    size = decl->configInfo.extraDataOffsets.max;
  }break;
  case MemType::STATE:{
    size = decl->configInfo.states.size;
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

        *wire = t.decl->configInfo.configs[i];
        wire->name = PushString(out,"%.*s_%.*s",UNPACK_SS(t.fullName),UNPACK_SS(t.decl->configInfo.configs[i].name));
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

          *wire = t.decl->configInfo.configs[i];
          wire->name = PushString(out,"%.*s_%.*s_%.*s",UNPACK_SS(parentName),UNPACK_SS(t.name),UNPACK_SS(t.decl->configInfo.configs[i].name));
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
      count += in.decl->configInfo.states.size;
    }
  }

  Array<String> res = PushArray<String>(out,count);
  int index = 0;
  for(InstanceInfo& in : info){
    if(!in.isComposite && in.statePos.has_value()){
      FUDeclaration* decl = in.decl;
      for(Wire& wire : decl->configInfo.states){
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
struct InstanceConfigurationOffsets{
  Hashmap<StaticId,int>* staticInfo; 
  FUDeclaration* parent;
  String topName;
  int configOffset;
  int stateOffset;
  int delayOffset;
  int delay; // Actual delay value, not the delay offset.
  int memOffset;
  int level;
  int* staticConfig; // This starts at 0x40000000 and at the end of the function we normalized it since we can only figure out the static position at the very end.
};

String NodeTypeToString(InstanceNode* node){
  switch(node->type){
  case InstanceNode::TAG_UNCONNECTED:{
    return STRING("UNCONNECTED");
  } break;
  case InstanceNode::TAG_COMPUTE:{
    return STRING("COMPUTE");
  } break;
  case InstanceNode::TAG_SOURCE:{
    return STRING("SOURCE");
  } break;
  case InstanceNode::TAG_SINK:{
    return STRING("SINK");
  } break;
  case InstanceNode::TAG_SOURCE_AND_SINK:{
    return STRING("SOURCE_AND_SINK");
  } break;
  default: NOT_IMPLEMENTED("Implemented as needed");
  }
  NOT_POSSIBLE("Implemented as needed");
  return {};
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
  info.fullName = topName;
  info.decl = topDecl;
  info.isStatic = inst->isStatic; 
  info.isShared = inst->sharedEnable;
  info.level = offsets.level;
  info.isComposite = IsTypeHierarchical(topDecl);
  info.parent = parentDeclaration;
  info.baseDelay = offsets.delay;
  info.type = NodeTypeToString(node);
  
  if(topDecl->memoryMapBits.has_value()){
    info.memMappedSize = (1 << topDecl->memoryMapBits.value());
    info.memMappedBitSize = topDecl->memoryMapBits.value();
    info.memMapped = AlignBitBoundary(offsets.memOffset,topDecl->memoryMapBits.value());
    info.memMappedMask = BinaryRepr(info.memMapped.value(),out);
  }

  info.configSize = topDecl->configInfo.configs.size;
  info.stateSize = topDecl->configInfo.states.size;
  info.delaySize = topDecl->configInfo.delayOffsets.max;
  
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
    
  info.isConfigStatic = configIsStatic;
  
  if(topDecl->configInfo.states.size){
    info.statePos = offsets.stateOffset;
  }
 
  int nDelays = topDecl->configInfo.delayOffsets.max;
  if(nDelays > 0 && !info.isComposite){
    info.delay = PushArray<int>(out,nDelays);
    Memset(info.delay,offsets.delay);
  }

  if(topDecl->configInfo.delayOffsets.max){
    info.delayPos = offsets.delayOffset;
  }
  
  return info;
}

AcceleratorInfo TransformGraphIntoArrayRecurse(InstanceNode* node,FUDeclaration* parentDecl,InstanceConfigurationOffsets offsets,Arena* out,Arena* temp){
  // This prevents us from fully using arenas because we are pushing more data than the actual structures that we keep track of. If this was hierarchical, we would't have this problem.
  AcceleratorInfo res = {};

  FUInstance* inst = node->inst;
  InstanceInfo top = GetInstanceInfo(node,parentDecl,offsets,out);
  
  FUDeclaration* topDecl = inst->declaration;
  if(!IsTypeHierarchical(topDecl)){
    Array<InstanceInfo> arr = PushArray<InstanceInfo>(out,1);
    arr[0] = top;
    res.info = arr;
    res.stateSize = top.stateSize;
    res.delaySize = top.delaySize;
    res.memSize = top.memMappedSize.value_or(0);
    res.delay = offsets.delay;
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
      *offsets.staticConfig += inst->declaration->configInfo.configOffsets.offsets[index];

      offsets.staticInfo->Insert(id,config);

      subOffsetsConfig = config;
    } else {
      changedConfigFromStatic = false; // Already inside a static, act as if it was in a non static context
    }
  }
  
  FOREACH_LIST_INDEXED(InstanceNode*,subNode,inst->declaration->fixedDelayCircuit->allocated,index){
    FUInstance* subInst = subNode->inst;
    bool containsConfig = subInst->declaration->configInfo.configs.size;
    
    InstanceConfigurationOffsets subOffsets = offsets;
    subOffsets.level += 1;
    subOffsets.topName = top.fullName;
    subOffsets.parent = topDecl;
    subOffsets.configOffset = subOffsetsConfig;
    
    // Bunch of static logic
    subOffsets.configOffset += inst->declaration->configInfo.configOffsets.offsets[index];
    subOffsets.stateOffset += inst->declaration->configInfo.stateOffsets.offsets[index];
    subOffsets.delayOffset += inst->declaration->configInfo.delayOffsets.offsets[index];
    if(subInst->declaration->configInfo.delayOffsets.max > 0){
      subOffsets.delay = offsets.delay + inst->declaration->calculatedDelays[delayIndex++];
    }

    if(subInst->declaration->memoryMapBits.has_value()){
      res.memSize = AlignBitBoundary(res.memSize,subInst->declaration->memoryMapBits.value());
    }

    subOffsets.memOffset = offsets.memOffset + res.memSize;

    AcceleratorInfo info = TransformGraphIntoArrayRecurse(subNode,inst->declaration,subOffsets,temp,out);
    
    res.memSize += info.memSize;
    
    for(InstanceInfo& f : info.info){
      *PushListElement(instanceList) = f;
    }
  }

  res.info = PushArrayFromList(out,instanceList);
  res.memSize = top.memMappedSize.value_or(0);
  
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

// TODO: Move this function to a better place
Array<InstanceInfo> CalculateAcceleratorInfo(Accelerator* accel,bool recursive,Arena* out,Arena* temp){
  // TODO: Have this function return everything that needs to be arraified  
  //       Printing a GraphArray struct should tell me everything I need to know about the accelerator

  ArenaList<InstanceInfo>* infoList = PushArenaList<InstanceInfo>(temp);
  Hashmap<StaticId,int>* staticInfo = PushHashmap<StaticId,int>(temp,99);

  CalculatedOffsets configOffsets = CalculateConfigOffsetsIgnoringStatics(accel,out);
  CalculatedOffsets delayOffsets = CalculateConfigurationOffset(accel,MemType::DELAY,out);
  CalculateDelayResult calculatedDelay = CalculateDelay(accel->versat,accel,temp); // TODO: Would be better if calculate delay did not need to receive versat.
  
  int staticConfig = 0x40000000; // TODO: Make this more explicit
  int index = 0;
  InstanceConfigurationOffsets subOffsets = {};
  subOffsets.topName = {};
  subOffsets.staticConfig = &staticConfig;
  subOffsets.staticInfo = staticInfo;
  FOREACH_LIST_INDEXED(InstanceNode*,node,accel->allocated,index){
    FUInstance* subInst = node->inst;

    subOffsets.configOffset = configOffsets.offsets[index];
    subOffsets.delayOffset = delayOffsets.offsets[index];
    subOffsets.delay = calculatedDelay.nodeDelay->GetOrFail(node);
    
    if(recursive){
      AcceleratorInfo info = TransformGraphIntoArrayRecurse(node,nullptr,subOffsets,temp,out);

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
  }
  
  Array<InstanceInfo> res = PushArrayFromList<InstanceInfo>(out,infoList);

  FUDeclaration* maxConfigDecl = nullptr;
  int maxConfig = 0;
  for(InstanceInfo& inst : res){
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
    int totalConfigSize = maxConfig + maxConfigDecl->configInfo.configOffsets.max;
    for(InstanceInfo& inst : res){
      if(inst.configPos.has_value()){
        int val = inst.configPos.value();
        if(val >= 0x40000000){
          inst.configPos = val - 0x40000000 + totalConfigSize;
        }
      }
    }
  }
  
  return res;
}

Array<InstanceInfo> ExtractFromInstanceInfoSameLevel(Array<InstanceInfo> instanceInfo,int level,Arena* out){
  Byte* mark = MarkArena(out);
  for(InstanceInfo& info : instanceInfo){
    if(info.level == level){
      *PushStruct<InstanceInfo>(out) = info;
    }
  }

  return PointArray<InstanceInfo>(out,mark);
}

Array<InstanceInfo> ExtractFromInstanceInfo(Array<InstanceInfo> instanceInfo,Arena* out){
  Byte* mark = MarkArena(out);

  for(InstanceInfo& info : instanceInfo){
    if(info.configSize > 0 || info.stateSize > 0 || info.memMappedSize.value_or(0) > 0 || info.delaySize){
      *PushStruct<InstanceInfo>(out) = info;
    }
  }

  return PointArray<InstanceInfo>(out,mark);
}

void CheckSanity(Array<InstanceInfo> instanceInfo,Arena* temp){
  // TODO: Add more conditions here as bugs appear that break this

  BLOCK_REGION(temp);
  for(int i = 0; ; i++){
    Array<InstanceInfo> sameLevel = ExtractFromInstanceInfoSameLevel(instanceInfo,i,temp);

    if(sameLevel.size == 0){
      break;
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
  ConfigurationInfo& info = type->configInfo;

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

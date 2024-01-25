#include "configurations.hpp"

#include "debug.hpp"
#include "debugGUI.hpp"
#include "memory.hpp"
#include "utils.hpp"
#include "utilsCore.hpp"
#include "versat.hpp"
#include "acceleratorStats.hpp"
#include "textualRepresentation.hpp"
#include <strings.h>

// Top level
void FUInstanceInterfaces::Init(Accelerator* accel){
  VersatComputedValues val = ComputeVersatValues(accel->versat,accel,false);

  config.Init(accel->configAlloc.ptr,val.nConfigs);
  delay.Init(accel->configAlloc.ptr + accel->startOfDelay,val.nDelays);
  statics.Init(accel->configAlloc.ptr + accel->startOfStatic,val.nStatics);

  state.Init(accel->stateAlloc);
  mem.Init(nullptr,val.memoryMappedBytes);
  extraData.Init(accel->extraDataAlloc);
  externalMemory.Init(accel->externalMemoryAlloc);
}

// Sub instance
void FUInstanceInterfaces::Init(Accelerator* topLevel,FUInstance* inst){
  FUDeclaration* decl = inst->declaration;

  VersatComputedValues val = ComputeVersatValues(topLevel->versat,topLevel,false);
  statics.Init(topLevel->configAlloc.ptr + topLevel->startOfStatic,val.nStatics);

  config.Init(inst->config,decl->configInfo.configOffsets.max);
  state.Init(inst->state,decl->configInfo.stateOffsets.max);
  delay.Init(inst->delay,decl->configInfo.delayOffsets.max);
  mem.Init((Byte*) inst->memMapped,1 << decl->memoryMapBits.value_or(0));
  extraData.Init(inst->extraData,decl->configInfo.extraDataOffsets.max);
  if(decl->externalMemory.size){
    externalMemory.Init(inst->externalMemory,ExternalMemoryByteSize(decl->externalMemory));
  }
}

void FUInstanceInterfaces::AssertEmpty(){
#if 0
  Assert(config.Empty());
  Assert(state.Empty());
  Assert(delay.Empty());
  Assert(outputs.Empty());
  Assert(storedOutputs.Empty());
  Assert(extraData.Empty());
#endif
}

void AddOffset(CalculatedOffsets* offsets,int amount){
  for(int i = 0; i < offsets->offsets.size; i++){
    offsets->offsets[i] += amount;
  }
  offsets->max += amount;
}

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
  default: NOT_IMPLEMENTED;
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

// MARKED
CalculatedOffsets ExtractConfig(Accelerator* accel,Arena* out){
  int count = 0;
  int maxConfig = 0;
  AcceleratorIterator iter = {};
  region(out){
    for(InstanceNode* node = iter.Start(accel,out,true); node; node = iter.Next()){
      FUInstance* inst = node->inst;
      if(inst->declaration->configInfo.configs.size){
        int config = inst->config - accel->configAlloc.ptr;
        maxConfig = std::max(config,maxConfig);
      }

      count += 1;
    }
  }

  Array<int> offsets = PushArray<int>(out,count);

  int index = 0;
  region(out){
    for(InstanceNode* node = iter.Start(accel,out,true); node; node = iter.Next(),index += 1){
      FUInstance* inst = node->inst;
      int config = 0;
      if(inst->config == nullptr){
        offsets[index] = -1;
        continue;
      }

      offsets[index] = inst->config - accel->configAlloc.ptr;
    }
  }

  CalculatedOffsets res = {};
  res.offsets = offsets;
  res.max = maxConfig;
  return res;
}

// MARKED
CalculatedOffsets ExtractState(Accelerator* accel,Arena* out){
  int count = NumberUnits(accel);
  Array<int> offsets = PushArray<int>(out,count);

  AcceleratorIterator iter = {};
  int index = 0;
  int max = 0;
  for(InstanceNode* node = iter.Start(accel,out,true); node; node = iter.Next(),index += 1){
    FUInstance* inst = node->inst;

    if(inst->state == nullptr){
      offsets[index] = -1;
      continue;
    }

    int state = inst->state - accel->stateAlloc.ptr;
    Assert(state < accel->stateAlloc.size);

    max = std::max(state,max);
    offsets[index] = state;
  }

  CalculatedOffsets res = {};
  res.offsets = offsets;
  res.max = max;
  return res;
}

// MARKED
CalculatedOffsets ExtractDelay(Accelerator* accel,Arena* out){
  int count = NumberUnits(accel);
  Array<int> offsets = PushArray<int>(out,count);

  AcceleratorIterator iter = {};
  int index = 0;
  int max = 0;
  for(InstanceNode* node = iter.Start(accel,out,true); node; node = iter.Next(),index += 1){
    FUInstance* inst = node->inst;

    if(inst->delay == nullptr){
      offsets[index] = -1;
      continue;
    }

    int delay = inst->delay - accel->configAlloc.ptr;
    max = std::max(delay,max);
    offsets[index] = delay;
  }

  CalculatedOffsets res = {};
  res.offsets = offsets;
  res.max = max;
  return res;
}

// MARKED
CalculatedOffsets ExtractMem(Accelerator* accel,Arena* out){
  int count = NumberUnits(accel);
  Array<int> offsets = PushArray<int>(out,count);

  AcceleratorIterator iter = {};
  int index = 0;
  int max = 0;
  for(InstanceNode* node = iter.Start(accel,out,true); node; node = iter.Next(),index += 1){
    FUInstance* inst = node->inst;

    if(inst->externalMemory == nullptr){
      offsets[index] = -1;
      continue;
    }

    int mem = inst->externalMemory - accel->externalMemoryAlloc.ptr;
    max = std::max(mem,max);
    offsets[index] = mem;
  }

  CalculatedOffsets res = {};
  res.offsets = offsets;
  res.max = max;
  return res;
}

// MARKED
CalculatedOffsets ExtractExtraData(Accelerator* accel,Arena* out){
  int count = NumberUnits(accel);
  Array<int> offsets = PushArray<int>(out,count);

  AcceleratorIterator iter = {};
  int index = 0;
  int max = 0;
  for(InstanceNode* node = iter.Start(accel,out,true); node; node = iter.Next(),index += 1){
    FUInstance* inst = node->inst;

    if(inst->extraData == nullptr){
      offsets[index] = -1;
      continue;
    }

    int extraData = inst->extraData - accel->extraDataAlloc.ptr;
    max = std::max(extraData,max);
    offsets[index] = extraData;
  }

  CalculatedOffsets res = {};
  res.offsets = offsets;
  res.max = max;
  return res;
}

bool CheckCorrectConfiguration(Accelerator* topLevel,FUInstance* inst){
  Assert(inst->config == nullptr || Inside(&topLevel->configAlloc,inst->config));
  Assert(inst->state == nullptr || Inside(&topLevel->stateAlloc,inst->state));
  Assert(inst->extraData == nullptr || Inside(&topLevel->extraDataAlloc,inst->extraData));
  //Assert(inst->outputs == nullptr || Inside(&topLevel->outputAlloc,inst->outputs));
  //Assert(inst->storedOutputs == nullptr || Inside(&topLevel->storedOutputAlloc,inst->storedOutputs));

  return true;
}

void CheckCorrectConfiguration(Accelerator* topLevel,FUInstanceInterfaces& inter,FUInstance* inst){
  if(IsConfigStatic(topLevel,inst)){
    Assert(inst->config == nullptr || Inside(&inter.statics,inst->config));
  } else {
    Assert(inst->config == nullptr || Inside(&inter.config,inst->config));
  }

  Assert(inst->state == nullptr || Inside(&inter.state,inst->state));
  Assert(inst->delay == nullptr || Inside(&inter.delay,inst->delay));
  Assert(inst->extraData == nullptr || Inside(&inter.extraData,inst->extraData));
}

// Populates sub accelerator
void PopulateAccelerator(Accelerator* topLevel,Accelerator* accel,FUDeclaration* topDeclaration,FUInstanceInterfaces& inter,std::unordered_map<StaticId,StaticData>* staticMap){
  int index = 0;
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    FUInstance* inst = ptr->inst;
    FUDeclaration* decl = inst->declaration;

    inst->config = nullptr;
    inst->state = nullptr;
    inst->delay = nullptr;
    inst->memMapped = nullptr;
    inst->extraData = nullptr;

#if 0
    int numberUnits = 0;
    if(IsTypeHierarchical(decl)){
      numberUnits = CalculateNumberOfUnits(decl->fixedDelayCircuit->allocated);
    }
    inst->debugData = inter.debugData.Push(numberUnits + 1);
#endif

    if(inst->isStatic && decl->configInfo.configs.size){
      StaticId id = {};
      id.parent = topDeclaration;
      id.name = inst->name;

      auto iter = staticMap->find(id);

      Assert(iter != staticMap->end());
      inst->config = &inter.statics.ptr[iter->second.offset];
    } else if(decl->configInfo.configs.size){
      inst->config = inter.config.Set(topDeclaration->configInfo.configOffsets.offsets[index],decl->configInfo.configs.size);
    }
    if(decl->configInfo.states.size){
      inst->state = inter.state.Set(topDeclaration->configInfo.stateOffsets.offsets[index],decl->configInfo.states.size);
    }
    if(decl->memoryMapBits.has_value()){
      inter.mem.timesPushed = AlignBitBoundary(inter.mem.timesPushed,decl->memoryMapBits.value());
      inst->memMapped = (int*) inter.mem.Push(1 << decl->memoryMapBits.value());
    }
    if(decl->configInfo.delayOffsets.max){
      inst->delay = inter.delay.Set(topDeclaration->configInfo.delayOffsets.offsets[index],decl->configInfo.delayOffsets.max);
    }
    if(decl->externalMemory.size){
	  inst->externalMemory = inter.externalMemory.Push(ExternalMemoryByteSize(decl->externalMemory));
    }
    if(decl->configInfo.extraDataOffsets.max){
      inst->extraData = inter.extraData.Set(topDeclaration->configInfo.extraDataOffsets.offsets[index],decl->configInfo.extraDataOffsets.max);
    }

#if 0 // Should be a debug flag
CheckCorrectConfiguration(topLevel,inter,inst);
CheckCorrectConfiguration(topLevel,inst);
#endif

index += 1;
  }
}

bool IsConfigStatic(Accelerator* topLevel,FUInstance* inst){
  int offset = inst->config - topLevel->configAlloc.ptr;
  if(offset >= topLevel->startOfStatic){
    return true;
  }
  return false;
}

// The true "Accelerator" populator
void PopulateTopLevelAccelerator(Accelerator* accel){
  STACK_ARENA(tempInst,Kilobyte(64));
  Arena* temp = &tempInst;

  int sharedUnits = 0;
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    if(ptr->inst->sharedEnable){
      sharedUnits += 1;
    }
  }

  FUInstanceInterfaces inter = {};
  inter.Init(accel);

  Hashmap<int,iptr*>* sharedToConfigPtr = PushHashmap<int,iptr*>(temp,sharedUnits);

  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    FUInstance* inst = ptr->inst;
    FUDeclaration* decl = inst->declaration;

    inst->config = nullptr;
    inst->state = nullptr;
    inst->delay = nullptr;
    inst->memMapped = nullptr;
    inst->extraData = nullptr;

#if 0
    int numberUnits = 0;
    if(IsTypeHierarchical(decl)){
      numberUnits = CalculateNumberOfUnits(decl->fixedDelayCircuit->allocated);
    }
    inst->debugData = inter.debugData.Push(numberUnits + 1);
#endif

    if(decl->configInfo.configs.size){
      if(inst->isStatic){
        StaticId id = {};
        id.name = inst->name;

        auto iter = accel->staticUnits.find(id);

        Assert(iter != accel->staticUnits.end());
        inst->config = &inter.statics.ptr[iter->second.offset];
      } else if(inst->sharedEnable){
        iptr** ptr = sharedToConfigPtr->Get(inst->sharedIndex);

        if(ptr){
          inst->config = *ptr;
        } else {
          inst->config = inter.config.Push(decl->configInfo.configs.size);
          sharedToConfigPtr->Insert(inst->sharedIndex,inst->config);
        }
      } else {
        inst->config = inter.config.Push(decl->configInfo.configs.size);
      }
    }

    if(decl->configInfo.states.size){
      inst->state = inter.state.Push(decl->configInfo.states.size);
    }

    if(decl->memoryMapBits.has_value()){
      inter.mem.timesPushed = AlignBitBoundary(inter.mem.timesPushed,decl->memoryMapBits.value());
      inst->memMapped = (int*) inter.mem.Push(1 << decl->memoryMapBits.value());
    }
    if(decl->configInfo.delayOffsets.max){
      inst->delay = inter.delay.Push(decl->configInfo.delayOffsets.max);
    }
    if(decl->externalMemory.size){
	  inst->externalMemory = inter.externalMemory.Push(ExternalMemoryByteSize(decl->externalMemory));
    }
#if 0
    if(decl->configInfo.outputOffsets.max){
      inst->outputs = inter.outputs.Push(decl->configInfo.outputOffsets.max);
      inst->storedOutputs = inter.storedOutputs.Push(decl->configInfo.outputOffsets.max);
    }
#endif
    if(decl->configInfo.extraDataOffsets.max){
      inst->extraData = inter.extraData.Push(decl->configInfo.extraDataOffsets.max);
    }

    CheckCorrectConfiguration(accel,inst);
  }
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

Array<Pair<String,iptr>> ExtractMem(Array<InstanceInfo> info,Arena* out){
  int count = 0;
  for(InstanceInfo& in : info){
    if(!in.isComposite && in.memMapped.has_value()){
      count += 1;
    }
  }

  Array<Pair<String,iptr>> res = PushArray<Pair<String,iptr>>(out,count);
  int index = 0;
  for(InstanceInfo& in : info){
    if(!in.isComposite && in.memMapped.has_value()){
      FUDeclaration* decl = in.decl;
      String name = PushString(out,"%.*s_addr",UNPACK_SS(in.fullName)); 
      res[index++] = {name,in.memMapped.value()};
    }
  }

  return res;
}

// TODO: I do not think that I actually use the size in the SizedConfig structs.
//       Remove / Rework these functions, I think they are recalculating data that we already have a better way of getting it.
// MARKED
Hashmap<String,SizedConfig>* ExtractNamedSingleConfigs(Accelerator* accel,Arena* out){
  STACK_ARENA(temp,Kilobyte(1));

  int count = 0;
  AcceleratorIterator iter = {};
  region(out){
    for(InstanceNode* node = iter.Start(accel,out,false); node; node = iter.Next()){
      FUInstance* inst = node->inst;
      FUDeclaration* decl = inst->declaration;
      if(decl->type == FUDeclaration::SINGLE){
        count += decl->configInfo.configs.size;
      }
    }
  }

  Hashmap<String,SizedConfig>* res = PushHashmap<String,SizedConfig>(out,count);

  for(InstanceNode* node = iter.Start(accel,out,false); node; node = iter.Next()){
    FUInstance* inst = node->inst;
    FUDeclaration* decl = inst->declaration;
    if(decl->type == FUDeclaration::SINGLE && decl->configInfo.configs.size){
      BLOCK_REGION(&temp);
      String name = iter.GetFullName(&temp,"_");

      for(int i = 0; i < decl->configInfo.configs.size; i++){
        String fullName = PushString(out,"%.*s_%.*s",UNPACK_SS(name),UNPACK_SS(decl->configInfo.configs[i].name));
        res->Insert(fullName,(SizedConfig){(iptr*)inst->config});
      }
    }
  }

  return res;
}

// MARKED
Hashmap<String,SizedConfig>* ExtractNamedSingleStates(Accelerator* accel,Arena* out){
  STACK_ARENA(temp,Kilobyte(1));

  int count = 0;
  AcceleratorIterator iter = {};
  region(out){
    for(InstanceNode* node = iter.Start(accel,out,false); node; node = iter.Next()){
      FUInstance* inst = node->inst;
      FUDeclaration* decl = inst->declaration;
      if(decl->type == FUDeclaration::SINGLE){
        count += decl->configInfo.states.size;
      }
    }
  }

  Hashmap<String,SizedConfig>* res = PushHashmap<String,SizedConfig>(out,count);

  for(InstanceNode* node = iter.Start(accel,out,false); node; node = iter.Next()){
    FUInstance* inst = node->inst;
    FUDeclaration* decl = inst->declaration;
    if(decl->type == FUDeclaration::SINGLE && decl->configInfo.states.size){
      BLOCK_REGION(&temp);
      String name = iter.GetFullName(&temp,"_");

      for(int i = 0; i < decl->configInfo.states.size; i++){
        String fullName = PushString(out,"%.*s_%.*s",UNPACK_SS(name),UNPACK_SS(decl->configInfo.states[i].name));
        res->Insert(fullName,(SizedConfig){(iptr*)inst->config});
      }
    }
  }

  return res;
}

// MARKED
Hashmap<String,SizedConfig>* ExtractNamedSingleMem(Accelerator* accel,Arena* out){
  int count = 0;
  AcceleratorIterator iter = {};
  region(out){
    for(InstanceNode* node = iter.Start(accel,out,false); node; node = iter.Next()){
      FUInstance* inst = node->inst;
      FUDeclaration* decl = inst->declaration;
      if(decl->type == FUDeclaration::SINGLE){
        count += 1;
      }
    }
  }

  Hashmap<String,SizedConfig>* res = PushHashmap<String,SizedConfig>(out,count);

  for(InstanceNode* node = iter.Start(accel,out,true); node; node = iter.Next()){
    FUInstance* inst = node->inst;
    FUDeclaration* decl = inst->declaration;
    if(decl->type == FUDeclaration::SINGLE && decl->memoryMapBits.has_value()){
      String name = iter.GetFullName(out,"_");
      String fullName = PushString(out,"%.*s_addr",UNPACK_SS(name)); // TODO: We could just extend the previous string

      res->Insert(fullName,(SizedConfig){(iptr*) inst->memMapped});
    }
  }

  return res;
}

// MARKED
void CalculateStaticConfigurationPositions(Versat* versat,Accelerator* accel,Arena* temp){
  BLOCK_REGION(temp);

  VersatComputedValues val = ComputeVersatValues(versat,accel,false);
  int staticStart = val.nConfigs + val.nDelays;

  accel->calculatedStaticPos = PushHashmap<StaticId,StaticData>(accel->accelMemory,val.nStatics);

  int staticIndex = 0; // staticStart; TODO: For now, test with static info beginning at zero
  AcceleratorIterator iter = {};
  for(InstanceNode* node = iter.Start(accel,temp,false); node; node = iter.Next()){
    FUInstance* inst = node->inst;
    FUDeclaration* decl = inst->declaration;
    if(inst->isStatic){
      FUDeclaration* parentDecl = iter.ParentInstance()->inst->declaration;

      StaticId id = {};
      id.name = inst->name;
      id.parent = parentDecl;

      GetOrAllocateResult res = accel->calculatedStaticPos->GetOrAllocate(id);

      if(res.alreadyExisted){
        //Assert(data == *res.data);
      } else {
        StaticData data = {};
        data.offset = staticIndex;
        data.configs = decl->configInfo.configs;

        *res.data = data;
        staticIndex += decl->configInfo.configs.size;
      }
    }
  }
  accel->staticSize = staticIndex;
}

void SetDefaultConfiguration(FUInstance* instance){
  // TODO; is this function even being used ?
  // I think whether to save defaults should be at the accelerator level
  FUInstance* inst = (FUInstance*) instance;

  inst->savedConfiguration = true;
}

void ShareInstanceConfig(FUInstance* inst, int shareBlockIndex){
  inst->sharedIndex = shareBlockIndex;
  inst->sharedEnable = true;
}

void SetStatic(Accelerator* accel,FUInstance* inst){
  // After two phase compilation change, we only need to store the change in the instance.
#if 0
  int size = inst->declaration->configs.size;
  iptr* oldConfig = inst->config;

  StaticId id = {};
  id.name = inst->name;

  auto iter = accel->staticUnits.find(id);
  if(iter == accel->staticUnits.end()){
    bool repopulate = false;
    if(accel->staticAlloc.size + size > accel->staticAlloc.reserved){
      int oldSize = accel->staticAlloc.size;
      ZeroOutRealloc(&accel->staticAlloc,accel->staticAlloc.size + size);
      accel->staticAlloc.size = oldSize;

      repopulate = true;
    }

    iptr* ptr = Push(&accel->staticAlloc,size);

    StaticData data = {};
    data.offset = ptr - accel->staticAlloc.ptr;

    accel->staticUnits.insert({id,data});

    // Only copy data if new static. No reason, do not know if always copying it would be better or not
    Memcpy(ptr,inst->config,size);
    inst->config = ptr;

    if(repopulate){
      Arena* arena = &accel->versat->temp;
      BLOCK_REGION(arena);
      AcceleratorIterator iter = {};
      iter.Start(accel,arena,true);
    }
  }

  RemoveChunkAndCompress(&accel->configAlloc,oldConfig,size);
#endif

  inst->isStatic = true;
}

#if 1
void InitializeAccelerator(Versat* versat,Accelerator* accel,Arena* temp){
  UnitValues val = CalculateAcceleratorValues(versat,accel);
  int numberUnits = CalculateNumberOfUnits(accel->allocated);

  ZeroOutAlloc(&accel->configAlloc,val.configs + val.delays + val.statics);
  ZeroOutAlloc(&accel->stateAlloc,val.states);
  //ZeroOutAlloc(&accel->outputAlloc,val.totalOutputs);
  //ZeroOutAlloc(&accel->storedOutputAlloc,val.totalOutputs);
  ZeroOutAlloc(&accel->extraDataAlloc,val.extraData);
  ZeroOutAlloc(&accel->externalMemoryAlloc,val.externalMemoryByteSize);

  CalculateStaticConfigurationPositions(versat,accel,temp);
  accel->startOfStatic = val.configs;
  accel->startOfDelay = val.configs + val.statics;

  AcceleratorIterator iter = {};
  InstanceNode* node = iter.Start(accel,temp,true);
}
#endif

String ReprStaticConfig(StaticId id,Wire* wire,Arena* out){
  String identifier = PushString(out,"%.*s_%.*s_%.*s",UNPACK_SS(id.parent->name),UNPACK_SS(id.name),UNPACK_SS(wire->name));

  return identifier;
}

// NOTE: We should access the declarations information instead of recursing into the sub accelerators.
//       At least for now. 
#if 0
void TransformGraphIntoArrayRecursive(Accelerator* accel,String currentHierarchicalName,Arena* out,ArenaList<InstanceInfo>* infoList){

  int index = 0;
  FOREACH_LIST_INDEXED(InstanceNode*,ptr,accel->allocated,index){
    
  }

  FUInstance* inst = node->inst;
  FUDeclaration* decl = inst->declaration;

  if(!IsTypeHierarchical(decl)){
    InstanceInfo* info = PushListElement(infoList);
    *info = {};

    // TODO: We could also just add hierarchical names back instead of allocating the full name everytime 
    info->decl = decl;
    info->fullName = PushString(out,"%.*s_%.*s",UNPACK_SS(currentHierarchicalName),UNPACK_SS(inst->name));

#if 0
    if(decl->configInfo.configs.size && inst->config && !IsConfigStatic(accel,inst)){
      info->configPos = inst->config - accel->configAlloc.ptr;
    }
    if(decl->configInfo.states.size && inst->state){
      info->statePos = inst->state - accel->stateAlloc.ptr;
    }
    if(decl->isMemoryMapped){
      info->memoryMapped = (iptr) inst->memMapped;
    }
#endif
    
    info->isComposite = decl->type == FUDeclaration::COMPOSITE;
  }
  
}
#endif

// TODO: Maybe it would be easier if we extracted the configs as if it was a "Command" and we accumulated an entire array of these commands which in the end we would join together to make the final configuration array.
//       It would change the code to make it such that each unit "makes an request to allocate space" in the configuration array and then we could create a function that would process these requests.
//       Static could be handled by simply sorting the requests so that non static appear first and are processed first, and when we reach static we knew immediatly the start of the configuration address. (Or something like that, I'm still not sure what's the best way of handling statics and so on.

// NOTE: The logic for static and shared configs is hard to decouple and simplify. I found no other way of doing this, other than printing the result for a known set of circuits and make small changes at a time.
struct InstanceConfigurationOffsets{
  Hashmap<StaticId,int>* staticInfo; 
  FUDeclaration* parent;
  String topName;
  int configOffset;
  int stateOffset;
  int delayOffset;
  int memOffset;
  int level;
  int* staticConfig; // This starts at 0x40000000 and at the end of the function we normalized it since we can only figure out the static position at the very end.
};

static InstanceInfo GetInstanceInfo(FUInstance* inst,InstanceConfigurationOffsets offsets,Arena* out){
  String topName = inst->name;
  if(offsets.topName.size != 0){
    topName = PushString(out,"%.*s_%.*s",UNPACK_SS(offsets.topName),UNPACK_SS(inst->name));
  }
    
  FUDeclaration* topDecl = inst->declaration;
  InstanceInfo info = {};
  info = {};

  info.fullName = topName;
  info.decl = topDecl;
  info.isStatic = inst->isStatic;
  info.isShared = inst->sharedEnable; // TODO: Is shared is not working currently, for now
  info.level = offsets.level;
  info.isComposite = IsTypeHierarchical(topDecl);

  // TODO: We are still looking at config info.
  info.configSize = topDecl->configInfo.configs.size;
  info.stateSize = topDecl->configInfo.states.size;
  info.delaySize = topDecl->configInfo.delayOffsets.max;

  if(topDecl->memoryMapBits.has_value()){
    info.memMappedSize = (1 << topDecl->memoryMapBits.value());
    info.memMapped = AlignBitBoundary(offsets.memOffset,topDecl->memoryMapBits.value());
  }
  
  bool containsConfigs = info.configSize;
  if(containsConfigs){
    if(inst->isStatic){
      StaticId id = {offsets.parent,inst->name};
      if(offsets.staticInfo->Exists(id)){
        info.configPos = offsets.staticInfo->GetOrFail(id);
      } else {
        int config = *offsets.staticConfig;

        offsets.staticInfo->Insert(id,config);
        info.configPos = config;

        *offsets.staticConfig += info.configSize;
      }
    } else {
      info.configPos = offsets.configOffset;
    }
  }
    
  if(topDecl->configInfo.states.size){
    info.statePos = offsets.stateOffset;
  }

  if(topDecl->configInfo.delayOffsets.max){
    info.delayPos = offsets.delayOffset;
  }
  
  return info;
}

AcceleratorInfo TransformGraphIntoArrayRecurse(FUInstance* inst,InstanceConfigurationOffsets offsets,Arena* out,Arena* temp){
  // This prevents us from fully using arenas because we are pushing more data than the actual structures that we keep track of. If this was hierarchical, we would't have this problem.
  AcceleratorInfo res = {};
  InstanceInfo top = GetInstanceInfo(inst,offsets,out);

  FUDeclaration* topDecl = inst->declaration;
  if(!IsTypeHierarchical(topDecl)){
    Array<InstanceInfo> arr = PushArray<InstanceInfo>(out,1);
    arr[0] = top;
    res.info = arr;
    res.stateSize = top.stateSize;
    res.delaySize = top.delaySize;
    res.memSize = top.memMappedSize.value_or(0);
    return res;
  }

  ArenaList<InstanceInfo>* instanceList = PushArenaList<InstanceInfo>(out); 
  *PushListElement(instanceList) = top;

  // Composite instance
  int index = 0;
  FOREACH_LIST_INDEXED(InstanceNode*,node,inst->declaration->fixedDelayCircuit->allocated,index){
    FUInstance* subInst = node->inst;
    bool containsConfig = subInst->declaration->configInfo.configs.size;
    
    InstanceConfigurationOffsets subOffsets = offsets;
    subOffsets.level += 1;
    subOffsets.topName = top.fullName;
    subOffsets.parent = topDecl;

    bool changedConfigFromStatic = false;
    int savedConfig = subOffsets.configOffset;
    // Bunch of static logic
    if(containsConfig){
      if(inst->isStatic){
        changedConfigFromStatic = true;
        StaticId id = {offsets.parent,inst->name};
        if(offsets.staticInfo->Exists(id)){
          subOffsets.configOffset = offsets.staticInfo->GetOrFail(id);
        } else if(offsets.configOffset <= 0x40000000){
          int config = *offsets.staticConfig;

          offsets.staticInfo->Insert(id,config);

          subOffsets.configOffset = config;
          *offsets.staticConfig += inst->declaration->configInfo.configOffsets.offsets[index];
        } else {
          changedConfigFromStatic = false; // Already inside a static, act as if it was in a non static context
          subOffsets.configOffset += inst->declaration->configInfo.configOffsets.offsets[index];
        }
      } else {
        subOffsets.configOffset += inst->declaration->configInfo.configOffsets.offsets[index];
      }
    }
    subOffsets.stateOffset += inst->declaration->configInfo.stateOffsets.offsets[index];
    subOffsets.delayOffset += inst->declaration->configInfo.delayOffsets.offsets[index];
    
    AcceleratorInfo info = TransformGraphIntoArrayRecurse(subInst,subOffsets,temp,out);
    
    for(InstanceInfo& f : info.info){
      *PushListElement(instanceList) = f;
    }
  }

  res.info = PushArrayFromList(out,instanceList);
  
  return res;
}

// For now I'm going to make this function return everything

// TODO: This should call all the Calculate* functions and then put everything together.
//       We are complication this more than needed. We already have all the logic needed to do this.
//       The only thing that we actually need to do is to recurse, access data from configInfo (or calculate if at the top) and them put them all together.

// NOTE: I could: Have a function that recurses and counts how many nodes there are.
//       Allocates the Array of instance info.
//       We can then fill everything by computing configInfo and stuff at the top and accessing fudeclaration configInfos as we go down.

// TODO: Move this function to a better place
Array<InstanceInfo> TransformGraphIntoArray(Accelerator* accel,Arena* out,Arena* temp){
  // TODO: Have this function return everything that needs to be arraified  
  //       Printing a GraphArray struct should tell me everything I need to know about the accelerator

  ArenaList<InstanceInfo>* infoList = PushArenaList<InstanceInfo>(temp);

#if 0

  printf("TEST\n");
  PrintAll(offsets.offsets,temp);
  NEWLINE();
  printf("%d\n",offsets.max);
  NEWLINE();
#endif

#if 1
  Hashmap<StaticId,int>* staticInfo = PushHashmap<StaticId,int>(temp,99);

  CalculatedOffsets configOffsets = CalculateConfigOffsetsIgnoringStatics(accel,out);
  CalculatedOffsets delayOffsets = CalculateConfigurationOffset(accel,MemType::DELAY,out);

  int staticConfig = 0x40000000; // TODO: Make this more explicit
  int index = 0;
  InstanceConfigurationOffsets subOffsets = {};
  subOffsets.topName = accel->name;
  subOffsets.staticConfig = &staticConfig;
  subOffsets.staticInfo = staticInfo;
  FOREACH_LIST_INDEXED(InstanceNode*,node,accel->allocated,index){
    printf("%d\n",index);
    FUInstance* subInst = node->inst;

    subOffsets.configOffset = configOffsets.offsets[index];
    subOffsets.delayOffset = delayOffsets.offsets[index];
    
    AcceleratorInfo info = TransformGraphIntoArrayRecurse(subInst,subOffsets,temp,out);
    subOffsets.memOffset += info.memSize;
    
    for(InstanceInfo& f : info.info){
      *PushListElement(infoList) = f;
    }
  }
#endif
  
#if 0
  AcceleratorIterator iter = {};
  
  for(InstanceNode* node = iter.Start(accel,temp,true); node; node = iter.Next()){ // For now use iter with true, but I want to rewrite this part so it is not needed and remove the entire Populate thing. It's unecessary and complicated
    FUInstance* inst = node->inst;
    FUDeclaration* decl = inst->declaration;

    InstanceInfo* info = PushListElement(infoList);
    *info = {};

    info->decl = decl;
    info->fullName = iter.GetFullName(out,"_");
    if(decl->configInfo.configs.size && inst->config && !IsConfigStatic(accel,inst)){
      info->configPos = inst->config - accel->configAlloc.ptr;
    }
    if(decl->configInfo.states.size && inst->state){
      info->statePos = inst->state - accel->stateAlloc.ptr;
    }
    if(decl->memoryMapBits.has_value()){
      info->memMapped = (iptr) inst->memMapped;
    }

    info->isComposite = decl->type == FUDeclaration::COMPOSITE;
  }
#endif
  
  Array<InstanceInfo> res = PushArrayFromList<InstanceInfo>(out,infoList);

#if 1
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
#endif
  
  return res;
}

#if 0
Array<String> GetConfigNames(Accelerator* accel,Arena* out,Arena* temp){
  BLOCK_REGION(temp);
  
  AcceleratorIterator iter = {};
  for(InstanceNode* node = iter.Start(accel,temp,false); node; node = iter.Next()){
  }

  return {};
}
#endif

OrderedConfigurations ExtractOrderedConfigurationNames(Versat* versat,Accelerator* accel,Arena* out,Arena* temp){
  UnitValues val = CalculateAcceleratorValues(versat,accel);

  OrderedConfigurations res =  {};
  res.configs = PushArray<Wire>(out,val.configs);
  res.statics = PushArray<Wire>(out,val.statics);
  res.delays  = PushArray<Wire>(out,val.delays);

  for(int i = 0; i < val.configs; i++){
    AcceleratorIterator iter = {};
    for(InstanceNode* node = iter.Start(accel,temp,true); node; node = iter.Next()){
      FUInstance* inst = node->inst;
      FUDeclaration* decl = inst->declaration;

      if(decl->configInfo.configs.size && inst->config && !IsConfigStatic(accel,inst)){
        int index = inst->config - accel->configAlloc.ptr;
        if(index != i){
          continue;
        }

        String name = iter.GetFullName(temp,"_");

        for(int ii = 0; ii < decl->configInfo.configs.size; ii++){
          res.configs[index + ii] = decl->configInfo.configs[ii];
          res.configs[index + ii].name = PushString(out,"%.*s_%.*s",UNPACK_SS(name),UNPACK_SS(decl->configInfo.configs[ii].name));
        }
      }
    }
  }

  for(Pair<StaticId,StaticData>& pair : accel->calculatedStaticPos){
    int offset = pair.second.offset;
    StaticId& id = pair.first;
    StaticData& data = pair.second;
    for(int ii = 0; ii < data.configs.size; ii++){
      String identifier = ReprStaticConfig(id,&data.configs[ii],out);
      res.statics[ii + offset] = data.configs[ii];
      res.statics[ii + offset].name = identifier;
    }
  }

  for(int i = 0; i < val.delays; i++){
    res.delays[i].name = PushString(out,"TOP_Delay%d",i);
    res.delays[i].bitSize = 32; // TODO: For now, delay is set at 32 bits
  }

#if 0
  for(Wire& wire : res.configs){
    BLOCK_REGION(temp);
    String str = Repr(wire,temp);
    printf("%.*s\n",UNPACK_SS(str));
  }
  for(Wire& wire : res.statics){
    BLOCK_REGION(temp);
    String str = Repr(wire,temp);
    printf("%.*s\n",UNPACK_SS(str));
  }
  for(Wire& wire : res.delays){
    BLOCK_REGION(temp);
    String str = Repr(wire,temp);
    printf("%.*s\n",UNPACK_SS(str));
  }
#endif
  
  return res;
}

Array<Wire> OrderedConfigurationsAsArray(OrderedConfigurations ordered,Arena* out){
  int size = ordered.configs.size + ordered.statics.size + ordered.delays.size;
  Array<Wire> res = PushArray<Wire>(out,size);

  int index = 0;
  for(Wire& wire : ordered.configs){
    res[index++] = wire;
  }
  for(Wire& wire : ordered.statics){
    res[index++] = wire;
  }
  for(Wire& wire : ordered.delays){
    res[index++] = wire;
  }

  return res;
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

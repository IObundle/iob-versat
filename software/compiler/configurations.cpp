#include "configurations.hpp"

#include "accelerator.hpp"
#include "debug.hpp"
#include "declaration.hpp"
#include "globals.hpp"
#include "memory.hpp"
#include "parser.hpp"
#include "utils.hpp"
#include "utilsCore.hpp"
#include "versat.hpp"
#include "textualRepresentation.hpp"
#include <strings.h>

Array<InstanceInfo>& AccelInfoIterator::GetCurrentMerge(){
  if(info->infos.size <= 1){
    return info->baseInfo;
  } else {
    return info->infos[mergeIndex].info;
  }
}

int AccelInfoIterator::MergeSize(){
  if(info->infos.size){
    return info->infos.size;
  } else {
    return 1;
  }
}

String AccelInfoIterator::GetMergeName(){
  if(info->infos.size <= 1){
    return {};
  } else {
    return info->infos[mergeIndex].name;
  }
}

bool AccelInfoIterator::IsValid(){
  bool res = (info && index >= 0 && index < GetCurrentMerge().size);
  return res;
}

int AccelInfoIterator::GetIndex(){
  return GetIndex(CurrentUnit());
}

// TODO: Might not work with merge indexes
int AccelInfoIterator::GetIndex(InstanceInfo* instance){
  int i = (instance - &GetCurrentMerge()[0]);
  Assert(i < GetCurrentMerge().size);
  return i;
}

AccelInfoIterator AccelInfoIterator::GetParent(){
  if(!IsValid() || this->index == 0){
    return {};
  }

  int currentLevel = GetCurrentMerge()[this->index].level;
  if(currentLevel == 0){
    return {};
  }

  for(int i = this->index - 1; i >= 0; i--){
    if(GetCurrentMerge()[i].level < currentLevel){
      Assert(GetCurrentMerge()[i].level == currentLevel - 1);
      AccelInfoIterator res = *this;
      res.index = i;
      return res;
    }
  }

  NOT_POSSIBLE("Any valid AccelInfo should never reach this point");
  return {};
}

InstanceInfo* AccelInfoIterator::GetParentUnit(){
  AccelInfoIterator iter = GetParent();
  if(!iter.IsValid()){
    return nullptr;
  }

  return iter.CurrentUnit();
}

InstanceInfo* AccelInfoIterator::CurrentUnit(){
  return &GetCurrentMerge()[index];
}

InstanceInfo* AccelInfoIterator::GetUnit(int indexParam){
  return &GetCurrentMerge()[indexParam];
}

AccelInfoIterator AccelInfoIterator::Next(){
  if(!IsValid()){
    return {};
  }

  int currentLevel = GetCurrentMerge()[index].level;
  for(int i = index + 1; i < GetCurrentMerge().size; i++){
    int level = GetCurrentMerge()[i].level; 
    if(level == currentLevel){
      AccelInfoIterator toReturn = *this;
      toReturn.index = i;
      return toReturn;
    }
    if(level < currentLevel){
      return {};
    }
  }

  return {};
}

AccelInfoIterator AccelInfoIterator::Step(){
  if(!IsValid()){
    return {};
  }

  if(this->index + 1 >= GetCurrentMerge().size){
    return {};
  }
  
  AccelInfoIterator toReturn = *this;
  toReturn.index = this->index + 1;
  return toReturn;
}

AccelInfoIterator AccelInfoIterator::StepInsideOnly(){
  if(!IsValid()){
    return {};
  }

  int currentLevel = GetCurrentMerge()[index].level;
  if(index + 1 < GetCurrentMerge().size && GetCurrentMerge()[index + 1].level > currentLevel){
    AccelInfoIterator toReturn = *this;
    toReturn.index = index + 1;
    return toReturn;
  }

  return {};
}

int AccelInfoIterator::CurrentLevelSize(){
  Array<InstanceInfo>& array = GetCurrentMerge();
  int currentLevel = array[index].level;

  int size = 0;
  for(int i = 0; i < array.size; i++){
    if(array[i].level == currentLevel){
      size += 1;
    }
  }

  return size;
}

Array<InstanceInfo*> GetAllSameLevelUnits(AccelInfo* info,int level,int mergeIndex,Arena* out){
  auto builder = StartArray<InstanceInfo*>(out);

  AccelInfoIterator iter = StartIteration(info);
  iter.mergeIndex = mergeIndex;

  for(; iter.IsValid(); iter = iter.Step()){
    InstanceInfo* unit = iter.CurrentUnit();

    if(unit->level == level){
      *builder.PushElem() = unit;
    }
  }

  Array<InstanceInfo*> res = EndArray(builder);
  return res;
}

AccelInfoIterator StartIteration(AccelInfo* info){
  AccelInfoIterator iter = {};
  iter.info = info;
  return iter;
}
  
CalculatedOffsets CalculateConfigOffsetsIgnoringStatics(Accelerator* accel,Arena* out){
  int size = accel->allocated.Size();

  Array<int> array = PushArray<int>(out,size);

  BLOCK_REGION(out);

  Hashmap<int,int>* sharedConfigs = PushHashmap<int,int>(out,size);

  int offset = 0;
  for(int index = 0; index < accel->allocated.Size(); index++){
    FUInstance* inst = accel->allocated.Get(index);

    // TODO: Temporarely set this to comment. Do not know what it affects.
    // Assert(!(inst->sharedEnable && inst->isStatic));

    int numberConfigs = inst->declaration->configs.size;
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
    size = decl->configs.size;
  }break;
  case MemType::DELAY:{
    size = decl->NumberDelays();
  }break;
  case MemType::STATE:{
    size = decl->states.size;
  }break;
  case MemType::STATIC:{
  }break;
  }

  return size;
}

CalculatedOffsets CalculateConfigurationOffset(Accelerator* accel,MemType type,Arena* out){
  if(type == MemType::CONFIG){
    return CalculateConfigOffsetsIgnoringStatics(accel,out);
  }

  Array<int> array = PushArray<int>(out,accel->allocated.Size());

  BLOCK_REGION(out);

  int offset = 0;
  for(int index = 0; index < accel->allocated.Size(); index++){
    FUInstance* inst = accel->allocated.Get(index);
    array[index] = offset;

    int size = GetConfigurationSize(inst->declaration,type);

    offset += size;
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

        *wire = t.decl->configs[i];
        wire->name = PushString(out,"%.*s_%.*s",UNPACK_SS(t.fullName),UNPACK_SS(t.decl->configs[i].name));
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

          *wire = t.decl->configs[i];
          wire->name = PushString(out,"%.*s_%.*s_%.*s",UNPACK_SS(parentName),UNPACK_SS(t.name),UNPACK_SS(t.decl->configs[i].name));
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
      count += in.decl->states.size;
    }
  }

  Array<String> res = PushArray<String>(out,count);
  int index = 0;
  for(InstanceInfo& in : info){
    if(!in.isComposite && in.statePos.has_value()){
      FUDeclaration* decl = in.decl;
      for(Wire& wire : decl->states){
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
  for(FUInstance* node : accel->allocated){
    FUInstance* subInst = node;
    FUDeclaration* decl = subInst->declaration;

    if(subInst->declaration->ConfigInfoSize() > 1){
      *partitionsArr.PushElem() = (Partition){.value = 0,.max = decl->ConfigInfoSize(),.mergeIndexStart = mergedPossibility};
      mergedPossibility += log2i(decl->ConfigInfoSize());
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

// TODO: Move this function to a better place
// This function cannot return an array for merge units because we do not have the merged units info in this level.
// This function only works for modules and for recursing the merged info to upper modules.
// The info needed by merge must be stored by the merge function.
Array<InstanceInfo> GenerateInitialInstanceInfo(Accelerator* accel,Arena* out,Arena* temp,int mergeIndex = 0){
  auto SetBaseInfo = [](InstanceInfo* elem,FUInstance* inst,int level){
    *elem = {};
    elem->inst = inst;
    elem->name = inst->name;
    elem->decl = inst->declaration;
    elem->level = level;
    elem->isComposite = IsTypeHierarchical(inst->declaration);
    elem->isMerge = inst->declaration->type == FUDeclarationType_MERGED;
    elem->isStatic = inst->isStatic;
    elem->isShared = inst->sharedEnable;
    elem->sharedIndex = inst->sharedIndex;
    elem->isMergeMultiplexer = inst->isMergeMultiplexer;
    elem->special = inst->literal;
    elem->id = inst->id;
    elem->mergeIndexStart = 0;
    elem->inputDelays = inst->declaration->GetInputDelays();
    elem->outputLatencies = inst->declaration->GetOutputLatencies();
    elem->connectionType = inst->type;
  };
  
  auto Function = [SetBaseInfo,mergeIndex](auto Recurse,DynamicArray<InstanceInfo>& array,Accelerator* accel,int level) -> void{
    for(FUInstance* inst : accel->allocated){
      InstanceInfo* elem = array.PushElem();
      SetBaseInfo(elem,inst,level);

      AccelInfoIterator iter = StartIteration(&inst->declaration->info);
      iter.mergeIndex = mergeIndex;
      for(; iter.IsValid(); iter = iter.Step()){
        InstanceInfo* subUnit = iter.CurrentUnit();
        InstanceInfo* elem = array.PushElem();

        *elem = *subUnit;
        elem->level += 1;
      }
    }
  };
  
  auto build = StartArray<InstanceInfo>(out);

  Function(Function,build,accel,0);
  Array<InstanceInfo> res = EndArray(build);

  return res;
}

struct Node{
  InstanceInfo* unit;
  int value;
  // Leafs have both of these at nullptr
  Node* left;
  Node* right;
}; 

Array<InstanceInfo> CalculateInstanceInfoTest(Accelerator* accel,Arena* out,Arena* temp){
  Array<InstanceInfo> test2 = GenerateInitialInstanceInfo(accel,out,temp);
  
  AccelInfo test = {};
  test.baseInfo = test2;

  // Save graph data:
  auto instanceToIndex = PushTrieMap<FUInstance*,int>(temp);

  for(AccelInfoIterator iter = StartIteration(&test); iter.IsValid(); iter = iter.Next()){
    int index = iter.GetIndex();
    InstanceInfo* info = iter.CurrentUnit();
    for(FUInstance* inst : accel->allocated){
      if(inst == info->inst){
        instanceToIndex->Insert(inst,index);
      }
    }
  }

  for(AccelInfoIterator iter = StartIteration(&test); iter.IsValid(); iter = iter.Next()){
    InstanceInfo* info = iter.CurrentUnit();
    FUInstance* inst = info->inst;
    
    ArenaList<SimplePortConnection>* list = PushArenaList<SimplePortConnection>(temp);
    
    FOREACH_LIST(ConnectionNode*,ptr,inst->allInputs){
      FUInstance* out = ptr->instConnectedTo.inst;
      int outPort = ptr->instConnectedTo.port;

      int outIndex = instanceToIndex->GetOrFail(out);

      SimplePortConnection* portInst = list->PushElem();
      portInst->outInst = outIndex;
      portInst->outPort = outPort;
      portInst->inPort = ptr->port;
    }
    info->inputs = PushArrayFromList(out,list);
  }
    
  // Calculate full name
  for(AccelInfoIterator iter = StartIteration(&test); iter.IsValid(); iter = iter.Step()){
    InstanceInfo* unit = iter.CurrentUnit();
    InstanceInfo* parent = iter.GetParentUnit();

    unit->parent = parent ? parent->decl : nullptr;

    // All these are basically to simplify debugging by having everything in one place.
    // They can be moved to GenerateInitialInstanceInfo.
    unit->configSize = unit->decl->NumberConfigs();
    unit->stateSize = unit->decl->NumberStates();
    unit->delaySize = unit->decl->NumberDelays();
    unit->memMappedBitSize = unit->decl->memoryMapBits;

    if(unit->decl->memoryMapBits.has_value()){
      unit->memMappedSize = (1 << unit->decl->memoryMapBits.value());
    }
    
    // Merge stuff but for now use the default value. (Since not dealing with merged for now)
    unit->baseName = unit->name;
    unit->belongs = true;
    
    if(!parent){
      unit->fullName = unit->name;
      continue;
    }

    unit->fullName = PushString(out,"%.*s_%.*s",UNPACK_SS(parent->fullName),UNPACK_SS(unit->name));
  }

  // Config info stuff
  auto CalculateConfig = [](auto Recurse,AccelInfoIterator& iter,int startIndex,Arena* temp) -> void{
    BLOCK_REGION(temp);

    InstanceInfo* parent = iter.GetParentUnit();
    if(parent){
      AccelInfoIterator configIter = StartIteration(&parent->decl->info);
      
      int index = 0;
      for(AccelInfoIterator it = iter; it.IsValid(); it = it.Next(),configIter = configIter.Next()){
        InstanceInfo* unit = it.CurrentUnit();
        InstanceInfo* configUnit = configIter.CurrentUnit();
        
        if(configUnit->configPos.has_value()){
          int configPos = startIndex + configUnit->configPos.value();
        
          if(unit->configSize && !unit->isStatic){
            unit->configPos = configPos;
          }
          
          index += 1;
          AccelInfoIterator inside = it.StepInsideOnly();
          if(inside.IsValid() && !unit->isStatic){
            Recurse(Recurse,inside,configPos,temp);
          }
        }
      }
    } else {
      TrieMap<int,int>* sharedIndexToConfig = PushTrieMap<int,int>(temp);
      
      int configIndex = startIndex;
      for(AccelInfoIterator it = iter; it.IsValid(); it = it.Next()){
        InstanceInfo* unit = it.CurrentUnit();

        bool isNewlyShared = false;
        int indexForCurrentUnit = configIndex;
        if(unit->isShared){
          int sharedIndex = unit->sharedIndex;
          GetOrAllocateResult<int> data = sharedIndexToConfig->GetOrAllocate(sharedIndex);
          if(data.alreadyExisted){
            indexForCurrentUnit = *data.data;
          } else {
            *data.data = configIndex;
            isNewlyShared = true;
          }
        }
        
        if(unit->configSize && !unit->isStatic){
          unit->configPos = indexForCurrentUnit;
        }
        
        AccelInfoIterator inside = it.StepInsideOnly();
        if(inside.IsValid() && !unit->isStatic){
          Recurse(Recurse,inside,indexForCurrentUnit,temp);
        }

        if(!isNewlyShared && !unit->isStatic){
          configIndex += unit->decl->NumberConfigs();
        }
      }
    }
  };

  AccelInfoIterator iter = StartIteration(&test);

  CalculateConfig(CalculateConfig,iter,0,temp);

  // Calculate state stuff
  auto CalculateState = [](auto Recurse,AccelInfoIterator& iter,int startIndex,Arena* temp) -> void{
    BLOCK_REGION(temp);

    InstanceInfo* parent = iter.GetParentUnit();
    if(parent){
      AccelInfoIterator stateIter = StartIteration(&parent->decl->info);
      
      int index = 0;
      for(AccelInfoIterator it = iter; it.IsValid(); it = it.Next(),stateIter = stateIter.Next()){
        InstanceInfo* unit = it.CurrentUnit();
        InstanceInfo* stateUnit = stateIter.CurrentUnit();
        
        if(stateUnit->statePos.has_value()){
          int statePos = startIndex + stateUnit->statePos.value();
          
          if(unit->stateSize){
            unit->statePos = statePos;
          }
          
          index += 1;
          AccelInfoIterator inside = it.StepInsideOnly();
          if(inside.IsValid()){
            Recurse(Recurse,inside,statePos,temp);
          }
        }
      }
    } else {
      int stateIndex = startIndex;
      for(AccelInfoIterator it = iter; it.IsValid(); it = it.Next()){
        InstanceInfo* unit = it.CurrentUnit();

        if(unit->stateSize){
          unit->statePos = stateIndex;
        }
        
        AccelInfoIterator inside = it.StepInsideOnly();
        if(inside.IsValid()){
          Recurse(Recurse,inside,stateIndex,temp);
        }

        stateIndex += unit->stateSize;
      }
    }
  };

  iter = StartIteration(&test);
  CalculateState(CalculateState,iter,0,temp);

  // Handle memory mapping
  auto CalculateMemory = [](auto Recurse,AccelInfoIterator& iter,int startIndex,Arena* out,Arena* temp) -> void{
    BLOCK_REGION(temp);
    
    // Let the inside units calculate their memory mapped first
    for(AccelInfoIterator it = iter; it.IsValid(); it = it.Next()){
      AccelInfoIterator inside = it.StepInsideOnly();
      if(inside.IsValid()){
        Recurse(Recurse,inside,startIndex,out,temp);
      }
    }

    auto builder = StartGrowableArray<Node*>(temp);
    for(AccelInfoIterator it = iter; it.IsValid(); it = it.Next()){
      InstanceInfo* unit = it.CurrentUnit();

      if(unit->memMappedBitSize.has_value()){
        Node* n = PushStruct<Node>(temp);
        *n = (Node){unit,unit->memMappedBitSize.value()};
        *builder.PushElem() = n;
      }
    }
    Array<Node*> baseNodes = EndArray(builder);
    
    if(baseNodes.size == 0){
      return;
    }

    auto Sort = [](Array<Node*>& toSort){
      for(int i = 0; i < toSort.size; i++){
        for(int j = i + 1; j < toSort.size; j++){
          if(toSort[i]->value < toSort[j]->value){
            SWAP(toSort[i],toSort[j]);
          }
        }
      }
    };

    //DEBUG_BREAK();
    Sort(baseNodes);

    while(baseNodes.size > 1){
      Node* left = baseNodes[baseNodes.size - 2];
      Node* right = baseNodes[baseNodes.size - 1];
      
      Node* n = PushStruct<Node>(temp);
      n->value = std::max(left->value,right->value) + 1;
      n->left = left;
      n->right = right;
      baseNodes[baseNodes.size - 2] = n;
      baseNodes.size -= 1;

      Sort(baseNodes);
    }
    
    Node* top = baseNodes[0];
    
    auto NodeRecurse = [](auto Recurse,Node* top,int bitAccum,int index,Arena* out) -> void{
      if(top->left == nullptr){
        Assert(top->right == nullptr);

        InstanceInfo* info = top->unit;

        auto builder = StartString(out);
        for(int i = 0; i < index; i++){
          builder.PushChar(GET_BIT(bitAccum,i) ? '1' : '0');
        }
        String decisionMask = EndString(builder);
        PushNullByte(out);
        info->memDecisionMask = decisionMask;
      } else {
        Recurse(Recurse,top->left,SET_BIT(bitAccum,index),index + 1,out);
        Recurse(Recurse,top->right,bitAccum,index + 1,out);
      }
    };

    NodeRecurse(NodeRecurse,top,0,0,out);
  };

  iter = StartIteration(&test);
  CalculateMemory(CalculateMemory,iter,0,out,temp);

  auto BinaryToInt = [](String val) -> int{
    int res = 0;
    for(int c : val){
      res *= 2;
      res += (c == '1' ? 1 : 0);
    }
    return res;
  };
  
  for(AccelInfoIterator iter = StartIteration(&test); iter.IsValid(); iter = iter.Step()){
    InstanceInfo* unit = iter.CurrentUnit();

    if(unit->memDecisionMask.has_value() && unit->memMappedBitSize.has_value()){
      auto builder = StartString(out);
      builder.PushString(unit->memDecisionMask.value());
      for(int i = 0; i < unit->memMappedBitSize.value(); i++){
        builder.PushChar('0');
      }
      unit->memMappedMask = EndString(builder);
      PushNullByte(out);
      
      unit->memMapped = BinaryToInt(unit->memMappedMask.value());
    }
  }

  // Delay pos is just basically State position without anything different, right?
  auto CalculateDelayPos = [](auto Recurse,AccelInfoIterator& iter,int startIndex,Arena* temp) -> void{
    BLOCK_REGION(temp);

    InstanceInfo* parent = iter.GetParentUnit();
    if(parent){
      AccelInfoIterator delayIter = StartIteration(&parent->decl->info);
      
      int index = 0;
      for(AccelInfoIterator it = iter; it.IsValid(); it = it.Next(),delayIter = delayIter.Next()){
        InstanceInfo* unit = it.CurrentUnit();
        InstanceInfo* delayUnit = delayIter.CurrentUnit();

        if(delayUnit->delayPos.has_value()){
          int delayPos = startIndex + delayUnit->delayPos.value();
        
          if(unit->delaySize){
            unit->delayPos = delayPos;
          }
          
          index += 1;
          AccelInfoIterator inside = it.StepInsideOnly();
          if(inside.IsValid()){
            Recurse(Recurse,inside,delayPos,temp);
          }
        }
      }
    } else {
      int delayIndex = startIndex;
      for(AccelInfoIterator it = iter; it.IsValid(); it = it.Next()){
        InstanceInfo* unit = it.CurrentUnit();

        if(unit->delaySize){
          unit->delayPos = delayIndex;
        }
        
        AccelInfoIterator inside = it.StepInsideOnly();
        if(inside.IsValid()){
          Recurse(Recurse,inside,delayIndex,temp);
        }

        delayIndex += unit->delaySize;
      }
    }
  };
  
  iter = StartIteration(&test);
  CalculateDelayPos(CalculateDelayPos,iter,0,temp);

  DAGOrderNodes order = CalculateDAGOrder(&accel->allocated,temp);
  CalculateDelayResult calculatedDelay = CalculateDelay(accel,order,temp);
 
  auto CalculateDelay = [](auto Recurse,AccelInfoIterator& iter,CalculateDelayResult& calculatedDelay,int startIndex,Arena* out) -> void{
    InstanceInfo* parent = iter.GetParentUnit();
    if(parent){
      AccelInfoIterator delayIter = StartIteration(&parent->decl->info);
      
      int index = 0;
      for(AccelInfoIterator it = iter; it.IsValid(); it = it.Next(),delayIter = delayIter.Next()){
        InstanceInfo* unit = it.CurrentUnit();
        InstanceInfo* delayUnit = delayIter.CurrentUnit();

        int delayPos = startIndex + delayUnit->baseDelay;
        unit->baseDelay = delayPos;

        if(unit->delaySize > 0 && !unit->isComposite){
          unit->delay = PushArray<int>(out,unit->delaySize);
          Memset(unit->delay,unit->baseDelay);
        }
        
        index += 1;
        AccelInfoIterator inside = it.StepInsideOnly();
        if(inside.IsValid()){
          Recurse(Recurse,inside,calculatedDelay,delayPos,out);
        }
      }
    } else {
      for(AccelInfoIterator it = iter; it.IsValid(); it = it.Next()){
        InstanceInfo* unit = it.CurrentUnit();
        
        unit->baseDelay = calculatedDelay.nodeDelay->GetOrFail(unit->inst).value;

        if(unit->delaySize > 0 && !unit->isComposite){
          unit->delay = PushArray<int>(out,unit->delaySize);
          Memset(unit->delay,unit->baseDelay);
        }

        AccelInfoIterator inside = it.StepInsideOnly();
        if(inside.IsValid()){
          Recurse(Recurse,inside,calculatedDelay,unit->baseDelay,out);
        }
      }
    }
  };

  iter = StartIteration(&test);
  CalculateDelay(CalculateDelay,iter,calculatedDelay,0,out);

  {
    iter = StartIteration(&test);
    for(AccelInfoIterator it = iter; it.IsValid(); it = it.Next()){
      InstanceInfo* info = it.CurrentUnit();

      FUInstance* inst = info->inst;
      for(int i = 0; i < order.instances.size; i++){
        if(order.instances[i] == inst){
          info->localOrder = i;
          break;
        }
      }
    }
  }
  
  return test2;
}

void CalculateInstanceInfoTest(AccelInfoIterator initialIter,Accelerator* accel,Arena* out,Arena* temp){
  // Calculate full name
  for(AccelInfoIterator iter = initialIter; iter.IsValid(); iter = iter.Step()){
    InstanceInfo* unit = iter.CurrentUnit();
    InstanceInfo* parent = iter.GetParentUnit();
    unit->parent = parent ? parent->decl : nullptr;

    // All these are basically to simplify debugging by having everything in one place.
    // They can be moved to GenerateInitialInstanceInfo.
    unit->configSize = unit->decl->NumberConfigs();
    unit->stateSize = unit->decl->NumberStates();
    unit->delaySize = unit->decl->NumberDelays();
    unit->memMappedBitSize = unit->decl->memoryMapBits;

    if(unit->decl->memoryMapBits.has_value()){
      unit->memMappedSize = (1 << unit->decl->memoryMapBits.value());
    }
    
    // Merge stuff but for now use the default value. (Since not dealing with merged for now)
    unit->baseName = unit->name;
    unit->belongs = true;
    
    if(!parent){
      unit->fullName = unit->name;
      continue;
    }

    unit->fullName = PushString(out,"%.*s_%.*s",UNPACK_SS(parent->fullName),UNPACK_SS(unit->name));
  }

  // Config info stuff
  auto CalculateConfig = [](auto Recurse,AccelInfoIterator& iter,int startIndex,Arena* temp) -> void{
    BLOCK_REGION(temp);

    InstanceInfo* parent = iter.GetParentUnit();
    if(parent){
      AccelInfoIterator configIter = StartIteration(&parent->decl->info);
      
      int index = 0;
      for(AccelInfoIterator it = iter; it.IsValid(); it = it.Next(),configIter = configIter.Next()){
        InstanceInfo* unit = it.CurrentUnit();
        InstanceInfo* configUnit = configIter.CurrentUnit();
        
        if(configUnit->configPos.has_value()){
          int configPos = startIndex + configUnit->configPos.value();
        
          if(unit->configSize && !unit->isStatic){
            unit->configPos = configPos;
          }
          
          index += 1;
          AccelInfoIterator inside = it.StepInsideOnly();
          if(inside.IsValid() && !unit->isStatic){
            Recurse(Recurse,inside,configPos,temp);
          }
        }
      }
    } else {
      TrieMap<int,int>* sharedIndexToConfig = PushTrieMap<int,int>(temp);
      
      int configIndex = startIndex;
      for(AccelInfoIterator it = iter; it.IsValid(); it = it.Next()){
        InstanceInfo* unit = it.CurrentUnit();

        bool isNewlyShared = false;
        int indexForCurrentUnit = configIndex;
        if(unit->isShared){
          int sharedIndex = unit->sharedIndex;
          GetOrAllocateResult<int> data = sharedIndexToConfig->GetOrAllocate(sharedIndex);
          if(data.alreadyExisted){
            indexForCurrentUnit = *data.data;
          } else {
            *data.data = configIndex;
            isNewlyShared = true;
          }
        }
        
        if(unit->configSize && !unit->isStatic){
          unit->configPos = indexForCurrentUnit;
        }
        
        AccelInfoIterator inside = it.StepInsideOnly();
        if(inside.IsValid() && !unit->isStatic){
          Recurse(Recurse,inside,indexForCurrentUnit,temp);
        }

        if(!isNewlyShared && !unit->isStatic){
          configIndex += unit->decl->NumberConfigs();
        }
      }
    }
  };

  CalculateConfig(CalculateConfig,initialIter,0,temp);

  // Calculate state stuff
  auto CalculateState = [](auto Recurse,AccelInfoIterator& iter,int startIndex,Arena* temp) -> void{
    BLOCK_REGION(temp);

    InstanceInfo* parent = iter.GetParentUnit();
    if(parent){
      AccelInfoIterator stateIter = StartIteration(&parent->decl->info);
      
      int index = 0;
      for(AccelInfoIterator it = iter; it.IsValid(); it = it.Next(),stateIter = stateIter.Next()){
        InstanceInfo* unit = it.CurrentUnit();
        InstanceInfo* stateUnit = stateIter.CurrentUnit();
        
        if(stateUnit->statePos.has_value()){
          int statePos = startIndex + stateUnit->statePos.value();
          
          if(unit->stateSize){
            unit->statePos = statePos;
          }
          
          index += 1;
          AccelInfoIterator inside = it.StepInsideOnly();
          if(inside.IsValid()){
            Recurse(Recurse,inside,statePos,temp);
          }
        }
      }
    } else {
      int stateIndex = startIndex;
      for(AccelInfoIterator it = iter; it.IsValid(); it = it.Next()){
        InstanceInfo* unit = it.CurrentUnit();

        if(unit->stateSize){
          unit->statePos = stateIndex;
        }
        
        AccelInfoIterator inside = it.StepInsideOnly();
        if(inside.IsValid()){
          Recurse(Recurse,inside,stateIndex,temp);
        }

        stateIndex += unit->stateSize;
      }
    }
  };

  CalculateState(CalculateState,initialIter,0,temp);

  // Handle memory mapping
  auto CalculateMemory = [](auto Recurse,AccelInfoIterator& iter,int startIndex,Arena* out,Arena* temp) -> void{
    BLOCK_REGION(temp);
    
    // Let the inside units calculate their memory mapped first
    for(AccelInfoIterator it = iter; it.IsValid(); it = it.Next()){
      AccelInfoIterator inside = it.StepInsideOnly();
      if(inside.IsValid()){
        Recurse(Recurse,inside,startIndex,out,temp);
      }
    }

    auto builder = StartGrowableArray<Node*>(temp);
    for(AccelInfoIterator it = iter; it.IsValid(); it = it.Next()){
      InstanceInfo* unit = it.CurrentUnit();

      if(unit->memMappedBitSize.has_value()){
        Node* n = PushStruct<Node>(temp);
        *n = (Node){unit,unit->memMappedBitSize.value()};
        *builder.PushElem() = n;
      }
    }
    Array<Node*> baseNodes = EndArray(builder);
    
    if(baseNodes.size == 0){
      return;
    }

    auto Sort = [](Array<Node*>& toSort){
      for(int i = 0; i < toSort.size; i++){
        for(int j = i + 1; j < toSort.size; j++){
          if(toSort[i]->value < toSort[j]->value){
            SWAP(toSort[i],toSort[j]);
          }
        }
      }
    };

    Sort(baseNodes);

    while(baseNodes.size > 1){
      Node* left = baseNodes[baseNodes.size - 2];
      Node* right = baseNodes[baseNodes.size - 1];
      
      Node* n = PushStruct<Node>(temp);
      n->value = std::max(left->value,right->value) + 1;
      n->left = left;
      n->right = right;
      baseNodes[baseNodes.size - 2] = n;
      baseNodes.size -= 1;

      Sort(baseNodes);
    }
    
    Node* top = baseNodes[0];
    
    auto NodeRecurse = [](auto Recurse,Node* top,int bitAccum,int index,Arena* out) -> void{
      if(top->left == nullptr){
        Assert(top->right == nullptr);

        InstanceInfo* info = top->unit;

        auto builder = StartString(out);
        for(int i = 0; i < index; i++){
          builder.PushChar(GET_BIT(bitAccum,i) ? '1' : '0');
        }
        String decisionMask = EndString(builder);
        PushNullByte(out);
        info->memDecisionMask = decisionMask;
      } else {
        Recurse(Recurse,top->left,SET_BIT(bitAccum,index),index + 1,out);
        Recurse(Recurse,top->right,bitAccum,index + 1,out);
      }
    };

    NodeRecurse(NodeRecurse,top,0,0,out);
  };

  CalculateMemory(CalculateMemory,initialIter,0,out,temp);

  auto BinaryToInt = [](String val) -> int{
    int res = 0;
    for(int c : val){
      res *= 2;
      res += (c == '1' ? 1 : 0);
    }
    return res;
  };
  
  for(AccelInfoIterator iter = initialIter; iter.IsValid(); iter = iter.Step()){
    InstanceInfo* unit = iter.CurrentUnit();

    if(unit->memDecisionMask.has_value() && unit->memMappedBitSize.has_value()){
      auto builder = StartString(out);
      builder.PushString(unit->memDecisionMask.value());
      for(int i = 0; i < unit->memMappedBitSize.value(); i++){
        builder.PushChar('0');
      }
      unit->memMappedMask = EndString(builder);
      PushNullByte(out);
      
      unit->memMapped = BinaryToInt(unit->memMappedMask.value());
    }
  }

  // Delay pos is just basically State position without anything different, right?
  auto CalculateDelayPos = [](auto Recurse,AccelInfoIterator& iter,int startIndex,Arena* temp) -> void{
    BLOCK_REGION(temp);

    InstanceInfo* parent = iter.GetParentUnit();
    if(parent){
      AccelInfoIterator delayIter = StartIteration(&parent->decl->info);
      
      int index = 0;
      for(AccelInfoIterator it = iter; it.IsValid(); it = it.Next(),delayIter = delayIter.Next()){
        InstanceInfo* unit = it.CurrentUnit();
        InstanceInfo* delayUnit = delayIter.CurrentUnit();

        if(delayUnit->delayPos.has_value()){
          int delayPos = startIndex + delayUnit->delayPos.value();
        
          if(unit->delaySize){
            unit->delayPos = delayPos;
          }
          
          index += 1;
          AccelInfoIterator inside = it.StepInsideOnly();
          if(inside.IsValid()){
            Recurse(Recurse,inside,delayPos,temp);
          }
        }
      }
    } else {
      int delayIndex = startIndex;
      for(AccelInfoIterator it = iter; it.IsValid(); it = it.Next()){
        InstanceInfo* unit = it.CurrentUnit();

        if(unit->delaySize){
          unit->delayPos = delayIndex;
        }
        
        AccelInfoIterator inside = it.StepInsideOnly();
        if(inside.IsValid()){
          Recurse(Recurse,inside,delayIndex,temp);
        }

        delayIndex += unit->delaySize;
      }
    }
  };
  
  CalculateDelayPos(CalculateDelayPos,initialIter,0,temp);

  auto SetDelays = [](auto Recurse,AccelInfoIterator& iter,SimpleCalculateDelayResult& calculatedDelay,int startIndex,Arena* out) -> void{
    InstanceInfo* parent = iter.GetParentUnit();
    if(parent){
      AccelInfoIterator delayIter = StartIteration(&parent->decl->info);
      
      int index = 0;
      for(AccelInfoIterator it = iter; it.IsValid(); it = it.Next(),delayIter = delayIter.Next()){
        InstanceInfo* unit = it.CurrentUnit();
        InstanceInfo* delayUnit = delayIter.CurrentUnit();

        int delayPos = startIndex + delayUnit->baseDelay;
        unit->baseDelay = delayPos;

        if(unit->delaySize > 0 && !unit->isComposite){
          unit->delay = PushArray<int>(out,unit->delaySize);
          Memset(unit->delay,unit->baseDelay);
        }
        
        index += 1;
        AccelInfoIterator inside = it.StepInsideOnly();
        if(inside.IsValid()){
          Recurse(Recurse,inside,calculatedDelay,delayPos,out);
        }
      }
    } else {
      for(AccelInfoIterator it = iter; it.IsValid(); it = it.Next()){
        InstanceInfo* unit = it.CurrentUnit();
        int order = unit->localOrder;

        unit->baseDelay = calculatedDelay.nodeDelayByOrder[order].value;
        unit->portDelay = CopyArray(calculatedDelay.inputPortDelayByOrder[order],out);
        
        if(unit->delaySize > 0 && !unit->isComposite){
          unit->delay = PushArray<int>(out,unit->delaySize);
          Memset(unit->delay,unit->baseDelay);
        }

        AccelInfoIterator inside = it.StepInsideOnly();
        if(inside.IsValid()){
          Recurse(Recurse,inside,calculatedDelay,unit->baseDelay,out);
        }
      }
    }
  };

  SimpleCalculateDelayResult delays = CalculateDelay(initialIter,out,temp);
  SetDelays(SetDelays,initialIter,delays,0,out);
}

AccelInfo CalculateAcceleratorInfo(Accelerator* accel,bool recursive,Arena* out,Arena* temp){
  AccelInfo result = {};
  Array<Partition> partitions = GenerateInitialPartitions(accel,temp);
  int partitionSize = 0;
  while(1){
    partitionSize += 1;
    
    if(!Next(partitions)){
      break;
    }
  }
  
  // MARKED
  int size = partitionSize;
  size = partitionSize;
  result.infos = PushArray<MergePartition>(out,size);

  auto ExtractInputDelays = [](AccelInfoIterator top,Arena* out) -> Array<int>{
    int maxInputs = -1;
    for(AccelInfoIterator iter = top; iter.IsValid(); iter = iter.Next()){
      InstanceInfo* info = iter.CurrentUnit();
      if(info->decl == BasicDeclaration::input){
        maxInputs = std::max(info->special,maxInputs);
      }
    }
    if(maxInputs == -1){
      return {};
    }

    Array<int> inputDelays = PushArray<int>(out,maxInputs + 1);
    for(AccelInfoIterator iter = top; iter.IsValid(); iter = iter.Next()){
      InstanceInfo* info = iter.CurrentUnit();
      if(info->decl == BasicDeclaration::input){
        inputDelays[info->special] = info->baseDelay;
      }
    }

    return inputDelays;
  };

  auto ExtractOutputDelays = [](AccelInfoIterator top,Arena* out) -> Array<int>{
    for(AccelInfoIterator iter = top; iter.IsValid(); iter = iter.Next()){
      InstanceInfo* info = iter.CurrentUnit();
      if(info->decl == BasicDeclaration::output){
        return CopyArray(info->portDelay,out);
      }
    }

    return {};
  };
  
  auto CalculateMuxConfigs = [](AccelInfoIterator top,Arena* out) -> Array<int>{
    STACK_ARENA(tempInst,Kilobyte(4));
    Arena* temp = &tempInst;
    
    // We need to take into account config sharing because it basically converts N configs into 1.
    Set<int>* seen = PushSet<int>(temp,10); // TODO: HACK, eventually this code will be removed, so no problem hacking away.
    
    int amountOfMuxConfigs = 0;
    for(AccelInfoIterator iter = top; iter.IsValid(); iter = iter.Step()){
      InstanceInfo* info = iter.CurrentUnit();
      if(info->isMergeMultiplexer){
        if(seen->Exists(info->configPos.value())){
          continue;
        }
        amountOfMuxConfigs += 1;
        seen->Insert(info->configPos.value());
      }
    }

    if(amountOfMuxConfigs == 0){
      return {};
    }
    
    seen = PushSet<int>(temp,10); // TODO: HACK, eventually this code will be removed, so no problem hacking away.
    Array<int> muxConfigs = PushArray<int>(out,amountOfMuxConfigs);
    int index = 0;
    for(AccelInfoIterator iter = top; iter.IsValid(); iter = iter.Step()){
      InstanceInfo* info = iter.CurrentUnit();
      if(info->isMergeMultiplexer){
        if(seen->Exists(info->configPos.value())){
          continue;
        }
        muxConfigs[index++] = info->mergePort;
        seen->Insert(info->configPos.value());
      }
    }
    
    return muxConfigs;
  };

#if 1
  AccelInfo temp2 = {};// = result;
  temp2.infos = PushArray<MergePartition>(globalPermanent,size);
  
  temp2.baseInfo = CalculateInstanceInfoTest(accel,globalPermanent,temp);
  AccelInfoIterator iter = StartIteration(&temp2);

  if(iter.MergeSize() > 1){
    for(int i = 0; i < iter.MergeSize(); i++){
      temp2.infos[i] = result.infos[i]; // This part must be removed as well in order to remove result.
      
      temp2.infos[i].info = GenerateInitialInstanceInfo(accel,globalPermanent,temp,i);
      
      iter.mergeIndex = i;
    
      CalculateInstanceInfoTest(iter,accel,globalPermanent,temp);
      temp2.infos[i].inputDelays = ExtractInputDelays(iter,out);
      temp2.infos[i].outputLatencies = ExtractOutputDelays(iter,out);
      Array<int> muxConfigs = CalculateMuxConfigs(iter,out);
      temp2.infos[i].muxConfigs = muxConfigs; //CalculateMuxConfigs(iter,out);
    }
  } else {
    temp2.infos[0] = result.infos[0];
    
#if 1
    temp2.infos[0].info = GenerateInitialInstanceInfo(accel,globalPermanent,temp);
    for(InstanceInfo& f : temp2.infos[0].info){
      f.belongs = true;
    }
    iter.mergeIndex = 0;
    CalculateInstanceInfoTest(iter,accel,globalPermanent,temp);

    temp2.infos[0].inputDelays = ExtractInputDelays(iter,out);
    temp2.infos[0].outputLatencies = ExtractOutputDelays(iter,out);
    temp2.infos[0].muxConfigs = CalculateMuxConfigs(iter,out);
#endif
  }

  //temp2.baseInfo = temp2.infos[0].info;
  
  //DEBUG_BREAK();
  
  result = temp2;
#else 
  Array<InstanceInfo> test2 = CalculateInstanceInfoTest(accel,globalPermanent,temp);
  result.baseInfo = test2;
#endif
    
  //result.memMappedBitsize = totalMaxBitsize;

  std::vector<bool> seenShared;

  int memoryMappedDWords = 0;

  // Handle non-static information
  for(FUInstance* ptr : accel->allocated){
    FUInstance* inst = ptr;
    FUDeclaration* type = inst->declaration;

    // Check if shared
    if(inst->sharedEnable){
      if(inst->sharedIndex >= (int) seenShared.size()){
        seenShared.resize(inst->sharedIndex + 1);
      }

      if(!seenShared[inst->sharedIndex]){
        result.configs += type->configs.size;
        result.sharedUnits += 1;
      }

      seenShared[inst->sharedIndex] = true;
    } else if(!inst->isStatic){ // Shared cannot be static
         result.configs += type->configs.size;
    }

    if(type->memoryMapBits.has_value()){
      result.isMemoryMapped = true;

      memoryMappedDWords = AlignBitBoundary(memoryMappedDWords,type->memoryMapBits.value());
      memoryMappedDWords += (1 << type->memoryMapBits.value());
    }

    if(type == BasicDeclaration::input){
      result.inputs += 1;
    }

    result.states += type->states.size;
    result.delays += type->NumberDelays();
    result.ios += type->nIOs;

    if(type->externalMemory.size){
      result.externalMemoryInterfaces += type->externalMemory.size;
    }

    result.signalLoop |= type->signalLoop;
  }

  result.memoryMappedBits = log2i(memoryMappedDWords);

  FUInstance* outputInstance = GetOutputInstance(&accel->allocated);
  if(outputInstance){
    EdgeIterator iter = IterateEdges(accel);
    while(iter.HasNext()){
      Edge edgeInst = iter.Next();
      Edge* edge = &edgeInst;

      if(edge->units[0].inst == outputInstance){
        result.outputs = std::max(result.outputs - 1,edge->units[0].port) + 1;
      }
      if(edge->units[1].inst == outputInstance){
        result.outputs = std::max(result.outputs - 1,edge->units[1].port) + 1;
      }
    }
  }

  Hashmap<StaticId,int>* staticSeen = PushHashmap<StaticId,int>(temp,1000);
  // Handle static information
  for(FUInstance* ptr : accel->allocated){
    FUInstance* inst = ptr;
    if(inst->isStatic){
      StaticId id = {};

      id.name = inst->name;

      staticSeen->Insert(id,1);
      result.statics += inst->declaration->configs.size;

      for(Wire& wire : inst->declaration->configs){
        result.staticBits += wire.bitSize;
      }
    }

    if(IsTypeHierarchical(inst->declaration)){
      for(Pair<StaticId,StaticData*> pair : inst->declaration->staticUnits){
        int* possibleFind = staticSeen->Get(pair.first);

        if(!possibleFind){
          staticSeen->Insert(pair.first,1);
          result.statics += pair.second->configs.size;

          for(Wire& wire : pair.second->configs){
            result.staticBits += wire.bitSize;
          }
        }
      }
    }
  }
  
  for(FUInstance* ptr : accel->allocated){
    FUInstance* inst = ptr;
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
#if 0
    int lastMem = -1;
    for(InstanceInfo& info : sameLevel){
      if(info.memMapped.has_value()){
        int mem = info.memMapped.value(); 
        Assert(mem > lastMem);
        lastMem = info.memMapped.value();
      }
    }
#endif    
  }
}

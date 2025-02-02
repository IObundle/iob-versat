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
  return info->infos[mergeIndex].info;
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
  auto builder = StartGrowableArray<InstanceInfo*>(out);

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

Array<Wire> ExtractAllConfigs(Array<InstanceInfo> info,Arena* out){
  TEMP_REGION(temp,out);

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
        Wire* wire = list->PushElem();

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
        seen->Insert(id);
        for(int i = 0; i < t.configSize; i++){
          Wire* wire = list->PushElem();

          *wire = t.decl->configs[i];
          wire->name = PushString(out,"%.*s_%.*s_%.*s",UNPACK_SS(parentName),UNPACK_SS(t.name),UNPACK_SS(t.decl->configs[i].name));
        }
      }
    }
  }

  for(int i = 0; i < info[0].delaySize; i++){
    Wire* wire = list->PushElem();

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

static bool Next(Array<Partition> arr){
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

// TODO: This function does not do what I wanted it to do.
//       I am mixing up merge and composite. For simple merges, we can only be at a single type at any given point.
//       Composites makes so we are currently at N types at any point, but this function is not calculating data.
//       For a merge of merges, this function must return one type.
Array<AccelInfoIterator> GetCurrentPartitionsAsIterators(AccelInfoIterator iter,Arena* out){
  TEMP_REGION(temp,out);
  // TODO: This is dependent on partitions. Not sure how I feel about that.
  // TODO: BAD
  Hashmap<FUDeclaration*,int>* seenAmount = PushHashmap<FUDeclaration*,int>(temp,10);
  AccelInfo* info = iter.info;
  for(int i = 0; i < info->infos.size; i++){
    FUDeclaration* type = info->infos[i].baseType;
    int amount = *seenAmount->GetOrInsert(type,0);
    seenAmount->Insert(type,amount + 1);
  }

  Array<Partition> partitions = PushArray<Partition>(temp,seenAmount->nodesUsed);
  int index = 0;
  for(Pair<FUDeclaration*,int*> p : seenAmount){
    partitions[index].decl = p.first;
    partitions[index].max = *p.second;
    index += 1;
  }
  
  for(int i = 0; i < iter.mergeIndex; i++){
    Next(partitions);
  }

  Array<AccelInfoIterator> iterators = PushArray<AccelInfoIterator>(out,partitions.size);
  for(int i = 0; i <  partitions.size; i++){
    Partition p = partitions[i];

    iterators[i] = StartIteration(&p.decl->info);
    iterators[i].mergeIndex = p.value;
    Assert(p.max == p.decl->info.infos.size);
  }
  
  return iterators;
}

// This function returns what we want.
// TODO: This whole thing is not good. Make it work first and check how to simplify.
AccelInfoIterator GetCurrentPartitionTypeAsIterator(AccelInfoIterator iter,Arena* out){
  TEMP_REGION(temp,out);
  // TODO: This is dependent on partitions. Not sure how I feel about that.

  int mergeIndex = iter.mergeIndex;
  
  // TODO: BAD
  Hashmap<FUDeclaration*,int>* seenAmount = PushHashmap<FUDeclaration*,int>(temp,10);
  AccelInfo* info = iter.info;
  for(int i = 0; i < info->infos.size; i++){
    FUDeclaration* type = info->infos[i].baseType;
    int amount = *seenAmount->GetOrInsert(type,0);
    seenAmount->Insert(type,amount + 1);
  }

  Array<Partition> partitions = PushArray<Partition>(temp,seenAmount->nodesUsed);
  int index = 0;
  for(Pair<FUDeclaration*,int*> p : seenAmount){
    partitions[index].decl = p.first;
    partitions[index].max = *p.second;
    index += 1;
  }

  for(Partition p : partitions){
    if(mergeIndex < p.max){
      FUDeclaration* type = p.decl;
      AccelInfoIterator res = StartIteration(&type->info);
      res.mergeIndex = mergeIndex;
      return res;
    } else {
      mergeIndex -= p.max;
    }
  }

  NOT_POSSIBLE("");
  return {};
}

int GetPartitionIndex(AccelInfoIterator iter){
  TEMP_REGION(temp,nullptr);
  // TODO: This is dependent on partitions. Not sure how I feel about that.

  int mergeIndex = iter.mergeIndex;
  
  // TODO: BAD
  Hashmap<FUDeclaration*,int>* seenAmount = PushHashmap<FUDeclaration*,int>(temp,10);
  AccelInfo* info = iter.info;
  for(int i = 0; i < info->infos.size; i++){
    FUDeclaration* type = info->infos[i].baseType;
    int amount = *seenAmount->GetOrInsert(type,0);
    seenAmount->Insert(type,amount + 1);
  }

  Array<Partition> partitions = PushArray<Partition>(temp,seenAmount->nodesUsed);
  int index = 0;
  for(Pair<FUDeclaration*,int*> p : seenAmount){
    partitions[index].decl = p.first;
    partitions[index].max = *p.second;
    index += 1;
  }

  int partitionIndex = 0;
  for(Partition p : partitions){
    if(mergeIndex < p.max){
      return partitionIndex;
    } else {
      mergeIndex -= p.max;
      partitionIndex += 1;
    }
  }

  NOT_POSSIBLE("");
  return -1;
}

Array<Partition> GenerateInitialPartitions(Accelerator* accel,Arena* out){
  auto partitionsArr = StartGrowableArray<Partition>(out);
  int mergedPossibility = 0;
  for(FUInstance* node : accel->allocated){
    FUDeclaration* decl = node->declaration;

    if(decl->MergePartitionSize() > 1){
      *partitionsArr.PushElem() = (Partition){.value = 0,.max = decl->MergePartitionSize(),.mergeIndexStart = mergedPossibility,.decl = decl};
      mergedPossibility += log2i(decl->MergePartitionSize());
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

String GetName(Array<Partition> partitions,Arena* out){
  auto builder = StartString(out);

  bool first = true;
  for(Partition p : partitions){
    if(first){
      first = false;
    } else {
      builder.PushString("_");
    }

    AccelInfo* info = &p.decl->info;
    builder.PushString(info->infos[p.value].name);
  }

  return EndString(builder);
}

// TODO: Move this function to a better place
// This function cannot return an array for merge units because we do not have the merged units info in this level.
// This function only works for modules and for recursing the merged info to upper modules.
// The info needed by merge must be stored by the merge function.
Array<InstanceInfo> GenerateInitialInstanceInfo(Accelerator* accel,Arena* out,Array<Partition> partitions){
  TEMP_REGION(temp,out);
  // Only for basic units on the top level of accel
  // The subunits are basically copied from the sub declarations.
  // Or they are calculated inside the Fill... function
  auto SetBaseInfo = [](InstanceInfo* elem,FUInstance* inst,int level){
    *elem = {};
    elem->inst = inst;
    elem->name = inst->name;
    elem->baseName = inst->name; // Remember this function only sets basic units, so the baseName is just the name.
    elem->decl = inst->declaration;
    elem->level = level;
    elem->isComposite = IsTypeHierarchical(inst->declaration);
    elem->isMerge = inst->declaration->type == FUDeclarationType_MERGED;
    elem->isStatic = inst->isStatic;
    elem->isShared = inst->sharedEnable;
    elem->sharedIndex = inst->sharedIndex;
    elem->isMergeMultiplexer = inst->isMergeMultiplexer;
    elem->special = inst->literal;
    elem->muxGroup = inst->muxGroup;
    elem->id = inst->id;
    elem->inputDelays = inst->declaration->GetInputDelays();
    elem->outputLatencies = inst->declaration->GetOutputLatencies();
    elem->connectionType = inst->type;
    elem->partitionIndex = 0;
  };
  
  auto Function = [SetBaseInfo](auto Recurse,GrowableArray<InstanceInfo>& array,Accelerator* accel,int level,Array<Partition> partitions) -> void{
    int partitionIndex = 0;
    for(FUInstance* inst : accel->allocated){
      InstanceInfo* elem = array.PushElem();
      SetBaseInfo(elem,inst,level);

      AccelInfoIterator iter = StartIteration(&inst->declaration->info);

      if(partitions.size > 0 && inst->declaration->info.infos.size > 1){
        iter.mergeIndex = partitions[partitionIndex].value;
        elem->outputLatencies = partitions[partitionIndex].decl->info.infos[iter.mergeIndex].outputLatencies;
        elem->partitionIndex = partitions[partitionIndex].value;
        partitionIndex += 1;
      }
      
      for(; iter.IsValid(); iter = iter.Step()){
        InstanceInfo* subUnit = iter.CurrentUnit();
        InstanceInfo* elem = array.PushElem();

        *elem = *subUnit;
        elem->level += 1;
      }
    }
  };
  
  auto build = StartGrowableArray<InstanceInfo>(out);

  Function(Function,build,accel,0,partitions);
  Array<InstanceInfo> res = EndArray(build);

  auto instanceToIndex = PushTrieMap<FUInstance*,int>(temp);

  for(int i = 0; i < res.size; i++){
    int index = i;
    InstanceInfo* info = &res[i];
    if(info->level != 0){
      continue;
    }
    
    for(FUInstance* inst : accel->allocated){
      if(inst == info->inst){
        instanceToIndex->Insert(inst,index);
      }
    }
  }

  for(int i = 0; i < res.size; i++){
    InstanceInfo* info = &res[i];
    if(info->level != 0){
      continue;
    }

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

  DAGOrderNodes order = CalculateDAGOrder(&accel->allocated,temp);
  {
    for(int index = 0; index < res.size; index++){
      InstanceInfo* info = &res[index];

      FUInstance* inst = info->inst;
      for(int i = 0; i < order.instances.size; i++){
        if(order.instances[i] == inst){
          info->localOrder = i;
          break;
        }
      }
    }
  }
  
  return res;
}

struct Node{
  InstanceInfo* unit;
  int value;
  // Leafs have both of these at nullptr
  Node* left;
  Node* right;
}; 

void FillInstanceInfo(AccelInfoIterator initialIter,Arena* out){
  TEMP_REGION(temp,out);
  Assert(!Empty(initialIter.accelName)); // For now, we must have some name if only to output debug info graphs and such
  
  // Calculate full name
  for(AccelInfoIterator iter = initialIter; iter.IsValid(); iter = iter.Step()){
    InstanceInfo* unit = iter.CurrentUnit();
    InstanceInfo* parent = iter.GetParentUnit();
    unit->parent = parent ? parent->decl : nullptr;

    // All these are basically to simplify debugging by having everything in one place.
    // They can be moved to GenerateInitialInstanceInfo. I think. TODO
    unit->configSize = unit->decl->NumberConfigs();
    unit->stateSize = unit->decl->NumberStates();
    unit->delaySize = unit->decl->NumberDelays();
    unit->memMappedBitSize = unit->decl->memoryMapBits;

    if(unit->decl->memoryMapBits.has_value()){
      unit->memMappedSize = (1 << unit->decl->memoryMapBits.value());
    }
    
    if(!parent){
      unit->fullName = unit->name;
      continue;
    }

    unit->fullName = PushString(out,"%.*s_%.*s",UNPACK_SS(parent->fullName),UNPACK_SS(unit->name));
  }

  // Config info stuff
  auto CalculateConfig = [](auto Recurse,AccelInfoIterator& iter,int startIndex) -> void{
    TEMP_REGION(temp,nullptr);

    InstanceInfo* parent = iter.GetParentUnit();
    if(parent){
      AccelInfoIterator configIter = StartIteration(&parent->decl->info);
      configIter.mergeIndex = parent->partitionIndex;
      
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
            Recurse(Recurse,inside,configPos);
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
          Recurse(Recurse,inside,indexForCurrentUnit);
        }

        if(!isNewlyShared && !unit->isStatic){
          configIndex += unit->decl->NumberConfigs();
        }
      }
    }
  };

  CalculateConfig(CalculateConfig,initialIter,0);

  // Calculate state stuff
  auto CalculateState = [](auto Recurse,AccelInfoIterator& iter,int startIndex) -> void{
    TEMP_REGION(temp,nullptr);

    InstanceInfo* parent = iter.GetParentUnit();
    if(parent){
      AccelInfoIterator stateIter = StartIteration(&parent->decl->info);
      stateIter.mergeIndex = parent->partitionIndex;
      
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
            Recurse(Recurse,inside,statePos);
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
          Recurse(Recurse,inside,stateIndex);
        }

        stateIndex += unit->stateSize;
      }
    }
  };

  CalculateState(CalculateState,initialIter,0);

  // Handle memory mapping
  auto CalculateMemory = [](auto Recurse,AccelInfoIterator& iter,iptr currentMem,Arena* out) -> void{
    TEMP_REGION(temp,out);
    
    InstanceInfo* parent = iter.GetParentUnit();
    if(parent){
      AccelInfoIterator parentIter = StartIteration(&parent->decl->info);
      parentIter.mergeIndex = parent->partitionIndex;

      for(AccelInfoIterator it = iter; it.IsValid(); it = it.Next(),parentIter = parentIter.Next()){
        InstanceInfo* unit = it.CurrentUnit();
        InstanceInfo* memUnit = parentIter.CurrentUnit();
        
        if(memUnit->memMapped.has_value()){
          unit->memMapped = memUnit->memMapped.value() + currentMem;
          
          AccelInfoIterator inside = it.StepInsideOnly();
          if(inside.IsValid()){
            Recurse(Recurse,inside,unit->memMapped.value(),out);
          }
        }
      }
    } else {
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
      
      auto NodeRecurse = [](auto Recurse,Node* top,int max,int bitAccum,Arena* out) -> void{
        if(top->left == nullptr){
          Assert(top->right == nullptr);

          InstanceInfo* info = top->unit;
          
          auto builder = StartString(out);
          for(int i = max - 1; i >= top->value; i--){
            builder.PushChar(GET_BIT(bitAccum,i) ? '1' : '0');
          }
          String decisionMask = EndString(builder);
          PushNullByte(out);
          info->memDecisionMask = decisionMask;

          info->memMapped = bitAccum;
        } else {
          Recurse(Recurse,top->left,max,SET_BIT(bitAccum,top->left->value),out);
          Recurse(Recurse,top->right,max,bitAccum,out);
        }
      };

      NodeRecurse(NodeRecurse,top,top->value,0,out);

      for(AccelInfoIterator it = iter; it.IsValid(); it = it.Next()){
        InstanceInfo* unit = it.CurrentUnit();

        if(unit->memMapped.has_value()){
          AccelInfoIterator inside = it.StepInsideOnly();
          Recurse(Recurse,inside,unit->memMapped.value(),out);
        }
      }
    }
  };

  CalculateMemory(CalculateMemory,initialIter,0,out);
  
  // Delay pos is just basically State position without anything different, right?
  auto CalculateDelayPos = [](auto Recurse,AccelInfoIterator& iter,int startIndex) -> void{
    TEMP_REGION(temp,nullptr);

    InstanceInfo* parent = iter.GetParentUnit();
    if(parent){
      AccelInfoIterator delayIter = StartIteration(&parent->decl->info);
      delayIter.mergeIndex = parent->partitionIndex;
      
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
            Recurse(Recurse,inside,delayPos);
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
          Recurse(Recurse,inside,delayIndex);
        }

        delayIndex += unit->delaySize;
      }
    }
  };
  
  CalculateDelayPos(CalculateDelayPos,initialIter,0);

  auto SetDelays = [](auto Recurse,AccelInfoIterator& iter,SimpleCalculateDelayResult& calculatedDelay,int startIndex,Arena* out) -> void{
    InstanceInfo* parent = iter.GetParentUnit();
    if(parent){
      // TODO: Delays are weird.
      AccelInfoIterator delayIter = StartIteration(&parent->decl->info);
      delayIter.mergeIndex = parent->partitionIndex;
      
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
        unit->portDelay = PushArray<int>(out,calculatedDelay.inputPortDelayByOrder[order].size);

        for(int i = 0; i < calculatedDelay.inputPortDelayByOrder[order].size; i++){
          unit->portDelay[i] = calculatedDelay.inputPortDelayByOrder[order][i].value;
        }
        
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

  SimpleCalculateDelayResult delays = CalculateDelay(initialIter,out);
  SetDelays(SetDelays,initialIter,delays,0,out);
}

void FillAccelInfoAfterCalculatingInstanceInfo(AccelInfo* info,Accelerator* accel){
  TEMP_REGION(temp,nullptr);
  
  GrowableArray<bool> seenShared = StartGrowableArray<bool>(temp);
  
  int memoryMappedDWords = 0;
  int unitsMapped = 0;

  // TODO: Use AccelInfoIterator

  // TODO: Using info directly might make create an error if the struct was not initialized to zero.
  
  // Handle non-static information
  for(FUInstance* ptr : accel->allocated){
    FUInstance* inst = ptr;
    FUDeclaration* type = inst->declaration;

    // Check if shared
    if(inst->sharedEnable){
      if(!seenShared[inst->sharedIndex]){
        info->configs += type->configs.size;
        info->sharedUnits += 1;
      }

      seenShared[inst->sharedIndex] = true;
    } else if(!inst->isStatic){ // Shared cannot be static
         info->configs += type->configs.size;
    }

    if(type->memoryMapBits.has_value()){
      info->isMemoryMapped = true;

      memoryMappedDWords = AlignBitBoundary(memoryMappedDWords,type->memoryMapBits.value());
      memoryMappedDWords += (1 << type->memoryMapBits.value());

      unitsMapped += 1;
    }

    if(type == BasicDeclaration::input){
      info->inputs += 1;
    }

    info->states += type->states.size;
    info->delays += type->NumberDelays();
    info->ios += type->nIOs;

    if(type->externalMemory.size){
      info->externalMemoryInterfaces += type->externalMemory.size;
    }

    info->signalLoop |= type->signalLoop;
  }

  info->memoryMappedBits = log2i(memoryMappedDWords);
  info->unitsMapped = unitsMapped;
  
  FUInstance* outputInstance = GetOutputInstance(&accel->allocated);
  if(outputInstance){
    EdgeIterator iter = IterateEdges(accel);
    while(iter.HasNext()){
      Edge edgeInst = iter.Next();
      Edge* edge = &edgeInst;

      if(edge->units[0].inst == outputInstance){
        info->outputs = std::max(info->outputs - 1,edge->units[0].port) + 1;
      }
      if(edge->units[1].inst == outputInstance){
        info->outputs = std::max(info->outputs - 1,edge->units[1].port) + 1;
      }
    }
  }

    TrieMap<StaticId,int>* staticSeen = PushTrieMap<StaticId,int>(temp);
  
  // TODO: I am not sure if static info is needed by all the accelerators
  //       or if it is only needed by the top accelerator.
  //       For now we calculate it to make easier to debug by inspecting the data.
  //       But need to take another look eventually
  for(FUInstance* ptr : accel->allocated){
    FUInstance* inst = ptr;
    if(inst->isStatic){
      StaticId id = {};

      id.name = inst->name;

      staticSeen->Insert(id,1);
      info->statics += inst->declaration->configs.size;

      for(Wire& wire : inst->declaration->configs){
        info->staticBits += wire.bitSize;
      }
    }

    if(IsTypeHierarchical(inst->declaration)){
      for(Pair<StaticId,StaticData*> pair : inst->declaration->staticUnits){
        int* possibleFind = staticSeen->Get(pair.first);

        if(!possibleFind){
          staticSeen->Insert(pair.first,1);
          info->statics += pair.second->configs.size;

          for(Wire& wire : pair.second->configs){
            info->staticBits += wire.bitSize;
          }
        }
      }
    }
  }
  
  for(FUInstance* ptr : accel->allocated){
    info->numberConnections += Size(ptr->allOutputs);
  }
}

Array<int> ExtractInputDelays(AccelInfoIterator top,Arena* out){
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

Array<int> ExtractOutputLatencies(AccelInfoIterator top,Arena* out){
  for(AccelInfoIterator iter = top; iter.IsValid(); iter = iter.Next()){
    InstanceInfo* info = iter.CurrentUnit();
    if(info->decl == BasicDeclaration::output){
      return CopyArray(info->portDelay,out);
    }
  }

  return {};
};

AccelInfo CalculateAcceleratorInfo(Accelerator* accel,bool recursive,Arena* out){
  TEMP_REGION(temp,out);
  AccelInfo result = {};

  // TODO: Still need to check if partitions are removed or if we still need them
  Array<Partition> partitions = GenerateInitialPartitions(accel,temp);
  int partitionSize = 0;
  while(1){
    partitionSize += 1;
    
    if(!Next(partitions)){
      break;
    }
  }

  // Accel might contain multiple merged units.
  // Each merged unit is responsible for adding "partitions"
  // Each "partition" is equivalent to "activating" a given merge type for a given merge unit.
  // For each partition, I should be able to get the name of all the units currently activated.
  // I do not think that I will keep partitions, but for now implement the logic first and worry about code quality later.
  
  int size = partitionSize;

  
  auto CalculateMuxConfigs = [](AccelInfoIterator top,Arena* out) -> Array<int>{
    TEMP_REGION(temp,out);
    
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

  if(size == 0){
    result.infos = PushArray<MergePartition>(globalPermanent,1);
  } else {
    result.infos = PushArray<MergePartition>(globalPermanent,size);
  }

  AccelInfoIterator iter = StartIteration(&result);
  iter.accelName = accel->name;
  
  // MARKED
  if(iter.MergeSize() > 1){
    Array<Partition> partitions = GenerateInitialPartitions(accel,temp);

    for(int i = 0; i < iter.MergeSize(); i++,Next(partitions)){
      iter.mergeIndex = i;

      // We do need partitions here, I think
      result.infos[i].info = GenerateInitialInstanceInfo(accel,globalPermanent,partitions);

      FillInstanceInfo(iter,globalPermanent);

      result.infos[i].inputDelays = ExtractInputDelays(iter,out);
      result.infos[i].outputLatencies = ExtractOutputLatencies(iter,out);
      result.infos[i].muxConfigs = CalculateMuxConfigs(iter,out);

      String name = GetName(partitions,out);
      result.infos[i].name = name;
    }
  } else {
    iter.mergeIndex = 0;
    result.infos[0].info = GenerateInitialInstanceInfo(accel,globalPermanent,{});
    FillInstanceInfo(iter,globalPermanent);

    result.infos[0].inputDelays = ExtractInputDelays(iter,out);
    result.infos[0].outputLatencies = ExtractOutputLatencies(iter,out);
    result.infos[0].muxConfigs = CalculateMuxConfigs(iter,out);
  }

  FillAccelInfoAfterCalculatingInstanceInfo(&result,accel);

  return result;
}

#include "configurations.hpp"

#include "accelerator.hpp"
#include "debug.hpp"
#include "declaration.hpp"
#include "globals.hpp"
#include "memory.hpp"
#include "parser.hpp"
#include "utils.hpp"
#include "utilsCore.hpp"
#include "verilogParsing.hpp"
#include "versat.hpp"
#include "textualRepresentation.hpp"
#include <strings.h>
#include "symbolic.hpp"

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
  bool res = (info && info->infos.size > 0 && index >= 0 && index < GetCurrentMerge().size);
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

AccelInfoIterator AccelInfoIterator::ReverseStep(){
  if(!IsValid()){
    return {};
  }

  if(this->index - 1 < 0){
    return {};
  }
  
  AccelInfoIterator toReturn = *this;
  toReturn.index = this->index - 1;
  return toReturn;
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

  AccelInfoIterator iter = StartIteration(info,mergeIndex);

  for(; iter.IsValid(); iter = iter.Step()){
    InstanceInfo* unit = iter.CurrentUnit();

    if(unit->level == level){
      *builder.PushElem() = unit;
    }
  }

  Array<InstanceInfo*> res = EndArray(builder);
  return res;
}

AccelInfoIterator StartIteration(AccelInfo* info,int mergeIndex){
  AccelInfoIterator iter = {};
  iter.info = info;
  iter.SetMergeIndex(mergeIndex);
  return iter;
}

Array<String> ExtractStates(Array<InstanceInfo> info,Arena* out){
  int count = 0;
  for(InstanceInfo& in : info){
    if(!in.isComposite && in.statePos.has_value()){
      count += in.states.size;
    }
  }

  Array<String> res = PushArray<String>(out,count);
  int index = 0;
  for(InstanceInfo& in : info){
    if(!in.isComposite && in.statePos.has_value()){
      for(Wire& wire : in.states){
        res[index++] = PushString(out,"%.*s_%.*s",UN(in.fullName),UN(wire.name)); 
      }
    }
  }

  return res;
}

String GetEntityMemName(InstanceInfo* info,Arena* out){
  String name = PushString(out,"%.*s_addr",UN(info->fullName)); 
  return name;
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
      String name = GetEntityMemName(&in,out); 
      res[index++] = {name,(int) in.memMapped.value()};
    }
  }

  return res;
}

// nocheck: Remove this, we already have a function that does this.
String ReprStaticConfig(StaticId id,Wire* wire,Arena* out){
  String identifier = PushString(out,"%.*s_%.*s_%.*s",UN(id.parent->name),UN(id.name),UN(wire->name));

  return identifier;
}

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

Array<Partition> GenerateInitialPartitions(Accelerator* accel,Arena* out){
  auto partitionsArr = StartArray<Partition>(out);
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

int PartitionSize(Array<Partition> parts){
  int size = 1;
  for(Partition p : parts){
    size *= p.max;
  }
  return size;
}

// ======================================
// Partition Info iteration

struct PartitionInfoIterator{
  Array<Partition> partitions;
  bool isValid;

  bool IsValid();
  void Next();
  Array<MergePartition*> GetInfos(Arena* out);
};

PartitionInfoIterator* StartPartitionIteration(Accelerator* accel,Arena* out){
  PartitionInfoIterator* res = PushStruct<PartitionInfoIterator>(out);
  res->partitions = GenerateInitialPartitions(accel,out);
  res->isValid = true;

  return res;
}

bool PartitionInfoIterator::IsValid(){
  return isValid;
}

void PartitionInfoIterator::Next(){
  isValid = ::Next(partitions);
}

Array<MergePartition*> PartitionInfoIterator::GetInfos(Arena* out){
  Array<MergePartition*> res = PushArray<MergePartition*>(out,partitions.size);

  for(int i = 0; i <  partitions.size; i++){
    Partition part = partitions[i];

    res[i] = &part.decl->info.infos[part.value];
  }

  return res;
}

String GetName(Array<Partition> partitions,Arena* out){
  TEMP_REGION(temp,out);
  auto builder = StartString(temp);

  bool first = true;
  for(Partition p : partitions){
    if(first){
      first = false;
    } else {
      builder->PushString("_");
    }

    AccelInfo* info = &p.decl->info;
    builder->PushString(info->infos[p.value].name);
  }

  return EndString(out,builder);
}

// TODO: Move this function to a better place
// This function cannot return an array for merge units because we do not have the merged units info in this level.
// This function only works for modules and for recursing the merged info to upper modules.
// The info needed by merge must be stored by the merge function.
Array<InstanceInfo> GenerateInitialInstanceInfo(Accelerator* accel,Arena* out,Array<Partition> partitions,bool calculateOrder){
  TEMP_REGION(temp,out);

  FREE_ARENA(mapArena);
  auto instanceToIndex = PushTrieMap<FUInstance*,int>(mapArena);

  // Only for basic units on the top level of accel
  // The subunits are basically copied from the sub declarations.
  // Or they are calculated inside the Fill... function
  int index = 0;
  auto SetBaseInfo = [&index](InstanceInfo* elem,FUInstance* inst,int level,Arena* out){
    TEMP_REGION(temp,out);
    
    *elem = {};
    elem->inst = inst;
    elem->localIndex = index++;
    elem->name = inst->name;
    elem->baseName = inst->name; // Remember this function only sets basic units, so the baseName is just the name.
    elem->decl = inst->declaration;
    elem->typeName = inst->declaration->name;
    elem->level = level;
    elem->isComposite = IsTypeHierarchical(inst->declaration);
    elem->isMerge = inst->declaration->type == FUDeclarationType_MERGED;
    elem->isStatic = inst->isStatic;
    elem->debug = inst->debug;
    elem->isGloballyStatic = inst->isStatic;
    elem->isShared = inst->sharedEnable;
    elem->isSpecificConfigShared = inst->isSpecificConfigShared;
    elem->sharedIndex = inst->sharedIndex;
    elem->isMergeMultiplexer = inst->isMergeMultiplexer;
    elem->special = inst->literal;
    elem->muxGroup = inst->muxGroup;
    elem->id = inst->id;
    elem->inputDelays = inst->declaration->GetInputDelays();
    elem->outputLatencies = inst->declaration->GetOutputLatencies();
    elem->connectionType = inst->type;
    elem->partitionIndex = 0;
    elem->individualWiresShared = inst->isSpecificConfigShared;
    elem->numberDelays = inst->declaration->NumberDelays();
    elem->memMapBits = inst->declaration->info.memMapBits;

    elem->configs = CopyArray(inst->declaration->configs,out);
    elem->states = CopyArray(inst->declaration->states,out);
    elem->singleInterfaces = inst->declaration->singleInterfaces;
    elem->externalMemory = CopyArray(inst->declaration->externalMemory,out);
    elem->supportedAddressGen = inst->declaration->supportedAddressGen;

    elem->configSize = elem->configs.size;
    elem->stateSize = elem->states.size;
    elem->memMapBits = elem->decl->info.memMapBits;

    if(elem->memMapBits.has_value()){
      elem->memMappedSize = 1 << elem->memMapBits.value();
    }

    if(inst->declaration == BasicDeclaration::input){
      elem->specialType = SpecialUnitType_INPUT;
    }
    if(inst->declaration == BasicDeclaration::output){
      elem->specialType = SpecialUnitType_OUTPUT;
    }
    if(inst->declaration == BasicDeclaration::fixedBuffer){
      elem->specialType = SpecialUnitType_FIXED_BUFFER;
    }
    if(inst->declaration == BasicDeclaration::variableBuffer){
      elem->specialType = SpecialUnitType_VARIABLE_BUFFER;
    }
    if(inst->declaration->isOperation){
      elem->specialType = SpecialUnitType_OPERATION;
    }

    // MARK

    Hashmap<String,SymbolicExpression*>* paramsMap = GetParametersOfUnit(inst,out);
    Array<ParamAndValue> params = PushArray<ParamAndValue>(out,paramsMap->nodesUsed);

    int index = 0;
    for(Pair<String,SymbolicExpression**> p : paramsMap){
      params[index++] = (ParamAndValue) {.name = p.first,.val = *p.second};
    }
    elem->params = params;

    // NOTE: Care when using arrays, these must be copied individually for each subunit (check below)
    // TODO: I do not remember if number configs is correct at this stage or not.
    //       Check and write some comment here if so
    elem->individualWiresGlobalStaticPos = PushArray<int>(out,inst->declaration->NumberConfigs());
    elem->individualWiresGlobalConfigPos = PushArray<int>(out,inst->declaration->NumberConfigs());
    elem->individualWiresLocalConfigPos = PushArray<int>(out,inst->declaration->NumberConfigs());
  };
  
  // NOTE: This function fills all the subunits that belong to composite units.
  //       Care must be taken when having arrays inside InstanceInfos, these need to be copied individually otherwise we are changing data for the subunits declarations as well.
  auto Function = [SetBaseInfo,&instanceToIndex](auto Recurse,GrowableArray<InstanceInfo>& array,Accelerator* accel,int level,Array<Partition> partitions,Arena* out) -> void{
    int partitionIndex = 0;
    for(FUInstance* inst : accel->allocated){
      // MARK
      instanceToIndex->Insert(inst,array.size);

      InstanceInfo* elem = array.PushElem();
      SetBaseInfo(elem,inst,level,out);

      AccelInfoIterator iter = StartIteration(&inst->declaration->info);

      if(partitions.size > 0 && inst->declaration->info.infos.size > 1){
        iter.SetMergeIndex(partitions[partitionIndex].value);
        elem->outputLatencies = partitions[partitionIndex].decl->info.infos[iter.mergeIndex].outputLatencies;
        elem->partitionIndex = partitions[partitionIndex].value;
        partitionIndex += 1;
      }
      
      for(; iter.IsValid(); iter = iter.Step()){
        InstanceInfo* subUnit = iter.CurrentUnit();
        InstanceInfo* elem = array.PushElem();

        *elem = *subUnit;
        elem->individualWiresGlobalConfigPos = CopyArray(subUnit->individualWiresGlobalConfigPos,out);
        Memset(elem->individualWiresGlobalConfigPos,0);
        elem->level += 1;
      }
    }
  };
  
  auto build = StartArray<InstanceInfo>(out);
  Function(Function,build,accel,0,partitions,out);
  Array<InstanceInfo> res = EndArray(build);

  for(Pair<FUInstance*,int> p : instanceToIndex){
    InstanceInfo* info = &res[p.second];
    FUInstance* inst = p.first;

    {
      ArenaList<SimplePortConnection>* list = PushList<SimplePortConnection>(temp);
      FOREACH_LIST(ConnectionNode*,ptr,inst->allInputs){
        FUInstance* out = ptr->instConnectedTo.inst;
        int outPort = ptr->instConnectedTo.port;

        int outIndex = instanceToIndex->GetOrFail(out);

        SimplePortConnection* portInst = list->PushElem();
        portInst->otherInst = outIndex;
        portInst->otherPort = outPort;
        portInst->port = ptr->port;
      }
      info->inputs = PushArray(out,list);
    }

    {
      info->inputsDirectly = PushArray<SimplePortInstance>(out,inst->inputs.size);
      
      for(int i = 0; i < inst->inputs.size; i++){
        if(inst->inputs[i].inst){
          info->inputsDirectly[i].inst = instanceToIndex->GetOrFail(inst->inputs[i].inst);
          info->inputsDirectly[i].port = inst->inputs[i].port;
        } else {
          info->inputsDirectly[i].inst = - 1;
        }
      }
    }

    {
      ArenaList<SimplePortConnection>* list = PushList<SimplePortConnection>(temp);
      FOREACH_LIST(ConnectionNode*,ptr,inst->allOutputs){
        FUInstance* other = ptr->instConnectedTo.inst;
        int otherPort = ptr->instConnectedTo.port;

        int otherIndex = instanceToIndex->GetOrFail(other);

        SimplePortConnection* portInst = list->PushElem();
        portInst->otherInst = otherIndex;
        portInst->otherPort = otherPort;
        portInst->port = ptr->port;
      }

      info->outputs = PushArray(out,list);
    }
    
    info->outputIsConnected = CopyArray(inst->outputs,out);
  }

  if(calculateOrder){
    DAGOrderNodes order = CalculateDAGOrder(accel,temp);
    for(Pair<FUInstance*,int> p : instanceToIndex){
      InstanceInfo* info = &res[p.second];
      FUInstance* inst = p.first;

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

    if(parent){
      unit->parentTypeName = PushString(out,parent->typeName);
    } else {
      unit->fullName = unit->name;
      continue;
    }

    unit->fullName = PushString(out,"%.*s_%.*s",UN(parent->fullName),UN(unit->name));
  }

  for(AccelInfoIterator iter = initialIter; iter.IsValid(); iter = iter.Step()){
    InstanceInfo* parent = iter.GetParentUnit();
    if(parent){
      InstanceInfo* unit = iter.CurrentUnit();

      if(parent->isGloballyStatic){
        unit->isGloballyStatic = true;
      }
    }
  }
  

  // TODO: Move to a better place
  auto GetSharedIndex = [](int unitSharedIndex,int wireIndex){
    return ((unitSharedIndex & 0x0000ffff) << 16 | (wireIndex & 0x0000ffff)); 
  };

  auto CalculateAmountOfLevels = [](AccelInfoIterator initialIter){
    int maxLevelSeen = 0;
    for(AccelInfoIterator iter = initialIter; iter.IsValid(); iter = iter.Step()){
      InstanceInfo* info = iter.CurrentUnit();
      maxLevelSeen = MAX(maxLevelSeen,info->level);
    }
    return maxLevelSeen + 1;
  };

  auto CalculateAmountOfUnitsPerLevel = [CalculateAmountOfLevels](AccelInfoIterator initialIter,Arena* out) -> Array<int>{
    int amountOfLevels = CalculateAmountOfLevels(initialIter);

    Array<int> amountOfUnits = PushArray<int>(out,amountOfLevels);
    for(AccelInfoIterator iter = initialIter; iter.IsValid(); iter = iter.Step()){
      InstanceInfo* info = iter.CurrentUnit();
      amountOfUnits[info->level] += 1;
    }

    return amountOfUnits;
  };
  
  auto GetIteratorsForEachLevel = [CalculateAmountOfLevels,CalculateAmountOfUnitsPerLevel](AccelInfoIterator initialIter,Arena* out) -> Array<Array<AccelInfoIterator>>{
    TEMP_REGION(temp,out);

    auto amountOfUnitsPerLevel = CalculateAmountOfUnitsPerLevel(initialIter,temp);
    int amountOfLevels = amountOfUnitsPerLevel.size;
    
    Array<Array<AccelInfoIterator>> iterators = PushArray<Array<AccelInfoIterator>>(out,amountOfLevels);
    for(int level = 0; level < amountOfLevels; level++){
      iterators[level] = PushArray<AccelInfoIterator>(out,amountOfUnitsPerLevel[level]);
    }
    
    Array<int> indexPerLevel = PushArray<int>(temp,amountOfLevels);

    for(AccelInfoIterator iter = initialIter; iter.IsValid(); iter = iter.Step()){
      InstanceInfo* info = iter.CurrentUnit();
      int level = info->level;
      int currentIndex = indexPerLevel[level];
      
      iterators[level][currentIndex] = iter;
      
      indexPerLevel[level] += 1;
    }

    return iterators;
  };

  auto allIteratorsPerLevel = GetIteratorsForEachLevel(initialIter,temp);

  // Units that are instantiated already have their config position calculated previously.
  // Which is stored inside the declaration for the unit.
  // The only thing that changes the normal configuration process is the fact that some units might be static.
  // This only changes the calculation of the configurations on the top level, because the bottom level already had the static configurations taken of consideration.
  // That means that we do have to care about statics until the top level.

  // Now, the normal recursive approach passes a start index for the beginning of every global config calculation.
  // We do not have this index when using a non recursive approach.
  // If the calculation of the configs is correct, we could just have a first phase storing the global config pos inside each unit for starters.
  // Then do the bottom to up fix up.
  
  // Go from top to bottom to put global config pos.
  // Go from bottom to top to calculate true global config.
  // NOTE: Do we need to go bottom -> top? Could we just have an iterator use step and be done with it?

  // We start by calculating global config pos for each top level instance and the
  // local wire config pos as well.
  {
    TrieMap<int,int>* sharedWireIndexToConfig = PushTrieMap<int,int>(temp);
    TrieMap<int,int>* firstAllocatedWire = PushTrieMap<int,int>(temp);
      
    int globalConfigPos = 0;
    for(AccelInfoIterator iter : allIteratorsPerLevel[0]){
      InstanceInfo* unit = iter.CurrentUnit();

      if(unit->isStatic){
        for(int i = 0; i < unit->individualWiresLocalConfigPos.size; i++){
          unit->individualWiresLocalConfigPos[i] = -1;
          unit->individualWiresGlobalConfigPos[i] = -1;
        }

        unit->globalConfigPos = {};
        continue;
      }

      for(int i = 0; i < unit->individualWiresLocalConfigPos.size; i++){
        int indexForCurrentWire = globalConfigPos;
        if(unit->individualWiresShared[i]){
          int sharedIndex = GetSharedIndex(unit->sharedIndex,i);
          GetOrAllocateResult<int> data = sharedWireIndexToConfig->GetOrAllocate(sharedIndex);

          if(data.alreadyExisted){
            indexForCurrentWire = *data.data;
          } else {
            *data.data = globalConfigPos;
            globalConfigPos += 1;
          }
        } else {
          globalConfigPos += 1;
        }

        unit->individualWiresLocalConfigPos[i] = indexForCurrentWire;

        // The first wire value is also the first local config position for the unit.
        // Except if we share configs, in which case the local config position is equal to the first shared unit local config pos.
        // This way, we can generate structs individually but with padding internally to line up the data correctly.
        if(i == 0){
          if(unit->isShared){
            GetOrAllocateResult<int> data = firstAllocatedWire->GetOrAllocate(unit->sharedIndex);
            if(!data.alreadyExisted){
              *data.data = indexForCurrentWire;
            }

            unit->localConfigPos = *data.data;
          } else {
            unit->localConfigPos = indexForCurrentWire;
          }
        }
      
        unit->individualWiresGlobalConfigPos[i] = indexForCurrentWire;
      }

      // Store global pos already because for top level localPos == globalPos
      unit->globalConfigPos = unit->localConfigPos;
    }
  }
  
  // Afterwards, we propagate the information downwards by iterating per level in order to calculate the global pos
  for(int level = 0; level < allIteratorsPerLevel.size; level++){
    for(AccelInfoIterator iter : allIteratorsPerLevel[level]){
      InstanceInfo* parent = iter.CurrentUnit();

      if(!parent->globalConfigPos.has_value()){
        continue;
      }
      
      int parentGlobalPos = parent->globalConfigPos.value();

      AccelInfoIterator configIter = StartIteration(&parent->decl->info,parent->partitionIndex);

      for(AccelInfoIterator it = iter.StepInsideOnly(); it.IsValid(); it = it.Next(),configIter = configIter.Next()){
        InstanceInfo* unit = it.CurrentUnit();
        InstanceInfo* configUnit = configIter.CurrentUnit();

        for(int i = 0; i < unit->individualWiresLocalConfigPos.size; i++){
          if(configUnit->individualWiresLocalConfigPos[i] != -1){
            unit->individualWiresGlobalConfigPos[i] = parentGlobalPos + configUnit->individualWiresLocalConfigPos[i];
          } else {
            unit->individualWiresGlobalConfigPos[i] = -1;
          }
        }

        if(configUnit->localConfigPos.has_value()){
          unit->globalConfigPos = parentGlobalPos + configUnit->localConfigPos.value();
        }
      }
    }
  }

  // TODO: Because of the approach taken for the configs, we might be able to write the following functions easier, by using the accelerator iterators we have previously calculated.
  //       There is not much point doing this however unless we feel like there is a need for it. The code works fine and I do not think we are even simplifying stuff. Both approaches seem equal in terms of complexity.
  
  // Calculate state stuff
  auto CalculateState = [](auto Recurse,AccelInfoIterator& iter,int startIndex) -> void{
    TEMP_REGION(temp,nullptr);

    InstanceInfo* parent = iter.GetParentUnit();
    if(parent){
      AccelInfoIterator stateIter = StartIteration(&parent->decl->info,parent->partitionIndex);
      
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
      AccelInfoIterator parentIter = StartIteration(&parent->decl->info,parent->partitionIndex);

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
      auto builder = StartArray<Node*>(temp);
      for(AccelInfoIterator it = iter; it.IsValid(); it = it.Next()){
        InstanceInfo* unit = it.CurrentUnit();

        if(unit->memMapBits.has_value()){
          Node* n = PushStruct<Node>(temp);
          *n = (Node){unit,unit->memMapBits.value()};
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
        n->value = MAX(left->value,right->value) + 1;
        n->left = left;
        n->right = right;
        baseNodes[baseNodes.size - 2] = n;
        baseNodes.size -= 1;

        Sort(baseNodes);
      }
    
      Node* top = baseNodes[0];
      
      auto NodeRecurse = [](auto Recurse,Node* top,int max,int bitAccum,Arena* out) -> void{
        if(top->left == nullptr){
          TEMP_REGION(temp,out);
          Assert(top->right == nullptr);

          InstanceInfo* info = top->unit;
          
          auto builder = StartString(temp);
          for(int i = max - 1; i >= top->value; i--){
            builder->PushChar(GET_BIT(bitAccum,i) ? '1' : '0');
          }
          String decisionMask = EndString(out,builder);
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
      AccelInfoIterator delayIter = StartIteration(&parent->decl->info,parent->partitionIndex);
      
      int index = 0;
      for(AccelInfoIterator it = iter; it.IsValid(); it = it.Next(),delayIter = delayIter.Next()){
        InstanceInfo* unit = it.CurrentUnit();
        InstanceInfo* delayUnit = delayIter.CurrentUnit();

        if(delayUnit->delayPos.has_value()){
          int delayPos = startIndex + delayUnit->delayPos.value();
        
          if(unit->numberDelays){
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

        if(unit->numberDelays){
          unit->delayPos = delayIndex;
        }
        
        AccelInfoIterator inside = it.StepInsideOnly();
        if(inside.IsValid()){
          Recurse(Recurse,inside,delayIndex);
        }

        delayIndex += unit->numberDelays;
      }
    }
  };
  
  CalculateDelayPos(CalculateDelayPos,initialIter,0);

  auto SetDelays = [](auto Recurse,AccelInfoIterator& iter,SimpleCalculateDelayResult& calculatedDelay,int startIndex,Arena* out) -> void{
    InstanceInfo* parent = iter.GetParentUnit();
    if(parent){
      // TODO: Delays are weird.
      AccelInfoIterator delayIter = StartIteration(&parent->decl->info,parent->partitionIndex);
      
      int index = 0;
      for(AccelInfoIterator it = iter; it.IsValid(); it = it.Next(),delayIter = delayIter.Next()){
        InstanceInfo* unit = it.CurrentUnit();
        InstanceInfo* delayUnit = delayIter.CurrentUnit();

        unit->variableBufferDelay = delayUnit->variableBufferDelay;

        int delayPos = startIndex + delayUnit->baseNodeDelay;
        unit->baseNodeDelay = delayPos;

        if(unit->numberDelays > 0 && !unit->isComposite){
          unit->extraDelay = PushArray<int>(out,unit->numberDelays);
          Memset(unit->extraDelay,unit->baseNodeDelay);
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

        unit->baseNodeDelay = calculatedDelay.nodeBaseLatencyByOrder[order].value;
        unit->portDelay = PushArray<int>(out,calculatedDelay.inputPortBaseLatencyByOrder[order].size);

        for(int i = 0; i < calculatedDelay.inputPortBaseLatencyByOrder[order].size; i++){
          unit->portDelay[i] = calculatedDelay.inputPortBaseLatencyByOrder[order][i].value;
        }
        
        if(unit->numberDelays > 0 && !unit->isComposite){
          unit->extraDelay = PushArray<int>(out,unit->numberDelays);
          Memset(unit->extraDelay,unit->baseNodeDelay);
        }

        AccelInfoIterator inside = it.StepInsideOnly();
        if(inside.IsValid()){
          Recurse(Recurse,inside,calculatedDelay,unit->baseNodeDelay,out);
        }
      }
    }
  };

  SimpleCalculateDelayResult delays = CalculateDelay(initialIter,out);
  SetDelays(SetDelays,initialIter,delays,0,out);
}

void FillStaticInfo(AccelInfo* info){
  TEMP_REGION(temp,nullptr);
  AccelInfoIterator iter = StartIteration(info);
  for(int i = 0; i < iter.MergeSize(); i++){
    AccelInfoIterator it = iter;
    it.SetMergeIndex(i);

    TrieMap<StaticId,int>* statics = PushTrieMap<StaticId,int>(temp);

    int currentStaticIndex = info->configs;
    for(; it.IsValid(); it = it.Step()){
      InstanceInfo* unit = it.CurrentUnit();
      InstanceInfo* parent = it.GetParentUnit(); 

      if(unit->configSize == 0){
        continue;
      }
        
      bool firstStaticSeen = parent ? unit->isStatic && !parent->isStatic : unit->isStatic;

      if(firstStaticSeen || (unit->isStatic && !unit->localConfigPos.has_value())){
        StaticId id = {};
        id.parent = parent ? GetTypeByName(unit->parentTypeName) : nullptr;
        id.name = unit->name;

        GetOrAllocateResult<int> result = statics->GetOrAllocate(id);
        int configPos = 0;
        if(result.alreadyExisted){
          configPos =  *result.data;
        } else {
          configPos = currentStaticIndex;
          currentStaticIndex += unit->configSize;
        }
          
        *result.data = configPos;
        unit->globalStaticPos = configPos;
      } else if(unit->isGloballyStatic && parent->globalStaticPos.has_value() && unit->localConfigPos.has_value()){
        int trueConfigPos = parent->globalStaticPos.value() + unit->localConfigPos.value();
        unit->globalStaticPos = trueConfigPos;
      }

      // NOTE: Local config is technically based on the parent config value.
      //       
      if(parent && parent->globalStaticPos.has_value()){
        int staticPos = parent->globalStaticPos.value();
        for(int i = 0; i < unit->individualWiresLocalConfigPos.size; i++){
          unit->individualWiresGlobalStaticPos[i] = staticPos + unit->individualWiresLocalConfigPos[i];
        }
      } else if(unit->globalStaticPos.has_value()){
        int staticPos = unit->globalStaticPos.value();
        for(int i = 0; i < unit->individualWiresLocalConfigPos.size; i++){
          unit->individualWiresGlobalStaticPos[i] = staticPos + i;
        }
      }
    }
  }

  TrieSet<Wire>* uniqueWires = PushTrieSet<Wire>(temp);
  
  for(auto iter = StartIteration(info); iter.IsValid(); iter = iter.Step()){
    InstanceInfo* info = iter.CurrentUnit();
    if(info->isStatic){
      for(Wire w : info->configs){
        w.name = GetStaticWireFullName(info,w,globalPermanent);
        uniqueWires->Insert(w);
      }
    }
  }

  info->allStaticWires = PushArray(globalPermanent,uniqueWires);
}

void FillAccelInfoFromCalculatedInstanceInfo(AccelInfo* info,Accelerator* accel){
  TEMP_REGION(temp,nullptr);
  
  GrowableArray<bool> seenShared = StartArray<bool>(temp);

  // TODO: Use AccelInfoIterator

  // TODO: Using info directly might make create an error if the struct was not initialized to zero.

  TrieSet<int>* configsSeen = PushTrieSet<int>(temp);

  AccelInfoIterator iter = StartIteration(info);
  for(;iter.IsValid(); iter = iter.Next()){
    InstanceInfo* info = iter.CurrentUnit();
    for(int i : info->individualWiresGlobalConfigPos){
      if(i >= 0){
        configsSeen->Insert(i);
      }
    }
  }

  info->configs = configsSeen->map->inserted;

  // Handle non-static information
  int memoryMappedDWords = 0;
  int unitsMapped = 0;
  int nDones = 0;
  for(FUInstance* ptr : accel->allocated){
    FUInstance* inst = ptr;
    FUDeclaration* type = inst->declaration;

    // Check if shared
    if(inst->sharedEnable){
      if(!seenShared[inst->sharedIndex]){
        info->sharedUnits += 1;
      }

      seenShared[inst->sharedIndex] = true;
    }
    
    if(type->info.memMapBits.has_value()){
      info->isMemoryMapped = true;

      memoryMappedDWords = AlignBitBoundary(memoryMappedDWords,type->info.memMapBits.value());
      memoryMappedDWords += (1 << type->info.memMapBits.value());

      unitsMapped += 1;
    }

    if(type == BasicDeclaration::input){
      info->inputs += 1;
    }

    info->states += type->states.size;
    info->delays += type->NumberDelays();
    info->nIOs += type->info.nIOs;

    if(type->externalMemory.size){
      info->externalMemoryInterfaces += type->externalMemory.size;
    }

    if(type->singleInterfaces & SingleInterfaces_SIGNAL_LOOP){
      info->signalLoop |= true;
    }
    if(ptr->declaration->singleInterfaces & SingleInterfaces_DONE){
      nDones += 1;
    }
  }

  if(unitsMapped){
    info->memMapBits = log2i(memoryMappedDWords);
  }
  info->unitsMapped = unitsMapped;
  info->nDones = nDones;
  
  FUInstance* outputInstance = GetOutputInstance(&accel->allocated);
  if(outputInstance){
    for(EdgeIterator iter = IterateEdges(accel); iter.IsValid(); iter.Next()){
      Edge edge = iter.Value();

      if(edge.units[0].inst == outputInstance){
        info->outputs = MAX(info->outputs - 1,edge.units[0].port) + 1;
      }
      if(edge.units[1].inst == outputInstance){
        info->outputs = MAX(info->outputs - 1,edge.units[1].port) + 1;
      }
    }
  }

  TrieMap<StaticId,int>* staticSeen = PushTrieMap<StaticId,int>(temp);
  
  // TODO: I am not sure if static info is needed by all the accelerators
  //       or if it is only needed by the top accelerator.
  //       For now we calculate it to make easier to debug by inspecting the data.
  //       But need to take another look eventually
  SymbolicExpression* staticExpr = PushLiteral(temp,0);
  for(FUInstance* ptr : accel->allocated){
    FUInstance* inst = ptr;
    if(inst->isStatic){
      StaticId id = {};

      id.name = inst->name;

      staticSeen->Insert(id,1);
      info->statics += inst->declaration->configs.size;

      for(Wire& wire : inst->declaration->configs){
        info->staticBits += wire.bitSize;

        staticExpr = Normalize(SymbolicAdd(staticExpr,wire.sizeExpr,temp),temp);
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

            staticExpr = Normalize(SymbolicAdd(staticExpr,wire.sizeExpr,temp),temp);
          }
        }
      }
    }
  }

  // TODO: HACK, VERY BAD
  info->staticExpr = PushRepr(globalPermanent,staticExpr);
  
  for(FUInstance* ptr : accel->allocated){
    info->numberConnections += Size(ptr->allOutputs);


    if(ptr->declaration->singleInterfaces & SingleInterfaces_DONE){
      info->implementsDone = true;
    }
  }
}

Array<int> ExtractInputDelays(AccelInfoIterator top,Arena* out){
  int maxInputs = -1;
  for(AccelInfoIterator iter = top; iter.IsValid(); iter = iter.Next()){
    InstanceInfo* info = iter.CurrentUnit();
    if(info->specialType == SpecialUnitType_INPUT){
      maxInputs = MAX(info->special,maxInputs);
    }
  }
  if(maxInputs == -1){
    return {};
  }

  Array<int> inputDelays = PushArray<int>(out,maxInputs + 1);
  for(AccelInfoIterator iter = top; iter.IsValid(); iter = iter.Next()){
    InstanceInfo* info = iter.CurrentUnit();
    if(info->specialType == SpecialUnitType_INPUT){
      inputDelays[info->special] = info->baseNodeDelay;
    }
  }

  return inputDelays;
};

Array<int> ExtractOutputLatencies(AccelInfoIterator top,Arena* out){
  for(AccelInfoIterator iter = top; iter.IsValid(); iter = iter.Next()){
    InstanceInfo* info = iter.CurrentUnit();
    if(info->specialType == SpecialUnitType_OUTPUT){
      return CopyArray(info->portDelay,out);
    }
  }

  return {};
};

AccelInfo CalculateAcceleratorInfo(Accelerator* accel,bool recursive,Arena* out,bool calculateOrder){
  TEMP_REGION(temp,out);
  AccelInfo result = {};

  // Accel might contain multiple merged units.
  // Each merged unit is responsible for adding a merge "partitions"
  // Each "partition" is equivalent to "activating" a given merge type for a given merge unit.
  
  // TODO: Still need to check if partitions are removed or if we still need them
  //       We can also embed partitions inside the AccelInfo.
  Array<Partition> partitions = GenerateInitialPartitions(accel,temp);
  int size = PartitionSize(partitions);

  Assert(size != 0);
  result.infos = PushArray<MergePartition>(out,size);

  AccelInfoIterator iter = StartIteration(&result);
  iter.accelName = accel->name;
  
  if(size > 1){
    for(int i = 0; i < iter.MergeSize(); i++,Next(partitions)){
      iter.SetMergeIndex(i);

      // We do need partitions here, I think
      result.infos[i].info = GenerateInitialInstanceInfo(accel,out,partitions,calculateOrder);

      FillInstanceInfo(iter,out);

      result.infos[i].inputDelays = ExtractInputDelays(iter,out);
      result.infos[i].outputLatencies = ExtractOutputLatencies(iter,out);
 
      result.infos[i].name = GetName(partitions,out);
    }
  } else {
    result.infos[0].info = GenerateInitialInstanceInfo(accel,out,{},calculateOrder);
    FillInstanceInfo(iter,out);

    result.infos[0].inputDelays = ExtractInputDelays(iter,out);
    result.infos[0].outputLatencies = ExtractOutputLatencies(iter,out);
  }

  FillAccelInfoFromCalculatedInstanceInfo(&result,accel);

  // User configs are declaration level.
  if(size > 1){
    PartitionInfoIterator* iter = StartPartitionIteration(accel,temp);

    for(int i = 0; i < size; i++,iter->Next()){
      auto list = PushList<ConfigFunction*>(temp);
    
      Array<MergePartition*> parts = iter->GetInfos(temp);

      for(MergePartition* part : parts){
        for(ConfigFunction* func : part->userFunctions){
          *list->PushElem() = func;
        }
      }

      result.infos[i].userFunctions = PushArray(out,list);
    }
  } else {
    auto list = PushList<ConfigFunction*>(temp);

    for(FUInstance* inst : accel->allocated){
      for(MergePartition part : inst->declaration->info.infos){
        for(ConfigFunction* func : part.userFunctions){
          *list->PushElem() = func;
        }
      }
    }
    
    result.infos[0].userFunctions = PushArray(out,list);
  }

  result.configWires = PushArray<Wire>(out,result.configs);
  result.stateWires = PushArray<Wire>(out,result.states);

  int configIndex = 0;
  for(AccelInfoIterator iter = StartIteration(&result); iter.IsValid(); iter = iter.Step()){
    InstanceInfo* unit = iter.CurrentUnit();

    Hashmap<String,SymbolicExpression*>* params = PushHashmap<String,SymbolicExpression*>(temp,unit->params.size);

    for(ParamAndValue p : unit->params){
      params->Insert(p.name,p.val);
    }
    
    if(unit->isGloballyStatic){
      continue;
    }

    for(int i = 0; i < unit->individualWiresGlobalConfigPos.size; i++){
      int globalPos = unit->individualWiresGlobalConfigPos[i];

      if(globalPos < 0){
        continue;
      }
      
      if(!Empty(result.configWires[globalPos].name)){
        continue;
      }

      Wire wire = unit->configs[i];
      result.configWires[configIndex] = wire;

      // TODO: We need to do this for state as well. And we probably want to make this more explicit.
      result.configWires[configIndex].sizeExpr = ReplaceVariables(wire.sizeExpr,params,out);
      result.configWires[configIndex++].name = PushString(out,"%.*s_%.*s",UN(unit->name),UN(wire.name));
    }
  }
  
  // TODO: This could be done based on the config offsets.
  //       Otherwise we are still basing code around static and shared logic when calculating offsets already does that for us.
  int stateIndex = 0;
  for(FUInstance* ptr : accel->allocated){
    FUInstance* inst = ptr;
    FUDeclaration* d = inst->declaration;

    for(Wire& wire : d->states){
      result.stateWires[stateIndex] = wire;

      result.stateWires[stateIndex].name = PushString(out,"%.*s_%.2d",UN(wire.name),stateIndex);
      stateIndex += 1;
    }
  }

  for(int i = 0; i < result.infos.size; i++){
    int id = 0;
    for(AccelInfoIterator iter = StartIteration(&result,i); iter.IsValid(); iter = iter.Step()){
      InstanceInfo* unit = iter.CurrentUnit();
      unit->id = id++;
    }
  }

  return result;
}

SymbolicExpression* GetParameterValue(InstanceInfo* info,String name){
  for(ParamAndValue val : info->params){
    if(val.name == name){
      return val.val;
    }
  }

  return nullptr;
}

bool IsUnitCombinatorialOperation(InstanceInfo* info){
  for(int delay : info->outputLatencies){
    if(delay != 0){
      return false;
    }
  }
  return info->specialType == SpecialUnitType_OPERATION;
}

String GetStaticFullName(InstanceInfo* info,Arena* out){
  String fullName = PushString(out,"%.*s_%.*s",UN(info->parentTypeName),UN(info->name));
  return fullName;
}

String GetStaticWireFullName(InstanceInfo* info,Wire wire,Arena* out){
  String fullName = PushString(out,"%.*s_%.*s_%.*s",UN(info->parentTypeName),UN(info->name),UN(wire.name));
  return fullName;
}

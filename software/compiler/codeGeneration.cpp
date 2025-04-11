#include "codeGeneration.hpp"

#include "accelerator.hpp"
#include "declaration.hpp"
#include "filesystem.hpp"
#include "logger.hpp"
#include "memory.hpp"
#include "utils.hpp"
#include "utilsCore.hpp"
#include "verilogParsing.hpp"
#include "versat.hpp"
#include "templateData.hpp"
#include "templateEngine.hpp"
#include "textualRepresentation.hpp"
#include "globals.hpp"
#include "configurations.hpp"
#include "addressGen.hpp"
#include "symbolic.hpp"
#include "emitter.hpp"

#include "versatSpecificationParser.hpp"

// TODO: REMOVE: Remove after proper implementation of AddressGenerators
Pool<AddressGenDef> addressGens;

Array<Difference> CalculateSmallestDifference(Array<int> oldValues,Array<int> newValues,Arena* out){
  Assert(oldValues.size == newValues.size); // For now

  int size = oldValues.size;

  auto arr = StartArray<Difference>(out);
  for(int i = 0; i < size; i++){
    int oldVal = oldValues[i];
    int newVal = newValues[i];

    if(oldVal != newVal){
      Difference* dif = arr.PushElem();

      dif->index = i;
      dif->newValue = newVal;
    }
  }

  Array<Difference> result = EndArray(arr);
  return result;
}

Array<FUDeclaration*> SortTypesByMemDependency(Array<FUDeclaration*> types,Arena* out){
  TEMP_REGION(temp,out);

  int size = types.size;

  int stored = 0;
  Array<FUDeclaration*> result = PushArray<FUDeclaration*>(out,size);

  Hashmap<FUDeclaration*,bool>* seen = PushHashmap<FUDeclaration*,bool>(temp,size);
  Array<Array<FUDeclaration*>> subTypes = PushArray<Array<FUDeclaration*>>(temp,size);
  Memset(subTypes,{});

  for(int i = 0; i < size; i++){
    subTypes[i] = MemSubTypes(&types[i]->info,temp);
    //subTypes[i] = MemSubTypes(types[i]->fixedDelayCircuit,temp); // TODO: We can reuse the SortTypesByConfigDependency function if we change it to receive the subTypes array from outside, since the rest of the code is equal.
    seen->Insert(types[i],false);
  }

  for(int iter = 0; iter < size; iter++){
    bool breakEarly = true;
    for(int i = 0; i < size; i++){
      FUDeclaration* type = types[i];
      Array<FUDeclaration*> sub = subTypes[i];

      bool* seenType = seen->Get(type);

      if(seenType && *seenType){
        continue;
      }

      bool allSeen = true;
      for(FUDeclaration* subIter : sub){
        bool* res = seen->Get(subIter);

        if(res && !*res){
          allSeen = false;
          break;
        }
      }

      if(allSeen){
        *seenType = true;
        result[stored++] = types[i];
        breakEarly = false;
      }
    }

    if(breakEarly){
      break;
    }
  }

  for(Pair<FUDeclaration*,bool*> p : seen){
    Assert(*p.second);
  }

  return result;
}

static Array<TypeStructInfoElement> GenerateAddressStructFromType(FUDeclaration* decl,Arena* out){
  if(decl->type == FUDeclarationType_SINGLE){
    int size = 1;
    Array<TypeStructInfoElement> entries = PushArray<TypeStructInfoElement>(out,size);

    for(int i = 0; i < size; i++){
      entries[i].typeAndNames = PushArray<SingleTypeStructElement>(out,1);
      entries[i].typeAndNames[0].type = STRING("void*");
      entries[i].typeAndNames[0].name = STRING("addr");
    }
    return entries;
  }

  int memoryMapped = 0;
  for(FUInstance* node : decl->fixedDelayCircuit->allocated){
    FUDeclaration* decl = node->declaration;

    if(!(decl->memoryMapBits.has_value())){
      continue;
    }

    memoryMapped += 1;
  }

  Array<TypeStructInfoElement> entries = PushArray<TypeStructInfoElement>(out,memoryMapped);

  int index = 0;
  for(FUInstance* node : decl->fixedDelayCircuit->allocated){
    FUDeclaration* decl = node->declaration;

    if(!(decl->memoryMapBits.has_value())){
      continue;
    }

    entries[index].typeAndNames = PushArray<SingleTypeStructElement>(out,1);
    entries[index].typeAndNames[0].type = PushString(out,"%.*sAddr",UNPACK_SS(decl->name));
    entries[index].typeAndNames[0].name = node->name;
    index += 1;
  }
  return entries;
}

static Array<TypeStructInfo> GetMemMappedStructInfo(AccelInfo* info,Arena* out){
  TEMP_REGION(temp,out);

  Array<FUDeclaration*> typesUsed = MemSubTypes(info,out);
  typesUsed = SortTypesByMemDependency(typesUsed,temp);

  Array<TypeStructInfo> structures = PushArray<TypeStructInfo>(out,typesUsed.size);
  int index = 0;
  for(auto& decl : typesUsed){
    Array<TypeStructInfoElement> val = GenerateAddressStructFromType(decl,out);
    structures[index].name = decl->name;
    structures[index].entries = val;
    index += 1;
  }

  return structures;
}

Array<String> ExtractMemoryMasks(AccelInfo info,Arena* out){
  auto builder = StartArray<String>(out);
  for(AccelInfoIterator iter = StartIteration(&info); iter.IsValid(); iter = iter.Next()){
    InstanceInfo* unit = iter.CurrentUnit();

    if(unit->memDecisionMask.has_value()){
      *builder.PushElem() = unit->memDecisionMask.value();
    } else if(unit->memMapped.has_value()){
      *builder.PushElem() = {}; // Empty mask (happens when only one unit with mem exists, bit weird but no need to change for now);
    }
  }

  return EndArray(builder);
}

Array<TypeStructInfoElement> ExtractStructuredConfigs(Array<InstanceInfo> info,Arena* out){
  TEMP_REGION(temp,out);

  Hashmap<int,ArenaList<String>*>* map = PushHashmap<int,ArenaList<String>*>(temp,9999);
  
  int maxConfig = 0;
  for(InstanceInfo& in : info){
    if(in.isComposite || !in.globalConfigPos.has_value() || in.isConfigStatic){
      continue;
    }
    
    for(int i = 0; i < in.configSize; i++){
      int config = in.individualWiresGlobalConfigPos[i];

      maxConfig = std::max(maxConfig,config);
      GetOrAllocateResult<ArenaList<String>*> res = map->GetOrAllocate(config);

      if(!res.alreadyExisted){
        *res.data = PushArenaList<String>(temp);
      }

      ArenaList<String>* list = *res.data;
      String name = PushString(out,"%.*s_%.*s",UNPACK_SS(in.fullName),UNPACK_SS(in.decl->configs[i].name));

      // Quick and dirty way of skipping same name
      bool skip = false;
      for(SingleLink<String>* ptr = list->head; ptr; ptr = ptr->next){
        if(CompareString(ptr->elem,name)){
          skip = true;
        }
      }
      // Cannot put same name since header file gives collision later on.
      if(skip){
        continue;
      }
      
      *list->PushElem() = name;
    }
  }
  
  int configSize = maxConfig + 1;
  ArenaList<TypeStructInfoElement>* elems = PushArenaList<TypeStructInfoElement>(temp);
  for(int i = 0; i < configSize; i++){
    ArenaList<String>** optList = map->Get(i);
    if(!optList){
      continue;
    }

    ArenaList<String>* list = *optList;
    int size = Size(list);

    Array<SingleTypeStructElement> arr = PushArray<SingleTypeStructElement>(out,size);
    int index = 0;
    for(SingleLink<String>* ptr = list->head; ptr; ptr = ptr->next){
      String elem = ptr->elem;
      arr[index].type = STRING("iptr");
      arr[index].name = elem;
      
      index += 1;
    }

    elems->PushElem()->typeAndNames = arr;
  }
  
  return PushArrayFromList(out,elems);
}

String GenerateVerilogParameterization(FUInstance* inst,Arena* out){
  TEMP_REGION(temp,out);
  FUDeclaration* decl = inst->declaration;
  Array<String> parameters = decl->parameters;
  int size = parameters.size;

  if(size == 0){
    return {};
  }
  
  auto mark = MarkArena(out);
  
  auto builder = StartString(temp);
  builder->PushString("#(");
  bool insertedOnce = false;
  for(int i = 0; i < size; i++){
    ParameterValue v = inst->parameterValues[i];
    String parameter = parameters[i];
    
    if(!v.val.size){
      continue;
    }

    if(insertedOnce){
      builder->PushString(",");
    }
    builder->PushString(".%.*s(%.*s)",UNPACK_SS(parameter),UNPACK_SS(v.val));
    insertedOnce = true;
  }
  builder->PushString(")");

  if(!insertedOnce){
    PopMark(mark);
    return {};
  }

  return EndString(out,builder);
}

void OutputCircuitSource(FUDeclaration* decl,FILE* file){
  TEMP_REGION(temp,nullptr);
  TEMP_REGION(temp2,temp);

  Accelerator* accel = decl->fixedDelayCircuit;
  
  ClearTemplateEngine(); // Make sure that we do not reuse data
  
  Assert(globalDebug.outputAccelerator); // Because FILE is created outside, code should not call this function if flag is set
  
  Pool<FUInstance> nodes = accel->allocated;
  for(FUInstance* node : nodes){
    if(node->declaration->nIOs){
      SetParameter(node,STRING("AXI_ADDR_W"),STRING("AXI_ADDR_W"));
      SetParameter(node,STRING("AXI_DATA_W"),STRING("AXI_DATA_W"));
      SetParameter(node,STRING("LEN_W"),STRING("LEN_W"));
    }
  }
  
  AccelInfo info = decl->info;
  Array<String> memoryMasks = ExtractMemoryMasks(info,temp);
  
  Array<String> parameters = PushArray<String>(temp,nodes.Size());
  for(int i = 0; i < nodes.Size(); i++){
    parameters[i] = GenerateVerilogParameterization(nodes.Get(i),temp);
  }

  Array<InstanceInfo*> topLevelUnits = GetAllSameLevelUnits(&info,0,0,temp);

  TemplateSetCustom("topLevel",MakeValue(&topLevelUnits));
  TemplateSetCustom("parameters",MakeValue(&parameters));
  TemplateSetNumber("unitsMapped",info.unitsMapped);
  TemplateSetNumber("numberConnections",info.numberConnections);

  TemplateSetCustom("inputDecl",MakeValue(BasicDeclaration::input));
  TemplateSetCustom("outputDecl",MakeValue(BasicDeclaration::output));
  TemplateSetCustom("arch",MakeValue(&globalOptions));
  TemplateSetCustom("accel",MakeValue(decl));
  TemplateSetCustom("memoryMasks",MakeValue(&memoryMasks));
  TemplateSetNumber("delaySize",DELAY_SIZE);
 
  TemplateSetCustom("instances",MakeValue(&nodes));

  Array<Wire> configs = decl->configs;
  
  Array<InstanceInfo*> allSameLevel = GetAllSameLevelUnits(&info,0,0,temp);

  auto builder = StartArray<Array<int>>(temp);

  for(InstanceInfo* p : allSameLevel){
    *builder.PushElem() = p->individualWiresGlobalConfigPos;
  }
  Array<Array<int>> wireIndexByInstanceGood = EndArray(builder);
  
  Array<Array<int>> wireIndexByInstance = Extract(info.infos[0].info,temp,&InstanceInfo::individualWiresGlobalConfigPos);

  TemplateSetCustom("configs",MakeValue(&configs));
  TemplateSetCustom("wireIndex",MakeValue(&wireIndexByInstanceGood));
  
  ProcessTemplate(file,BasicTemplates::acceleratorTemplate);
}

void OutputIterativeSource(FUDeclaration* decl,FILE* file){
  TEMP_REGION(temp,nullptr);
  TEMP_REGION(temp2,temp);
  // NOTE: Iteratives have not been properly maintained.
  //       Most of this code probably will not work

  Accelerator* accel = decl->fixedDelayCircuit;

  ClearTemplateEngine(); // Make sure that we do not reuse data

  Assert(globalDebug.outputAccelerator); // Because FILE is created outside, code should not call this function if flag is set

  for(FUInstance* ptr : accel->allocated){
    FUInstance* inst = ptr;
    if(inst->declaration->nIOs){
      SetParameter(inst,STRING("AXI_ADDR_W"),STRING("AXI_ADDR_W"));
      SetParameter(inst,STRING("AXI_DATA_W"),STRING("AXI_DATA_W"));
      SetParameter(inst,STRING("LEN_W"),STRING("LEN_W"));
    }
  }

  // TODO: If we get iterative working again, this should just get the AccelInfo from the decl.
  AccelInfo info = CalculateAcceleratorInfo(accel,true,temp);

  // TODO: If going back to iteratives, remove this. Only Top level should care about Versat Values.
  VersatComputedValues val = ComputeVersatValues(&info,false);

  Array<String> memoryMasks = ExtractMemoryMasks(info,temp);

  Hashmap<StaticId,StaticData>* staticUnits = CollectStaticUnits(&info,temp);
  
  Pool<FUInstance> nodes = accel->allocated;
  Array<String> parameters = PushArray<String>(temp,nodes.Size());
  for(int i = 0; i < nodes.Size(); i++){
    parameters[i] = GenerateVerilogParameterization(nodes.Get(i),temp);
  }
  
  Array<InstanceInfo*> topLevelUnits = GetAllSameLevelUnits(&info,0,0,temp);

  TemplateSetCustom("topLevel",MakeValue(&topLevelUnits));
  TemplateSetCustom("parameters",MakeValue(&parameters));
  TemplateSetCustom("staticUnits",MakeValue(staticUnits));
  TemplateSetCustom("accel",MakeValue(decl));
  TemplateSetCustom("memoryMasks",MakeValue(&memoryMasks));
  TemplateSetCustom("instances",MakeValue(&nodes));
  TemplateSetNumber("unitsMapped",val.unitsMapped);
  TemplateSetCustom("inputDecl",MakeValue(BasicDeclaration::input));
  TemplateSetCustom("outputDecl",MakeValue(BasicDeclaration::output));
  TemplateSetNumber("memoryAddressBits",val.memoryAddressBits);
    
  int lat = decl->lat;
  TemplateSetNumber("special",lat);

  ProcessTemplate(file,BasicTemplates::iterativeTemplate);
}

// TODO: Move all this stuff to a better place
#include <filesystem>
namespace fs = std::filesystem;

String GetRelativePathFromSourceToTarget(String sourcePath,String targetPath,Arena* out){
  TEMP_REGION(temp,out);
  fs::path source(StaticFormat("%.*s",UNPACK_SS(sourcePath)));
  fs::path target(StaticFormat("%.*s",UNPACK_SS(targetPath)));

  fs::path res = fs::relative(target,source);

  return PushString(out,"%s",res.c_str());
}

// TODO: Move to a better place
bool ContainsPartialShare(InstanceInfo* info){
  bool seenFalse = false;
  bool seenTrue = false;
  for(bool b : info->individualWiresShared){
    if(b){
      seenTrue = true;
    }
    if(!b){
      seenFalse = true;
    }
  }

  return (seenTrue && seenFalse);
}

// TODO: Do not know if we need to merge the config and state flows.
//       But something should be done about the integer hack below.
//       Feels kinda weird. Some changes in the data structures might simplify the code somewhat.
StructInfo* GenerateStateStruct(AccelInfoIterator iter,Arena* out){
  TEMP_REGION(temp,out);

  StructInfo* res = PushStruct<StructInfo>(out);

  InstanceInfo* parent = iter.GetParentUnit();
  if(parent){
    res->name = parent->decl->name;
  }

  // TODO: HACK
  static StructInfo integer = {};
  integer.name = STRING("iptr");
  
  auto list = PushArenaDoubleList<StructElement>(out);
  for(AccelInfoIterator it = iter; it.IsValid(); it = it.Next()){
    InstanceInfo* unit = it.CurrentUnit();
    StructElement elem = {};

    if(!unit->statePos.has_value()){
      continue;
    }

    int parentGlobalStatePos = 0;
    if(parent){
      parentGlobalStatePos = parent->statePos.value();
    }
    
    elem.name = unit->baseName;

    if(unit->isMerge){
      auto GenerateMergeStruct = [](InstanceInfo* topUnit,AccelInfoIterator iter,Arena* out)->StructInfo*{
        auto list = PushArenaDoubleList<StructElement>(out);
        StructInfo* res = PushStruct<StructInfo>(out);
        
        StructElement elem = {};
        for(int i = 0; i < iter.MergeSize(); i++){
          iter.mergeIndex = i;
        
          StructInfo* subInfo = GenerateStateStruct(iter,out);

          // NOTE: Why config version does not do this?
          subInfo->name = iter.GetMergeName();
          
          elem.name = iter.GetMergeName();
          elem.childStruct = subInfo;
          elem.localPos = 0;
          elem.size = topUnit->stateSize;

          *list->PushElem() = elem;
        }
        
        res->name = topUnit->decl->name;
        res->memberList = list;
        
        return res;
      };

      AccelInfoIterator inside = it.StepInsideOnly();
      StructInfo* subInfo = GenerateMergeStruct(unit,inside,out);
      elem.childStruct = subInfo;
    } else if(unit->isComposite){
      AccelInfoIterator inside = it.StepInsideOnly();
      StructInfo* subInfo = GenerateStateStruct(inside,out);
      elem.childStruct = subInfo;
    } else {
      StructInfo* simpleSubInfo = PushStruct<StructInfo>(out);

      simpleSubInfo->name = unit->decl->name;
      
      ArenaDoubleList<StructElement>* elements = PushArenaDoubleList<StructElement>(out);
      int index = 0;
      Array<Wire> states = unit->decl->states;
      for(Wire w : states){
        StructElement* elem = elements->PushElem();
        elem->name = w.name;
        elem->localPos = index++;
        elem->isMergeMultiplexer = false;
        elem->size = 1;
        elem->childStruct = &integer;
      }
      simpleSubInfo->memberList = elements;
      
      elem.childStruct = simpleSubInfo;
    }

    // TODO: Kinda of an hack because we do not have local state pos the same way we have local config pos.
    elem.localPos = unit->statePos.value() - parentGlobalStatePos;
    elem.size = unit->stateSize;
    elem.doesNotBelong = unit->doesNotBelong;
    
    *list->PushElem() = elem;
  }
  
  res->memberList = list;
  
  return res;
}

StructInfo* GenerateConfigStructRecurse(AccelInfoIterator iter,TrieMap<StructInfo,StructInfo*>* generatedStructs,Arena* out){
  StructInfo res = {};//PushStruct<StructInfo>(out);

  InstanceInfo* parent = iter.GetParentUnit();
 
  if(parent && parent->isMerge){
    res.name = iter.GetMergeName();
  } else if(parent){
    res.name = parent->decl->name;
  }

  // TODO: HACK
  static StructInfo integer = {};
  integer.name = STRING("iptr");
  
  // This function returns one struct info.
  auto list = PushArenaDoubleList<StructElement>(out);
  for(AccelInfoIterator it = iter; it.IsValid(); it = it.Next()){
    InstanceInfo* unit = it.CurrentUnit();
    StructElement elem = {};
      
    if(!unit->globalConfigPos.has_value()){
      continue;
    }
    
    int globalPos = unit->globalConfigPos.value();
    int unitLocalPos = unit->localConfigPos.value();
    
    elem.name = unit->baseName;

    if(unit->isMerge){
      // Merge struct is different, we do not iterate members but instead iterate the base types.
      auto GenerateMergeStruct =[](InstanceInfo* topUnit,AccelInfoIterator iter,TrieMap<StructInfo,StructInfo*>* generatedStructs,Arena* out)->StructInfo*{
        auto list = PushArenaDoubleList<StructElement>(out);
        StructInfo res = {}; //PushStruct<StructInfo>(out);
        
        StructElement elem = {};
        for(int i = 0; i < iter.MergeSize(); i++){
          iter.mergeIndex = i;
        
          StructInfo* subInfo = GenerateConfigStructRecurse(iter,generatedStructs,out);

          elem.name = iter.GetMergeName();
          elem.childStruct = subInfo;
          elem.localPos = 0; // Zero because the merge struct is basically a wrapper. 
          elem.size = topUnit->configSize;
          elem.isMergeMultiplexer = topUnit->isMergeMultiplexer;

          *list->PushElem() = elem;
        }
        
        res.name = topUnit->decl->name;
        res.originalName = res.name;
        res.memberList = list;

        auto result = generatedStructs->GetOrAllocate(res);
        if(!result.alreadyExisted){
          StructInfo* allocated = PushStruct<StructInfo>(out);
          *allocated = res;
          
          *result.data = allocated;
        }
        
        return *result.data;
      };

      AccelInfoIterator inside = it.StepInsideOnly();

      StructInfo* subInfo = GenerateMergeStruct(unit,inside,generatedStructs,out);
      elem.childStruct = subInfo;
    } else if(unit->isComposite){
      AccelInfoIterator inside = it.StepInsideOnly();
      StructInfo* subInfo = GenerateConfigStructRecurse(inside,generatedStructs,out);
      elem.childStruct = subInfo;
    } else {
      // NOTE: We are generating the sub struct info directly here instead of recursing.
      Array<int> localPos = unit->individualWiresLocalConfigPos;
      
      if(ContainsPartialShare(unit)){
        StructInfo simpleSubInfo = {};
        
        Array<Wire> configs = unit->decl->configs;
        
        ArenaDoubleList<StructElement>* elements = PushArenaDoubleList<StructElement>(out);
        int index = 0;

        Array<int> localPos = unit->individualWiresLocalConfigPos;
        for(Wire w : configs){
          StructElement* elem = elements->PushElem();
          elem->name = w.name;
          elem->localPos = localPos[index++] - unitLocalPos;

          Assert(elem->localPos >= 0);
          
          elem->isMergeMultiplexer = false;
          elem->size = 1;
          elem->childStruct = &integer;
        }
        
        simpleSubInfo.name = unit->decl->name;
        simpleSubInfo.originalName = unit->decl->name;
        simpleSubInfo.memberList = elements;

        auto res = generatedStructs->GetOrAllocate(simpleSubInfo);
        if(!res.alreadyExisted){
          StructInfo* allocated = PushStruct<StructInfo>(out);
          *allocated = simpleSubInfo;
          
          *res.data = allocated;
        }

        elem.childStruct = *res.data;
      } else {
        // NOTE: This structure should be the same accross all the invocations of this function, since we are just generating the struct of a simple type without any struct wide modifications (like partial share or merge).

        // NOTE: As such, we can detect in here wether we are generating something that already exists or not. Instead of using the TrieMap approach later in the ExtractStructs.
        StructInfo simpleSubInfo = {};
        
        Array<Wire> configs = unit->decl->configs;
        
        ArenaDoubleList<StructElement>* elements = PushArenaDoubleList<StructElement>(out);
        int index = 0;
        for(Wire w : configs){
          StructElement* elem = elements->PushElem();
          elem->name = w.name;
          elem->localPos = localPos[index++] - unitLocalPos;

          Assert(elem->localPos >= 0);

          elem->isMergeMultiplexer = false;
          elem->size = 1;
          elem->childStruct = &integer;
        }
        
        simpleSubInfo.name = unit->decl->name;
        simpleSubInfo.originalName = unit->decl->name;
        simpleSubInfo.memberList = elements;

        auto res = generatedStructs->GetOrAllocate(simpleSubInfo);
        if(!res.alreadyExisted){
          StructInfo* allocated = PushStruct<StructInfo>(out);
          *allocated = simpleSubInfo;
          
          *res.data = allocated;
        }

        elem.childStruct = *res.data;
      }
    }

    int maxPos = 0;
    for(int i : unit->individualWiresLocalConfigPos){
      maxPos = MAX(maxPos,i);
    }

    elem.localPos = unitLocalPos;
    elem.size = maxPos - unitLocalPos + 1;

    Assert(elem.size > 0);

    elem.isMergeMultiplexer = unit->isMergeMultiplexer;
    elem.doesNotBelong = unit->doesNotBelong;

    unit->structInfo = elem.childStruct;
    
    *list->PushElem() = elem;
  }
      
  res.originalName = res.name;
  res.memberList = list;

  auto result = generatedStructs->GetOrAllocate(res);
  if(!result.alreadyExisted){
    StructInfo* allocated = PushStruct<StructInfo>(out);
    *allocated = res;
          
    *result.data = allocated;
  }
  
  return *result.data;
}

StructInfo* GenerateConfigStruct(AccelInfoIterator iter,Arena* out){
  TEMP_REGION(temp,out);

  // NOTE: It is important that same structs are "shared" because later we might change the name of the struct in order to avoid naming conflicts and by sharing the same struct every "user" of that struct gets updated at the same time.
  TrieMap<StructInfo,StructInfo*>* generatedStructs = PushTrieMap<StructInfo,StructInfo*>(temp);

  StructInfo* result = GenerateConfigStructRecurse(iter,generatedStructs,out);

  for(; iter.IsValid(); iter = iter.Step()){
    InstanceInfo* unit = iter.CurrentUnit();

    if(unit->structInfo){
      unit->structInfo = generatedStructs->GetOrFail(*unit->structInfo);
    }
  }
  
  return result;
}
  
size_t HashStructInfo(StructInfo* info){
  if(!info) { return 0;};
  return std::hash<StructInfo>()(*info);
}

Array<StructInfo*> ExtractStructs(StructInfo* structInfo,Arena* out){
  TEMP_REGION(temp,out);

  // NOTE: Because we share the structs that are equal, we could technically implement this as just a pointer triemap, insteado of hashing the entire struct. That is because we are sure that the same pointer is equivalent to same struct, but for now leave this as it is.
  TrieMap<StructInfo,StructInfo*>* info = PushTrieMap<StructInfo,StructInfo*>(temp);

  auto Recurse = [](auto Recurse,StructInfo* top,TrieMap<StructInfo,StructInfo*>* map) -> void {
    if(!top){
      return;
    }

    for(DoubleLink<StructElement>* ptr = top->memberList ? top->memberList->head : nullptr; ptr; ptr = ptr->next){
      Recurse(Recurse,ptr->elem.childStruct,map);
    }
    // TODO: HACK
    if(top->name == STRING("iptr")){
      return;
    }
    
    map->InsertIfNotExist(*top,top);
  };

  Recurse(Recurse,structInfo,info);
  
  Array<StructInfo*> res = PushArrayFromTrieMapData(out,info);

  return res;
}

// typeString - Config or State
Array<TypeStructInfo> GenerateStructs(Array<StructInfo*> info,String typeString,bool useIptr /* TODO: HACK */,Arena* out){
  TEMP_REGION(temp,out);
  ArenaList<TypeStructInfo>* list = PushArenaList<TypeStructInfo>(temp);

  for(StructInfo* structInfo : info){
    TypeStructInfo* type = list->PushElem();
    type->name = structInfo->name;

    int maxPos = 0;
    for(DoubleLink<StructElement>* ptr = structInfo->memberList ? structInfo->memberList->head : nullptr; ptr; ptr = ptr->next){
      StructElement elem = ptr->elem;
      maxPos = MAX(maxPos,elem.localPos + elem.size);
    }
    
    Array<int> amountOfEntriesAtPos = PushArray<int>(temp,maxPos);
    for(DoubleLink<StructElement>* ptr = structInfo->memberList ? structInfo->memberList->head : nullptr; ptr; ptr = ptr->next){
      StructElement elem = ptr->elem;
      amountOfEntriesAtPos[elem.localPos] += 1;
    }

    Array<bool> validPositions = PushArray<bool>(temp,maxPos);
    Array<bool> startPositionsOnly = PushArray<bool>(temp,maxPos);
    
    for(DoubleLink<StructElement>* ptr = structInfo->memberList ? structInfo->memberList->head : nullptr; ptr; ptr = ptr->next){
      StructElement elem = ptr->elem;

      startPositionsOnly[elem.localPos] = true;
      for(int i = elem.localPos; i < elem.localPos + elem.size; i++){
        validPositions[i] = true;
      }
    }

    auto builder = StartArray<IndexInfo>(temp);
    for(int i = 0; i < maxPos; i++){
      if(!validPositions[i]){
        *builder.PushElem() = {true,i,0};
      } else if(startPositionsOnly[i]){
        *builder.PushElem() = {false,i,amountOfEntriesAtPos[i]};
      }
    }
    Array<IndexInfo> indexInfo = EndArray(builder);

    // TODO: This could just be an array.
    TrieMap<int,int>* positionToIndex = PushTrieMap<int,int>(temp);
    for(int i = 0; i < indexInfo.size; i++){
      IndexInfo info = indexInfo[i];
      if(!info.isPadding){
        positionToIndex->Insert(info.pos,i);
      }
    }
    
    type->entries = PushArray<TypeStructInfoElement>(out,indexInfo.size);

    for(int i = 0; i < indexInfo.size; i++){
      IndexInfo info = indexInfo[i];
      type->entries[i].typeAndNames = PushArray<SingleTypeStructElement>(out,info.amountOfEntries);
    }

    Array<int> indexes = PushArray<int>(temp,maxPos);
    
    int indexPos = 0;
    int paddingAdded = 0;
    for(DoubleLink<StructElement>* ptr = structInfo->memberList ? structInfo->memberList->head : nullptr; ptr;){
      StructElement elem = ptr->elem;
      int pos = elem.localPos;

      int index = positionToIndex->GetOrFail(pos);
      
      int subIndex = indexes[pos]++;
      type->entries[index].typeAndNames[subIndex].name = elem.name;

      if(elem.doesNotBelong){
        type->entries[index].typeAndNames[subIndex].name = PushString(out,"padding_%d",paddingAdded++);
      }
      
      // TODO: HACK
      if(elem.childStruct->name == STRING("iptr")){
        if(useIptr) {
          type->entries[index].typeAndNames[subIndex].type = STRING("iptr");
        } else {
          type->entries[index].typeAndNames[subIndex].type = STRING("int");
        }
      } else {
        type->entries[index].typeAndNames[subIndex].type = PushString(out,"%.*s%.*s",UNPACK_SS(elem.childStruct->name),UNPACK_SS(typeString));
      }
      type->entries[index].typeAndNames[subIndex].arraySize = 0;

      ptr = ptr->next;
    }

    for(int i = 0; i < indexInfo.size; i++){
      IndexInfo info = indexInfo[i];
      if(info.isPadding){
        int pos = info.pos;
        type->entries[pos].typeAndNames = PushArray<SingleTypeStructElement>(out,1);

        // TODO: HACK
        if(useIptr) {
          type->entries[pos].typeAndNames[0].type = STRING("iptr");
        } else {
          type->entries[pos].typeAndNames[0].type = STRING("int");
        }
        type->entries[pos].typeAndNames[0].name = PushString(out,"padding_%d",paddingAdded++);
      }
    }

    for(int i = 0; i < type->entries.size; i++){
      Assert(type->entries[i].typeAndNames.size > 0);
    }
  }
  
  Array<TypeStructInfo> res = PushArrayFromList(out,list);
  return res;
}

// NOTE: Does sharing the struct info cause problems here?
void PushMergeMultiplexersUpTheHierarchy(StructInfo* top){
  auto PushMergesUp = [](StructInfo* parent,StructElement* parentElem,StructInfo* child) -> void{
    if(child == nullptr || child->memberList == nullptr || child->memberList->head == nullptr){
      return;
    }

    PushMergeMultiplexersUpTheHierarchy(child);
    
    for(DoubleLink<StructElement>* childPtr = child->memberList->head; childPtr;){
      if(childPtr->elem.isMergeMultiplexer){
        DoubleLink<StructElement>* node = childPtr;

        int nodeActualPos = parentElem->localPos + childPtr->elem.localPos;
        
        childPtr = RemoveNodeFromList(child->memberList,node);

        // TODO: We should do a full comparison.
        //       For now only name because I already know muxes will have different names if different.
        bool sameName = false;
        for(DoubleLink<StructElement>* parentPtr = parent->memberList->head; parentPtr; parentPtr = parentPtr->next){
          sameName |= CompareString(parentPtr->elem.name,node->elem.name);
        }

        if(!sameName){
          node->elem.localPos = nodeActualPos;
          parent->memberList->PushNode(node);
        }
      } else {
        childPtr = childPtr->next;
      }
    }

  };
  
  for(DoubleLink<StructElement>* ptr = top->memberList->head; ptr;){
    PushMergesUp(top,&ptr->elem,ptr->elem.childStruct);

    if(ptr->elem.childStruct->memberList && Empty(ptr->elem.childStruct->memberList)){
      ptr = RemoveNodeFromList(top->memberList,ptr);
    } else {
      ptr = ptr->next;
    }
  }
}

String GenerateAddressGenCompilationFunction(AddressAccess access,String addressGenName,Arena* out){
  TEMP_REGION(temp,out);

  Array<String> inputVars = access.inputVariableNames;
  String varName = STRING("args");
  
  auto EmitStoreAddressGenIntoConfig = [varName](CEmitter* emitter,AddressReadParameters params) -> void{
    TEMP_REGION(temp,emitter->arena);
          
    String* view = (String*) &params;
    for(int i = 0; i <  META_AddressReadParameters_Members.size; i++){
      String str = META_AddressReadParameters_Members[i];

      String t = PushString(temp,"%.*s.%.*s",UNPACK_SS(varName),UNPACK_SS(str));
      emitter->Assignment(t,view[i]);
    }
  };

  auto EmitDoubleOrSingleLoopCode = [EmitStoreAddressGenIntoConfig](CEmitter* c,int loopIndex,AddressAccess access){
    TEMP_REGION(temp,c->arena);
    
    AddressAccess* doubleLoop = ConvertAccessTo2ExternalLoopsUsingNLoopAsBiggest(&access,loopIndex,temp);
    AddressAccess* singleLoop = ConvertAccessSingleLoop(&access,temp);

    region(temp){
      String repr = PushRepresentation(GetLoopLinearSumTotalSize(doubleLoop->external,temp),temp);
      c->VarDeclare(STRING("int"),STRING("doubleLoop"),repr);
    }

    region(temp){
      String repr2 = PushRepresentation(GetLoopLinearSumTotalSize(singleLoop->external,temp),temp);
      c->VarDeclare(STRING("int"),STRING("singleLoop"),repr2);
    }
      
    c->If(STRING("doubleLoop < singleLoop"));
    c->Comment(STRING("Dobule is smaller (better)"));
    region(temp){
      String r = PushString(temp,"printf(\"Dobule loop\\n\")"); 
      //c->Statement(r);
              
      ExternalMemoryAccess ext = CompileExternalMemoryAccess(doubleLoop->external,temp);
      Array<AddressGenLoopSpecificatonSym> internal = CompileAddressGenDef(doubleLoop->internal,temp);
              
      AddressReadParameters parameters = InstantiateAccess2(ext,internal,temp);
      EmitStoreAddressGenIntoConfig(c,parameters);
    }

    c->Else();
    c->Comment(STRING("Single is smaller (better)"));
    region(temp){
      String r = PushString(temp,"printf(\"Single loop\\n\")"); 
      //c->Statement(r);

      ExternalMemoryAccess ext = CompileExternalMemoryAccess(singleLoop->external,temp);
      Array<AddressGenLoopSpecificatonSym> internal = CompileAddressGenDef(singleLoop->internal,temp);

      String res = InstantiateAccess(ext,internal,temp);
      AddressReadParameters parameters = InstantiateAccess2(ext,internal,temp);

      EmitStoreAddressGenIntoConfig(c,parameters);
    }

    c->EndIf();
  };
  
  AddressAccess initial = access;
  
  auto Recurse = [EmitDoubleOrSingleLoopCode,EmitStoreAddressGenIntoConfig,&initial](auto Recurse,int loopIndex,CEmitter* c,Arena* out) -> void{
    TEMP_REGION(temp,out);

    LoopLinearSum* external = initial.external;
          
    int totalSize = external->terms.size;
    int leftOverSize = totalSize - loopIndex;

    // Last member must generate an 'else' instead of a 'else if'
    if(leftOverSize > 1){
      auto other = StartString(temp);
            
      SymbolicExpression* currentDecider = GetLoopHighestDecider(&external->terms[loopIndex]);
      bool first = true;
      for(int i = loopIndex + 1; i < totalSize; i++){
        SymbolicExpression* otherDecider = GetLoopHighestDecider(&external->terms[i]);

        if(first){
          first = false;
        } else {
          other->PushString(" && ");
        }

        other->PushString("(a%d > a%d)",loopIndex,i);
      }

      String expr = EndString(temp,other);

      if(loopIndex == 0){
        c->If(expr);
      } else {
        // The other ifs are elseifs of the first if done.
        c->ElseIf(expr);
      }

      String comment = PushString(temp,"Loop var %.*s is the largest",UNPACK_SS(external->terms[loopIndex].var));
      c->Comment(comment);
      String r = PushString(temp,"printf(\"LoopIndex: %d\\n\")",loopIndex); 
      //c->Statement(r);

      EmitDoubleOrSingleLoopCode(c,loopIndex,initial);
      
      Recurse(Recurse,loopIndex + 1,c,out);
    } else {
      c->Else();

      String comment = PushString(temp,"Loop var %.*s is the largest",UNPACK_SS(external->terms[loopIndex].var));
      c->Comment(comment);
      String r = PushString(temp,"printf(\"LoopIndex: %d\\n\")",loopIndex); 
      //c->Statement(r);

      EmitDoubleOrSingleLoopCode(c,loopIndex,initial);

      c->EndIf();
    }
  };

  CEmitter* m = StartCCode(temp);

  String functionName = PushString(temp,"CompileVUnit_%.*s",UNPACK_SS(addressGenName));
  m->Function(STRING("static AddressVReadArguments"),functionName);

  m->Argument(STRING("iptr"),STRING("ext"));

  for(String input : inputVars){
    m->Argument(STRING("int"),input);
  }

  m->VarDeclare(STRING("AddressVReadArguments"),varName,STRING("{}"));
  
  for(int i = 0; i <  initial.external->terms.size; i++){
    LoopLinearSumTerm term  =  initial.external->terms[i];
    String repr = PushRepresentation(GetLoopHighestDecider(&term),temp);
    String name = PushString(temp,"a%d",i);
    String comment = PushString(temp,"Loop var: %.*s",UNPACK_SS(term.var));
    m->Comment(comment);
    m->VarDeclare(STRING("int"),name,repr);
  }

  for(int i = 0; i <  initial.external->terms.size; i++){
    LoopLinearSumTerm term  =  initial.external->terms[i];
          
    String repr = PushRepresentation(GetLoopHighestDecider(&term),temp);
  }

  Recurse(Recurse,0,m,temp);

  m->Return(varName);
  CAST* ast = EndCCode(m);

  StringBuilder* b = StartString(temp);
  Repr(ast,b);
  String data = EndString(out,b);

  return data;
}

void OutputTopLevelFiles(Accelerator* accel,FUDeclaration* topLevelDecl,const char* hardwarePath,const char* softwarePath,bool isSimple){
  TEMP_REGION(temp,nullptr);
  TEMP_REGION(temp2,temp);
  
  ClearTemplateEngine(); // Make sure that we do not reuse data
  
  AccelInfo info = CalculateAcceleratorInfo(accel,true,temp);
  FillStaticInfo(&info);

  AccelInfoIterator iter = StartIteration(&info);

  StructInfo* stateStructInfo = GenerateStateStruct(iter,temp);

  Array<StructInfo*> allStateStructs = {};
  Array<TypeStructInfo> stateStructs = {};

  if(!Empty(stateStructInfo->memberList)){
    // We generate an extra level, so we just remove it here.
    stateStructInfo = stateStructInfo->memberList->head->elem.childStruct;

    allStateStructs = ExtractStructs(stateStructInfo,temp);

    // TODO: Maybe turn into a function together with the code below
    Array<int> indexes = PushArray<int>(temp,allStateStructs.size);
    Memset(indexes,2);
    for(int i = 0; i < allStateStructs.size; i++){
      String name = allStateStructs[i]->name;
      
      for(int ii = 0; ii < i; ii++){
        String possibleDuplicate = allStateStructs[ii]->name;

        // TODO: The fact that we are doing string comparison is kinda bad.
        if(CompareString(possibleDuplicate,name)){
          allStateStructs[i]->name = PushString(temp,"%.*s_%d",UNPACK_SS(possibleDuplicate),indexes[ii]++);
          break;
        }
      }
    }

    stateStructs = GenerateStructs(allStateStructs,STRING("State"),false,temp);
  }

  // NOTE: This function also fills the instance info member of the acceleratorInfo. This function only fills the first partition, but I think that it is fine because that is the only one we use. We generate the same structs either way.
  StructInfo* structInfo = GenerateConfigStruct(iter,temp);

  Array<TypeStructInfo> structs = {};
  Array<StructInfo*> allStructs = {};
  
  // If we only contain static configs, this will appear empty.
  if(!Empty(structInfo->memberList)){
    // We generate an extra level, so we just remove it here.
    structInfo = structInfo->memberList->head->elem.childStruct;
  
    // NOTE: We for now push all the merge muxs to the top.
    //       It might be better to only push them to the merge struct (basically only 1 level up).
    //       Still need more examples to see.
    PushMergeMultiplexersUpTheHierarchy(structInfo);
    
    allStructs = ExtractStructs(structInfo,temp);
    
    Array<int> indexes = PushArray<int>(temp,allStructs.size);
    Memset(indexes,2);
    for(int i = 0; i < allStructs.size; i++){
      String name = allStructs[i]->name;
      
      for(int ii = 0; ii < i; ii++){
        String possibleDuplicate = allStructs[ii]->name;

        // TODO: The fact that we are doing string comparison is kinda bad.
        if(CompareString(possibleDuplicate,name)){
          allStructs[i]->name = PushString(temp,"%.*s_%d",UNPACK_SS(possibleDuplicate),indexes[ii]++);
          break;
        }
      }
    }

    structs = GenerateStructs(allStructs,STRING("Config"),true,temp);
  }
  
  auto PrettyPrintMemory = [](int val){
    if(val < 1024){
      printf("%d bytes",val);
    } else if(val < Megabyte(1)){
      int leftover = val % 1024;
      printf("%d.%.3d kilobytes",val / 1024,leftover);
    } else if(val < Gigabyte(1)){
      int leftover = val % (1024 * 1024);
      
      printf("%d.%.3d megabytes",val / (1024 * 1024),leftover);
    } else {
      int leftover = val % (1024 * 1024 * 1024);
      printf("%d.%.3d gigabytes",val / (1024 * 1024 * 1024),leftover);
    }
  };

  VersatComputedValues val = ComputeVersatValues(&info,globalOptions.useDMA);
  printf("Some stats\n");
  printf("CONFIG_BITS: %d\n",val.configurationBits);
  printf("STATE_BITS: %d\n",val.stateBits);
  printf("MEM_USED: ");
  PrettyPrintMemory(val.totalExternalMemory);
  printf("\n");

  printf("UNITS: %d\n",val.nUnits);
  
  // TODO: A lot of cruft in this function
  Hashmap<StaticId,StaticData>* staticUnits = CollectStaticUnits(&info,temp);
  TemplateSetCustom("staticUnits",MakeValue(staticUnits));
  int index = 0;
  Array<Wire> allStaticsVerilatorSide = PushArray<Wire>(temp,999); // TODO: Correct size
  for(Pair<StaticId,StaticData*> p : staticUnits){
    for(Wire& config : p.second->configs){
      allStaticsVerilatorSide[index] = config;
      allStaticsVerilatorSide[index].name = ReprStaticConfig(p.first,&config,temp);
      index += 1;
    }
  }
  allStaticsVerilatorSide.size = index;

  // All these are common to all the following templates usages
  // Since templates only check if data exists at runtime, we want to pass data using C code as much as possible
  // (Because any change to the structs leads to a compile error instead of a runtime error).
  // This is not enforced and its fine if we get a few template errors. Since the tests are fast
  // we likely wont have many problems with this approach.

  // NOTE: Either compile to C or we create a proper backend to emit Verilog code and C code (and keep templates for things like makefiles and such).
  // NOTE2: Need to run some tests to verify which is easier to code in and to maintain. I like the templates since it is helpful to see the structure of the generated code. It is also easier to organize the generic code with the specific code.
  //       I am not a fan of C printfs or C++ ostreams, mainly because it is more difficult to organize the code and because we have to worry about escaping stuff (although maybe not as big as a problem, since verilog and C do not have a lot to escape).
  //       I am also not a fan of emitting code. It is more stable but I lose the ability to see directly how the general code will look like. Trading stability for easy of prototyping and changing things.
  
  // TODO: If free time this should be a weekend project since we are basically just making a different backend.
  TemplateSetString("typeName",accel->name);

  // Dependent on val.
  TemplateSetNumber("configurationBits",val.configurationBits);
  TemplateSetNumber("stateBits",val.stateBits);
  TemplateSetNumber("stateAddressBits",val.stateAddressBits);
  TemplateSetNumber("configurationAddressBits",val.configurationAddressBits);
  TemplateSetNumber("numberConnections",val.numberConnections);
  TemplateSetNumber("delayStart",val.delayBitsStart);
  TemplateSetNumber("nIO",val.nUnitsIO);
  TemplateSetNumber("unitsMapped",val.unitsMapped);
  TemplateSetNumber("memoryMappedBytes",val.memoryMappedBytes);
  TemplateSetNumber("memoryConfigDecisionBit",val.memoryConfigDecisionBit);
  TemplateSetNumber("versatConfig",val.versatConfigs);
  TemplateSetNumber("versatState",val.versatStates);
  TemplateSetNumber("memoryAddressBits",val.memoryAddressBits);
  TemplateSetNumber("memoryMappedBase",1 << val.memoryConfigDecisionBit);
  TemplateSetNumber("nConfigs",val.nConfigs);
  TemplateSetNumber("nStates",val.nStates);
  TemplateSetNumber("nStatics",val.nStatics);
  TemplateSetNumber("delays",val.nDelays);
  TemplateSetNumber("totalExternalMemory",val.totalExternalMemory);

  TemplateSetCustom("arch",MakeValue(&globalOptions));
  
  // NOTE: This data is printed so it can be captured by the IOB python setup.
  // TODO: Probably want a more robust way of doing this. Eventually want to printout some stats so we can
  //       actually visualize what we are producing.
  printf("ADDR_W - %d\n",val.memoryConfigDecisionBit + 1);
  if(val.nUnitsIO){
    printf("HAS_AXI - True\n");
  }

  // Verilog includes for parameters and such
  {
    // No need for templating, small file
    FILE* c = OpenFileAndCreateDirectories(PushString(temp,"%s/versat_defs.vh",hardwarePath),"w",FilePurpose_VERILOG_INCLUDE);
    FILE* f = OpenFileAndCreateDirectories(PushString(temp,"%s/versat_undefs.vh",hardwarePath),"w",FilePurpose_VERILOG_INCLUDE);
    DEFER_CLOSE_FILE(c);
    DEFER_CLOSE_FILE(f);
  
    if(!c || !f){
      printf("Error creating file, check if filepath is correct: %s\n",hardwarePath);
      return;
    }

    fprintf(c,"`define NUMBER_UNITS %d\n",accel->allocated.Size());
    fprintf(f,"`undef  NUMBER_UNITS\n");
    fprintf(c,"`define CONFIG_W %d\n",val.configurationBits);
    fprintf(f,"`undef  CONFIG_W\n");
    fprintf(c,"`define STATE_W %d\n",val.stateBits);
    fprintf(f,"`undef  STATE_W\n");
    fprintf(c,"`define MAPPED_UNITS %d\n",val.unitsMapped);
    fprintf(f,"`undef  MAPPED_UNITS\n");
    fprintf(c,"`define MAPPED_BIT %d\n",val.memoryConfigDecisionBit);
    fprintf(f,"`undef  MAPPED_BIT\n");
    fprintf(c,"`define nIO %d\n",val.nUnitsIO);
    fprintf(f,"`undef  nIO\n");
    fprintf(c,"`define LEN_W %d\n",20);
    fprintf(f,"`undef  LEN_W\n");

    if(globalOptions.architectureHasDatabus){
      fprintf(c,"`define VERSAT_ARCH_HAS_IO 1\n");
      fprintf(f,"`undef  VERSAT_ARCH_HAS_IO\n");
    }

    if(info.inputs || info.outputs){
      fprintf(c,"`define EXTERNAL_PORTS\n");
      fprintf(f,"`undef  EXTERNAL_PORTS\n");
    }

    if(val.nUnitsIO){
      fprintf(c,"`define VERSAT_IO\n");
      fprintf(f,"`undef  VERSAT_IO\n");
    }

    if(val.externalMemoryInterfaces){
      fprintf(c,"`define VERSAT_EXTERNAL_MEMORY\n");
      fprintf(f,"`undef  VERSAT_EXTERNAL_MEMORY\n");
    }

    if(globalOptions.exportInternalMemories){
      fprintf(c,"`define VERSAT_EXPORT_EXTERNAL_MEMORY\n");
      fprintf(f,"`undef  VERSAT_EXPORT_EXTERNAL_MEMORY\n");
    }
  }
    
  // Output configuration file
  DAGOrderNodes order = CalculateDAGOrder(&accel->allocated,temp);
  Array<FUInstance*> ordered = order.instances;
  
  for(FUInstance* node : order.instances){
    if(node->declaration->nIOs){
      SetParameter(node,STRING("AXI_ADDR_W"),STRING("AXI_ADDR_W"));
      SetParameter(node,STRING("AXI_DATA_W"),STRING("AXI_DATA_W"));
      SetParameter(node,STRING("LEN_W"),STRING("LEN_W"));
    }
  }

  Array<ExternalMemoryInterface> external = PushArray<ExternalMemoryInterface>(temp,val.externalMemoryInterfaces);
  int externalIndex = 0;
  for(InstanceInfo& in : info.infos[0].info){
    if(!in.isComposite){
      for(ExternalMemoryInterface& inter : in.decl->externalMemory){
        external[externalIndex++] = inter;
      }
    }
  }

  if(globalOptions.exportInternalMemories){
    int index = 0;
    for(ExternalMemoryInterface inter : external){
      switch(inter.type){
      case ExternalMemoryType::DP:{
        printf("DP - %d",index++);
        for(int i = 0; i < 2; i++){
          printf(",%d",inter.dp[i].bitSize);
          printf(",%d",inter.dp[i].dataSizeOut);
          printf(",%d",inter.dp[i].dataSizeIn);
        }
        printf("\n");
      }break;
      case ExternalMemoryType::TWO_P:{
        printf("2P - %d",index++);
        printf(",%d",inter.tp.bitSizeOut);
        printf(",%d",inter.tp.bitSizeIn);
        printf(",%d",inter.tp.dataSizeOut);
        printf(",%d",inter.tp.dataSizeIn);
        printf("\n");
      }break;
      }
    }
  }

  // TODO: Base this on the AccelInfo approach.
  int staticStart = 0;
  for(FUInstance* ptr : accel->allocated){
    FUDeclaration* decl = ptr->declaration;
    for(Wire& wire : decl->configs){
      staticStart += wire.bitSize;
    }
  }

  // All dependent on external
  TemplateSetCustom("external",MakeValue(&external));
  {
    FILE* f = OpenFileAndCreateDirectories(PushString(temp,"%s/versat_external_memory_inst.vh",hardwarePath),"w",FilePurpose_VERILOG_INCLUDE);
    DEFER_CLOSE_FILE(f);
    ProcessTemplate(f,BasicTemplates::externalInstTemplate);
  }

  {
    FILE* f = OpenFileAndCreateDirectories(PushString(temp,"%s/versat_internal_memory_wires.vh",hardwarePath),"w",FilePurpose_VERILOG_INCLUDE);
    DEFER_CLOSE_FILE(f);
    ProcessTemplate(f,BasicTemplates::internalWiresTemplate);
  }

  {
    FILE* f = OpenFileAndCreateDirectories(PushString(temp,"%s/versat_external_memory_port.vh",hardwarePath),"w",FilePurpose_VERILOG_INCLUDE);
    DEFER_CLOSE_FILE(f);
    ProcessTemplate(f,BasicTemplates::externalPortTemplate);
  }

  {
    FILE* f = OpenFileAndCreateDirectories(PushString(temp,"%s/versat_external_memory_internal_portmap.vh",hardwarePath),"w",FilePurpose_VERILOG_INCLUDE);
    DEFER_CLOSE_FILE(f);
    ProcessTemplate(f,BasicTemplates::externalInternalPortmapTemplate);
 }

  Array<String> memoryMasks = ExtractMemoryMasks(info,temp);

  Pool<FUInstance> nodes = accel->allocated;
  Array<String> parameters = PushArray<String>(temp,nodes.Size());
  for(int i = 0; i < nodes.Size(); i++){
    parameters[i] = GenerateVerilogParameterization(nodes.Get(i),temp);
  }

  Array<InstanceInfo*> topLevelUnits = GetAllSameLevelUnits(&info,0,0,temp);
  TemplateSetCustom("parameters",MakeValue(&parameters));
  TemplateSetCustom("inputDecl",MakeValue(BasicDeclaration::input));
  TemplateSetCustom("outputDecl",MakeValue(BasicDeclaration::output));
  TemplateSetNumber("nInputs",info.inputs);
  TemplateSetNumber("nOutputs",info.outputs);
  TemplateSetCustom("ordered",MakeValue(&ordered));
  TemplateSetCustom("memoryMasks",MakeValue(&memoryMasks));
  TemplateSetCustom("instances",MakeValue(&nodes));
  TemplateSetNumber("staticStart",staticStart);
  TemplateSetNumber("delaySize",DELAY_SIZE);
  TemplateSetBool("useDMA",globalOptions.useDMA);
  TemplateSetCustom("opts",MakeValue(&globalOptions));
  
  int configBit = 0;
  int addr = val.versatConfigs;
  
  // TODO: We are still relying on explicit data contained inside the 
  // Join configs statics and delays into a single array.
  // Simplifies the code gen, since we generate the same code regardless of the wire origin.
  // Only wire, position and stuff like that matters
  auto arr = StartArray<WireInformation>(temp);
  for(auto n : nodes){
    for(Wire w : n->declaration->configs){
      WireInformation info = {};
      info.wire = w;
      info.configBitStart = configBit;
      configBit += w.bitSize;
      info.addr = 4 * addr++;
      *arr.PushElem() = info;
    }
  }
  for(Pair<StaticId,StaticData*> n : staticUnits){
    for(Wire w : n.data->configs){
      WireInformation info = {};
      info.wire = w;
      info.wire.name = ReprStaticConfig(n.first,&w,temp2);
      info.configBitStart = configBit;
      configBit += w.bitSize;
      info.addr = 4 * addr++;
      info.isStatic = true;
      *arr.PushElem() = info;
    }
  }
  int delaysInserted = 0;
  for(auto n : nodes){
    for(int i = 0; i < n->declaration->NumberDelays(); i++){
      Wire wire = {};
      wire.bitSize = DELAY_SIZE;
      wire.name = PushString(temp2,"Delay%d",delaysInserted++);
      wire.stage = VersatStage_COMPUTE;

      WireInformation info = {};
      info.wire = wire;
      info.configBitStart = configBit;
      configBit += DELAY_SIZE;
      info.addr = 4 * addr++;
      *arr.PushElem() = info;
    }
  }
  
  auto test = EndArray(arr);
  TemplateSetCustom("wireInfo",MakeValue(&test));

  // Top accelerator
  {
    FILE* s = OpenFileAndCreateDirectories(PushString(temp,"%s/versat_instance.v",hardwarePath),"w",FilePurpose_VERILOG_CODE);
    DEFER_CLOSE_FILE(s);

    if(!s){
      printf("Error creating file, check if filepath is correct: %s\n",hardwarePath);
      return;
    }

    ProcessTemplate(s,BasicTemplates::topAcceleratorTemplate);
  }

  // Top configurations
  {
    FILE* s = OpenFileAndCreateDirectories(PushString(temp,"%s/versat_configurations.v",hardwarePath),"w",FilePurpose_VERILOG_CODE);
    DEFER_CLOSE_FILE(s);

    if(!s){
      printf("Error creating file, check if filepath is correct: %s\n",hardwarePath);
      return;
    }

    ProcessTemplate(s,BasicTemplates::topConfigurationsTemplate);
  }
  
  TemplateSetBool("isSimple",isSimple);
  if(isSimple){
    FUInstance* inst = nullptr; // TODO: Should probably separate isSimple to a separate function, because otherwise we are recalculating stuff that we already know.
    for(FUInstance* ptr : accel->allocated){
      if(CompareString(ptr->name,STRING("TOP"))){
        inst = ptr;
        break;
      }
    }
    Assert(inst);

    Accelerator* accel = inst->declaration->fixedDelayCircuit;
    if(accel){
      for(FUInstance* ptr : accel->allocated){
        if(CompareString(ptr->name,STRING("simple"))){
          inst = ptr;
          break;
        }
      }
      Assert(inst);

      TemplateSetNumber("simpleInputs",inst->declaration->NumberInputs());
      TemplateSetNumber("simpleOutputs",inst->declaration->NumberOutputs());
    } else {
      TemplateSetNumber("simpleInputs",0);
      TemplateSetNumber("simpleOutputs",0);
    }
  }

  TemplateSetCustom("configStructures",MakeValue(&structs));

  TemplateSetCustom("stateStructures",MakeValue(&stateStructs));

  Array<TypeStructInfo> addressStructures = GetMemMappedStructInfo(&info,temp2);
  TemplateSetCustom("addressStructures",MakeValue(&addressStructures));

  // TODO: There is a lot of repeated code here, but probably will remain until I make the template engine change to work at compile time.
  Array<TypeStructInfoElement> structuredConfigs = ExtractStructuredConfigs(info.infos[0].info,temp);

  // Accelerator header
  {
    auto arr = StartArray<int>(temp);
    for(InstanceInfo& t : info.infos[0].info){
      if(!t.isComposite){
        for(int d : t.extraDelay){
          *arr.PushElem() = d;
        }
      }
    }
    Array<int> delays = EndArray(arr);
    TemplateSetCustom("delay",MakeValue(&delays));

    // TODO: We eventually only want to put this as true if we output at least one address gen.
    TemplateSetBool("simulateLoops",true);
        
    Array<Array<int>> allDelays = PushArray<Array<int>>(temp,info.infos.size);
    if(info.infos.size >= 2){
      int i = 0;
      for(int ii = 0; ii <  info.infos.size; ii++){
        Array<InstanceInfo> allInfos = info.infos[ii].info;
        auto arr = StartArray<int>(temp);
        for(InstanceInfo& t : allInfos){
          if(!t.isComposite){
            for(int d : t.extraDelay){
              *arr.PushElem() = d;
            }
          }
        }
        Array<int> delays = EndArray(arr);
        allDelays[i++] = delays;
      }
    }
    TemplateSetCustom("mergedDelays",MakeValue(&allDelays));
    TemplateSetNumber("amountMerged",allDelays.size);

    auto diffArray = StartArray<DifferenceArray>(temp2);
    for(int oldIndex = 0; oldIndex < allDelays.size; oldIndex++){
      for(int newIndex = 0; newIndex < allDelays.size; newIndex++){
        if(oldIndex == newIndex){
          continue;
        }
        
        Array<Difference> difference = CalculateSmallestDifference(allDelays[oldIndex],allDelays[newIndex],temp);

        DifferenceArray* diff = diffArray.PushElem();
        diff->oldIndex = oldIndex;
        diff->newIndex = newIndex;
        diff->differences = difference;
      }
    }
    Array<DifferenceArray> differenceArray = EndArray(diffArray);
    TemplateSetCustom("differences",MakeValue(&differenceArray));
    TemplateSetCustom("allStatics",MakeValue(&allStaticsVerilatorSide));
    
    Array<String> allStates = ExtractStates(info.infos[0].info,temp2);

    Array<Pair<String,int>> allMem = ExtractMem(info.infos[0].info,temp2);
    
    TemplateSetCustom("structuredConfigs",MakeValue(&structuredConfigs));
    TemplateSetCustom("namedStates",MakeValue(&allStates));
    TemplateSetCustom("namedMem",MakeValue(&allMem));

    TemplateSetBool("outputChangeDelay",false);

    Array<String> names = Extract(info.infos,temp,&MergePartition::name);
    TemplateSetCustom("mergeNames",MakeValue(&names));
    
    auto ExtractMuxInfo = [](AccelInfoIterator iter,Arena* out){
      TEMP_REGION(temp,out);

      auto builder = StartArray<MuxInfo>(out);
      for(; iter.IsValid(); iter = iter.Step()){
        InstanceInfo* info = iter.CurrentUnit();
        if(info->isMergeMultiplexer){
          int muxGroup = info->muxGroup;
          if(!builder[muxGroup].info){
            builder[muxGroup].configIndex = info->globalConfigPos.value();
            builder[muxGroup].val = info->mergePort;
            builder[muxGroup].name = info->baseName;
            builder[muxGroup].info = info;
          }
        }
      }

      return EndArray(builder);
    };
    
    AccelInfoIterator iter = StartIteration(&info);
    Array<Array<MuxInfo>> muxInfo = PushArray<Array<MuxInfo>>(temp,iter.MergeSize());
    for(int i = 0; i < iter.MergeSize(); i++){
      AccelInfoIterator it = iter;
      it.mergeIndex = i;
      muxInfo[i] = ExtractMuxInfo(it,temp);
    }
    
    TemplateSetCustom("mergeMux",MakeValue(&muxInfo));

    // Combines address gen with the struct names that use them. Mostly done this way because we generate C code which does not support method overloading meaning that we have to do it manually.
    TrieSet<Pair<String,AddressGenDef*>>* structNameAndAddressGen = PushTrieSet<Pair<String,AddressGenDef*>>(temp);
    TrieSet<AddressGenDef*>* addressGenUsed = PushTrieSet<AddressGenDef*>(temp);
    
    auto GetAddressGen = [](String addressGenName) -> AddressGenDef*{
      for(AddressGenDef* def : addressGens){
        if(CompareString(def->name,addressGenName)){
          return  def;
        }
      }
      return nullptr;
    };

    for(AccelInfoIterator iter = StartIteration(&info); iter.IsValid(); iter = iter.Step()){
      InstanceInfo* unit = iter.CurrentUnit();

      for(String addressGenName : unit->addressGenUsed){
        Assert(unit->structInfo); // If using address gen, need to have a valid struct info otherwise cannot proceed
        Assert(!Empty(unit->structInfo->name));
        Assert(!Empty(unit->structInfo->originalName));
        
        String typeName = unit->structInfo->name;
        String originalName = unit->structInfo->originalName;
        
        AddressGenDef* def = GetAddressGen(addressGenName);

        if(!def){
          printf("Did not find address gen with name: %.*s\n",UNPACK_SS(addressGenName));
        } else {
          structNameAndAddressGen->Insert({typeName,def});
          addressGenUsed->Insert(def);
        }
      }
    }
    
    // By getting the info directly from the structs, we make sure that we are always generating the functions needed for each possible structure that gets created/changed due to partial share or merge.
    // Address gen is therefore capable of handling all the struct modifing features as long as they are properly implemented inside structInfo.

    auto ConvertAddressGenDef = [](AddressGenDef* def,Arena* out) -> AddressAccess*{
      TEMP_REGION(temp,out);
      
      auto loopVarBuilder = StartArray<String>(temp);
      for(int i = 0; i <  def->loops.size; i++){
        AddressGenForDef loop  =  def->loops[i];

        *loopVarBuilder.PushElem() = PushString(temp,loop.loopVariable);
      }
      Array<String> loopVars = EndArray(loopVarBuilder);
      
      // Builds expression for the internal address which is basically just a multiplication of all the loops sizes
      SymbolicExpression* loopExpression = PushLiteral(temp,1);
      for(AddressGenForDef loop : def->loops){
        SymbolicExpression* start = ParseSymbolicExpression(loop.start,temp);
        SymbolicExpression* end = ParseSymbolicExpression(loop.end,temp);
          
        SymbolicExpression* diff = SymbolicSub(end,start,temp);

        loopExpression = SymbolicMult(loopExpression,diff,temp);
      }
      SymbolicExpression* finalExpression = Normalize(loopExpression,temp);
      
      // Building expression for the external address
      SymbolicExpression* normalized = Normalize(def->symbolic,temp);
      for(String str : loopVars){
        normalized = Group(normalized,str,temp);
      }
        
      LoopLinearSum* expr = PushLoopLinearSumEmpty(temp);
      for(int i = 0; i < loopVars.size; i++){
        String var = loopVars[i];

        Opt<SymbolicExpression*> termOpt = GetMultExpressionAssociatedTo(normalized,var,temp); 

        SymbolicExpression* term = termOpt.value_or(nullptr);
        if(!term){
          term = PushLiteral(temp,0);
        }
          
        AddressGenForDef loop = def->loops[i];
          
        SymbolicExpression* loopStart = ParseSymbolicExpression(loop.start,temp);
        SymbolicExpression* loopEnd = ParseSymbolicExpression(loop.end,temp);
          
        LoopLinearSum* sum = PushLoopLinearSumSimpleVar(loop.loopVariable,term,loopStart,loopEnd,temp);
        expr = AddLoopLinearSum(sum,expr,temp);
      }

      // Extracts the constant term
      SymbolicExpression* toCalcConst = normalized;

      // TODO: Currently we are not dealing with loops that do not start at zero
      SymbolicExpression* zero = PushLiteral(temp,0);
      for(String str : loopVars){
        toCalcConst = SymbolicReplace(toCalcConst,str,zero,temp);
      }
      toCalcConst = Normalize(toCalcConst,temp);

      LoopLinearSum* freeTerm = PushLoopLinearSumFreeTerm(toCalcConst,temp);
      
      AddressAccess* result = PushStruct<AddressAccess>(out);
      result->inputVariableNames = CopyArray<String>(def->inputs,out);
      result->internal = PushLoopLinearSumSimpleVar(STRING("x"),PushLiteral(temp,1),PushLiteral(temp,0),finalExpression,out);
      result->external = AddLoopLinearSum(expr,freeTerm,out);
      return result;
    };

    GrowableArray<String> builder = StartArray<String>(temp2);
    
    // Only one compilation function is generated per address gen, nothing depends on the config structures here.
    for(AddressGenDef* def : addressGenUsed){
      AddressAccess* initial = ConvertAddressGenDef(def,temp);
      String data = GenerateAddressGenCompilationFunction(*initial,def->name,temp);
      *builder.PushElem() = data;
    }

    // TODO: Missing the generic methods that use C++ overloading to simplify the name of the methods

    for(Pair<String,AddressGenDef*> p : structNameAndAddressGen){
      String structName = p.first;
      AddressGenDef* def = p.second;

      AddressAccess* initial = ConvertAddressGenDef(def,temp);
      String argName = PushString(temp,"volatile %.*sConfig*",UNPACK_SS(structName));

      String functionName = PushString(temp,"%.*s_%.*s",UNPACK_SS(def->name),UNPACK_SS(structName));
      String loadFunctionName = PushString(temp,"LoadVUnit_%.*s",UNPACK_SS(structName));

      Array<String> inputVars = initial->inputVariableNames;
      String varName = STRING("config");

      {
        CEmitter* m = StartCCode(temp);
      
        m->Function(STRING("static void"),loadFunctionName);
        m->Argument(argName,varName);
        m->Argument(STRING("AddressVReadArguments"),STRING("args"));

        for(int i = 0; i <  META_AddressReadParameters_Members.size; i++){
          String str = META_AddressReadParameters_Members[i];

          String lhs = PushString(temp,"config->%.*s",UNPACK_SS(str));
          String rhs = PushString(temp,"args.%.*s",UNPACK_SS(str));
          m->Assignment(lhs,rhs);
        }

        CAST* ast = EndCCode(m);

        StringBuilder* strBuilder = StartString(temp);
        Repr(ast,strBuilder);
        String data = EndString(temp,strBuilder);

        *builder.PushElem() = data;
      }

      {
        CEmitter* m = StartCCode(temp);
      
        m->Function(STRING("static void"),functionName);

        m->Argument(argName,varName);
        m->Argument(STRING("iptr"),STRING("ext"));

        for(String input : inputVars){
          m->Argument(STRING("int"),input);
        }

        auto strBuilder = StartString(temp);
        strBuilder->PushString("CompileVUnit_%.*s(ext",UNPACK_SS(def->name));
        for(String input : inputVars){
          strBuilder->PushString(",");
          strBuilder->PushString(input);
        }
        strBuilder->PushString(")");
        String functionCall = EndString(temp,strBuilder);

        String load = PushString(temp,"%.*s(%.*s,args)",UNPACK_SS(loadFunctionName),UNPACK_SS(varName));
  
        m->Assignment(STRING("AddressVReadArguments args"),functionCall);
        m->Statement(load);
        CAST* ast = EndCCode(m);

        {
          StringBuilder* strBuilder = StartString(temp);
          Repr(ast,strBuilder);
          String data = EndString(temp,strBuilder);

          *builder.PushElem() = data;
        }
      }
    }

    Array<String> content = EndArray(builder);
    
    TemplateSetCustom("addrGen",MakeValue(&content));
    
    FILE* f = OpenFileAndCreateDirectories(PushString(temp,"%s/versat_accel.h",softwarePath),"w",FilePurpose_SOFTWARE);
    DEFER_CLOSE_FILE(f);
    ProcessTemplate(f,BasicTemplates::acceleratorHeaderTemplate);
  }

  // Verilator Wrapper (.cpp file that controls simulation)
  {
    Pool<FUInstance> nodes = accel->allocated;

    // We need to bundle config + static (type->config) only contains config, but not static
    // TODO: This is not good. Eventually need to take a look at what is happening inside the wrapper.
    auto arr = StartArray<WireExtra>(temp);
    for(auto n : nodes){
      for(Wire config : n->declaration->configs){
        WireExtra* ptr = arr.PushElem();
        *ptr = config;
        ptr->source = STRING("config->TOP_");
        ptr->source2 = STRING("config->");
      }
    }
    for(Wire& staticWire : allStaticsVerilatorSide){
      WireExtra* ptr = arr.PushElem();
      *ptr = staticWire;
      ptr->source = STRING("statics->");
      ptr->source2 = STRING("statics->");
    }
    Array<WireExtra> allConfigsVerilatorSide = EndArray(arr);

    auto builder = StartArray<ExternalMemoryInterface>(temp);
    for(AccelInfoIterator iter = StartIteration(&info); iter.IsValid(); iter = iter.Next()){
      for(ExternalMemoryInterface& inter : iter.CurrentUnit()->decl->externalMemory){
        *builder.PushElem() = inter;
      }
    }
    auto external = EndArray(builder);

    TemplateSetCustom("allConfigsVerilatorSide",MakeValue(&allConfigsVerilatorSide));

    TemplateSetNumber("delays",info.delays);
    TemplateSetCustom("structuredConfigs",MakeValue(&structuredConfigs));
    TemplateSetCustom("states",MakeValue(&topLevelDecl->states));
    Array<String> statesHeaderSide = ExtractStates(info.infos[0].info,temp);
  
    TemplateSetCustom("statesHeader",MakeValue(&statesHeaderSide));
    TemplateSetCustom("namedStates",MakeValue(&statesHeaderSide));
    TemplateSetNumber("nInputs",info.inputs);
    TemplateSetCustom("opts",MakeValue(&globalOptions));
    TemplateSetBool("implementsDone",info.implementsDone);

    TemplateSetNumber("memoryMapBits",info.memoryMappedBits);
    TemplateSetNumber("nIOs",info.nIOs);
    TemplateSetBool("trace",globalDebug.outputVCD);
    TemplateSetBool("signalLoop",info.signalLoop);
    TemplateSetNumber("numberDelays",info.delays);
    TemplateSetNumber("numberDelays",info.delays);
    TemplateSetCustom("externalMemory",MakeValue(&external));
  
    String wrapperPath = PushString(temp,"%.*s/wrapper.cpp",UNPACK_SS(globalOptions.softwareOutputFilepath));
    FILE* output = OpenFileAndCreateDirectories(wrapperPath,"w",FilePurpose_SOFTWARE);
    DEFER_CLOSE_FILE(output);

    CompiledTemplate* templ = CompileTemplate(versat_wrapper_template,"wrapper",temp);
    ProcessTemplate(output,templ);
  }

  // Makefile file for verilator
  {
    String outputPath = globalOptions.softwareOutputFilepath;
    String verilatorMakePath = PushString(temp,"%.*s/VerilatorMake.mk",UNPACK_SS(outputPath));
    FILE* output = OpenFileAndCreateDirectories(verilatorMakePath,"w",FilePurpose_MAKEFILE);
    DEFER_CLOSE_FILE(output);
  
    TemplateSetBool("traceEnabled",globalDebug.outputVCD);
    CompiledTemplate* comp = CompileTemplate(versat_makefile_template,"makefile",temp);

    // TODO: We eventually only want to put this as true if we output at least one address gen.
    TemplateSetBool("simulateLoops",true);
    
    fs::path outputFSPath = StaticFormat("%.*s",UNPACK_SS(outputPath));
    fs::path srcLocation = fs::current_path();
    fs::path fixedPath = fs::weakly_canonical(outputFSPath / srcLocation);

    String relativePath = GetRelativePathFromSourceToTarget(globalOptions.softwareOutputFilepath,globalOptions.hardwareOutputFilepath,temp);

    Array<String> allFilenames = PushArray<String>(temp,globalOptions.verilogFiles.size);
    for(int i = 0; i <  globalOptions.verilogFiles.size; i++){
      String filepath  =  globalOptions.verilogFiles[i];
      fs::path path(StaticFormat("%.*s",UNPACK_SS(filepath)));
      allFilenames[i] = PushString(temp,"%s",path.filename().c_str());
    }

    TemplateSetArray("allFilenames","String",UNPACK_SS(allFilenames));
  
    TemplateSetString("generatedUnitsLocation",relativePath);
    TemplateSetArray("extraSources","String",UNPACK_SS(globalOptions.extraSources));
    ProcessTemplate(output,comp);

    // TODO: Need to add some form of error checking and handling, for the case where verilator root is not found
    // Used by make to find the verilator root of the build server
    String getVerilatorRootScript = STRING("#!/bin/bash\nTEMP_DIR=$(mktemp -d)\n\npushd $TEMP_DIR &> /dev/null\n\necho \"module Test(); endmodule\" > Test.v\nverilator --cc Test.v &> /dev/null\n\npushd ./obj_dir &> /dev/null\nVERILATOR_ROOT=$(awk '\"VERILATOR_ROOT\" == $1 { print $3}' VTest.mk)\nrm -r $TEMP_DIR\n\necho $VERILATOR_ROOT\n");
    {
      String getVerilatorScriptPath = PushString(temp,"%.*s/GetVerilatorRoot.sh",UNPACK_SS(globalOptions.softwareOutputFilepath));
      FILE* output = OpenFileAndCreateDirectories(getVerilatorScriptPath,"w",FilePurpose_MISC);
      DEFER_CLOSE_FILE(output);

      fprintf(output,"%.*s",UNPACK_SS(getVerilatorRootScript));
      fflush(output);
      OS_SetScriptPermissions(output);
    }
  }
}

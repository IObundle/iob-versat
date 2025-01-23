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

#include "versatSpecificationParser.hpp"

// TODO: REMOVE: Remove after proper implementation of AddressGenerators
Pool<AddressGenDef> savedAddressGen;

Array<Difference> CalculateSmallestDifference(Array<int> oldValues,Array<int> newValues,Arena* out){
  Assert(oldValues.size == newValues.size); // For now

  int size = oldValues.size;

  DynamicArray<Difference> arr = StartArray<Difference>(out);
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

// TODO: Can reuse SortTypes functions by receiving the Array of Arrays as an argument. Useful for memory and probably state and so on.
#if 0
Array<FUDeclaration*> SortTypesByConfigDependency(Array<FUDeclaration*> types,Arena* out,Arena* temp){
  BLOCK_REGION(temp);

  int size = types.size;

  int stored = 0;
  Array<FUDeclaration*> result = PushArray<FUDeclaration*>(out,size);
  
  Hashmap<FUDeclaration*,bool>* seen = PushHashmap<FUDeclaration*,bool>(temp,size);
  Array<Array<FUDeclaration*>> subTypes = PushArray<Array<FUDeclaration*>>(temp,size);
  Memset(subTypes,{});

  for(int i = 0; i < size; i++){
    subTypes[i] = ConfigSubTypes(types[i]->fixedDelayCircuit,temp,out);
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
#endif

Array<FUDeclaration*> SortTypesByMemDependency(Array<FUDeclaration*> types,Arena* out,Arena* temp){
  BLOCK_REGION(temp);

  int size = types.size;

  int stored = 0;
  Array<FUDeclaration*> result = PushArray<FUDeclaration*>(out,size);

  Hashmap<FUDeclaration*,bool>* seen = PushHashmap<FUDeclaration*,bool>(temp,size);
  Array<Array<FUDeclaration*>> subTypes = PushArray<Array<FUDeclaration*>>(temp,size);
  Memset(subTypes,{});

  for(int i = 0; i < size; i++){
    subTypes[i] = MemSubTypes(types[i]->fixedDelayCircuit,temp,out); // TODO: We can reuse the SortTypesByConfigDependency function if we change it to receive the subTypes array from outside, since the rest of the code is equal.
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

#if 0
Array<TypeStructInfoElement> GenerateStructFromType(FUDeclaration* decl,Arena* out,Arena* temp){
  if(decl->type == FUDeclarationType_SPECIAL){
    return {};
  }

  if(decl->type == FUDeclarationType_SINGLE){
    int size = decl->configs.size;
    Array<TypeStructInfoElement> entries = PushArray<TypeStructInfoElement>(out,size);

    for(int i = 0; i < size; i++){
      entries[i].typeAndNames = PushArray<SingleTypeStructElement>(out,1);
      entries[i].typeAndNames[0].type = STRING("iptr");
      entries[i].typeAndNames[0].name = decl->configs[i].name;
    }
    return entries;
  }

  BLOCK_REGION(temp);

  CalculatedOffsets& offsets = decl->baseConfig.configOffsets;
  Accelerator* accel = decl->fixedDelayCircuit;
  int configSize = offsets.max;

  Array<int> configAmount = PushArray<int>(temp,configSize);
  for(int i = 0; i < accel->allocated.Size(); i++){
    FUInstance* node = accel->allocated.Get(i);
    FUDeclaration* decl = node->declaration;
    
    int configOffset = offsets.offsets[i];
    if(configOffset < 0 || configOffset >= 0x40000000){
      continue;
    }

    int numberConfigs = decl->configs.size;
    configAmount[configOffset] += 1; // Counts how many configs a given offset has.
  }
  
  Array<int> nonZeroConfigs = GetNonZeroIndexes(configAmount,temp);
  Hashmap<int,int>* mapOffsetToIndex = PushHashmap<int,int>(temp,nonZeroConfigs.size);
  for(int i = 0; i < nonZeroConfigs.size; i++){
    mapOffsetToIndex->Insert(nonZeroConfigs[i],i);
  }

  int numberEntries = nonZeroConfigs.size;
  Array<TypeStructInfoElement> entries = PushArray<TypeStructInfoElement>(out,numberEntries);

  // Allocates space for entries
  for(int i = 0; i < numberEntries; i++){
    entries[i].typeAndNames = PushArray<SingleTypeStructElement>(out,configAmount[nonZeroConfigs[i]]);
  }
  
  Array<int> configSeen = PushArray<int>(temp,numberEntries);
  for(int i = 0; i < accel->allocated.Size(); i++){
    FUInstance* node = accel->allocated.Get(i);
    FUDeclaration* decl = node->declaration;

    int configOffset = offsets.offsets[i];
    if(configOffset < 0 || configOffset >= 0x40000000){
      continue;
    }

    Assert(offsets.offsets[i] >= 0);

    int index = mapOffsetToIndex->GetOrFail(configOffset);
    entries[index].typeAndNames[configSeen[index]].type = PushString(out,"%.*sConfig",UNPACK_SS(decl->name));
    entries[index].typeAndNames[configSeen[index]].name = node->name;
    configSeen[index] += 1;
  }

  return entries;
}
#endif

#if 0
Array<SubTypesInfo> GetSubTypesInfo(Accelerator* accel,Arena* out,Arena* temp){
  BLOCK_REGION(temp);
  
  Array<FUDeclaration*> typesUsed = ConfigSubTypes(accel,out,temp);
  typesUsed = SortTypesByConfigDependency(typesUsed,temp,out);

  Set<SubTypesInfo>* info = PushSet<SubTypesInfo>(temp,99);
  
  for(FUDeclaration* type : typesUsed){
    if(type->type == FUDeclarationType_MERGED){
      for(int i = 0; i < type->ConfigInfoSize(); i++){
        ConfigurationInfo in = type->configInfo[i];
        SubTypesInfo res = {};
        res.type = nullptr; // in.baseType;
        res.mergeTop = type;
        res.info = &type->configInfo[i];
        res.isFromMerged = true;
 
        info->Insert(res); // Takes precedence over non merged types
      }

      SubTypesInfo res = {};
      res.type = type;
      res.containsMerged = true;
      info->Insert(res);
    } else {
      SubTypesInfo in = {};
      in.type = type;

      info->Insert(in);
    }
  }
  
  Array<SubTypesInfo> result = PushArrayFromSet(out,info);
  
  return result;
}
#endif

// NOTE: The biggest source of complexity comes from merging, since merging affects the lower structures,
//       causing the addition of padded members
#if 0
Array<TypeStructInfo> GetConfigStructInfo(Accelerator* accel,Arena* out,Arena* temp){
  BLOCK_REGION(temp);

  Array<FUDeclaration*> typesUsed = ConfigSubTypes(accel,out,temp);
  typesUsed = SortTypesByConfigDependency(typesUsed,temp,out);

  // Get all the subtypes used (type and wether it was inside a merge or not).
  // Inside a merge we need to create a merged view of the structure
  Array<SubTypesInfo> subTypesInfo =  GetSubTypesInfo(accel,out,temp);
  
  Array<TypeStructInfo> structures = PushArray<TypeStructInfo>(out,typesUsed.size + 99); // TODO: Small hack to handle merge for now.
  int index = 0;
  for(SubTypesInfo subType : subTypesInfo){
    // 
    if(subType.isFromMerged){
      FUDeclaration* decl = subType.mergeTop;
      int maxOffset = decl->MaxConfigs(); // TODO: This should be equal for all merged types, no need to duplicate data
      ConfigurationInfo& info = *subType.info;
      CalculatedOffsets& offsets = info.configOffsets;
        
      Array<bool> seenIndex = PushArray<bool>(temp,offsets.max);
        
      for(int i = 0; i < decl->fixedDelayCircuit->allocated.Size(); i++){
        FUInstance* node = decl->fixedDelayCircuit->allocated.Get(i);
        int config = offsets.offsets[i];
          
        if(config >= 0){
          int nConfigs = node->declaration->configs.size;
          for(int ii = 0; ii < nConfigs; ii++){
            seenIndex[config + ii] = true;
          }
        }
      }
        
      ArenaList<TypeStructInfoElement>* list = PushArenaList<TypeStructInfoElement>(temp);
        
      int unused = 0;
      int configNeedToSee = 0;
      while(1){
        int unusedToPut = 0;
        while(configNeedToSee < maxOffset && seenIndex[configNeedToSee] == false){
          unusedToPut += 1;
          configNeedToSee += 1;
          }

        if(unusedToPut){
          TypeStructInfoElement* elem = PushListElement(list);
          *elem = {};
          elem->typeAndNames = PushArray<SingleTypeStructElement>(out,1);
          elem->typeAndNames[0] = {};
          elem->typeAndNames[0].type = STRING("iptr");
          elem->typeAndNames[0].name = PushString(out,"unused%d",unused++);
          elem->typeAndNames[0].arraySize = unusedToPut;
        }
          
        if(configNeedToSee >= maxOffset){
          break;
        }

        for(int i = 0; i < decl->fixedDelayCircuit->allocated.Size(); i++){
          FUInstance* node = decl->fixedDelayCircuit->allocated.Get(i);
          int config = offsets.offsets[i];
          if(configNeedToSee == config){
            int nConfigs = node->declaration->configs.size;
              
            TypeStructInfoElement* elem = PushListElement(list);
            elem->typeAndNames = PushArray<SingleTypeStructElement>(out,1);
            elem->typeAndNames[0] = {};
            elem->typeAndNames[0].type = PushString(out,"%.*sConfig",UNPACK_SS(node->declaration->name));
            elem->typeAndNames[0].name = info.baseName[i];
              
            configNeedToSee += nConfigs;
          }
        }
      }

      Array<TypeStructInfoElement> elem = PushArrayFromList(out,list);
      structures[index].name = PushString(out,"%.*s",UNPACK_SS(info.name));
      structures[index].entries = elem;
      index += 1;
    } else if(subType.containsMerged){
      // Contains a merged unit. 
      FUDeclaration* decl = subType.type;

      Array<TypeStructInfoElement> merged = PushArray<TypeStructInfoElement>(out,1);

      merged[0].typeAndNames = PushArray<SingleTypeStructElement>(out,decl->ConfigInfoSize());
      for(int i = 0; i < decl->ConfigInfoSize(); i++){
        merged[0].typeAndNames[i] = {};
        merged[0].typeAndNames[i].type = PushString(out,"%.*sConfig",UNPACK_SS(decl->configInfo[i].name));
        merged[0].typeAndNames[i].name = decl->configInfo[i].name;
      }
      structures[index].name = PushString(out,"%.*s",UNPACK_SS(decl->name));
      structures[index].entries = merged;
      index += 1;
    } else {
      FUDeclaration* decl = subType.type;
      Array<TypeStructInfoElement> val = GenerateStructFromType(decl,out,temp);
      structures[index].name = decl->name;
      structures[index].entries = val;
      index += 1;
    }
  }
  structures.size = index;
  
  return structures;
}
#endif

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

static Array<TypeStructInfo> GetMemMappedStructInfo(Accelerator* accel,Arena* out,Arena* temp){
  BLOCK_REGION(temp);

  Array<FUDeclaration*> typesUsed = MemSubTypes(accel,out,temp);
  typesUsed = SortTypesByMemDependency(typesUsed,temp,out);

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
    } else if(unit->memMappedMask.has_value()){
      *builder.PushElem() = {}; // Empty mask (happens when only one unit with mem exists, bit weird but no need to change for now);
    }
  }

  return EndArray(builder);
}

#if 0
Array<TypeStructInfoElement> ExtractStructuredStatics(Array<InstanceInfo> info,Arena* out,Arena* temp){
  BLOCK_REGION(temp);

  Hashmap<StaticId,int>* seenStatic = PushHashmap<StaticId,int>(temp,999);

  int maxConfig = 0;
  for(InstanceInfo& in : info){
#if 0
    if(in.isComposite){
      StaticId id = {};
      id.name = in.name;
      id.parent = in.parentDeclaration;
      if(seenStatic->ExistsOrInsert(id)){
        for(int i = 0; i < in.configSize; i++){
          int config = i + in.configPos.value();

          maxConfig = std::max(maxConfig,config);
          GetOrAllocateResult<ArenaList<String>*> res = map->GetOrAllocate(config);

          if(!res.alreadyExisted){
            *res.data = PushArenaList<String>(temp);
          }

          ArenaList<String>* list = *res.data;
          String name = PushString(out,"TOP_%.*s_%.*s_%.*s",UNPACK_SS(in.parentDeclaration->name),UNPACK_SS(in.name),UNPACK_SS(in.decl->configInfo.configs[i].name));

          *PushListElement(list) = name;
        }
        continue;
      }
    }
#endif

    if(in.isComposite || !in.configPos.has_value() || in.isConfigStatic){
      continue;
    }
    
    for(int i = 0; i < in.configSize; i++){
      int config = i + in.configPos.value();

      maxConfig = std::max(maxConfig,config);
      GetOrAllocateResult<ArenaList<String>*> res = map->GetOrAllocate(config);

      if(!res.alreadyExisted){
        *res.data = PushArenaList<String>(temp);
      }
    
      ArenaList<String>* list = *res.data;
      String name = PushString(out,"%.*s_%.*s",UNPACK_SS(in.fullName),UNPACK_SS(in.decl->configInfo.configs[i].name));

      *PushListElement(list) = name;
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
    for(ListedStruct<String>* ptr = list->head; ptr; ptr = ptr->next){
      String elem = ptr->elem;
      arr[index].type = STRING("iptr");
      arr[index].name = elem;
      
      index += 1;
    }

    PushListElement(elems)->typeAndNames = arr;
  }
  
  return PushArrayFromList(out,elems);
}
#endif

Array<TypeStructInfoElement> ExtractStructuredConfigs(Array<InstanceInfo> info,Arena* out,Arena* temp){
  BLOCK_REGION(temp);

  Hashmap<int,ArenaList<String>*>* map = PushHashmap<int,ArenaList<String>*>(temp,9999);
  
  int maxConfig = 0;
  for(InstanceInfo& in : info){
    if(in.isComposite || !in.configPos.has_value() || in.isConfigStatic){
      continue;
    }
    
    for(int i = 0; i < in.configSize; i++){
      int config = i + in.configPos.value();

      maxConfig = std::max(maxConfig,config);
      GetOrAllocateResult<ArenaList<String>*> res = map->GetOrAllocate(config);

      if(!res.alreadyExisted){
        *res.data = PushArenaList<String>(temp);
      }

      ArenaList<String>* list = *res.data;
      String name = PushString(out,"%.*s_%.*s",UNPACK_SS(in.fullName),UNPACK_SS(in.decl->configs[i].name));

      // Quick and dirty way of skipping same name
      bool skip = false;
      for(ListedStruct<String>* ptr = list->head; ptr; ptr = ptr->next){
        if(CompareString(ptr->elem,name)){
          skip = true;
        }
      }
      // Cannot put same name since header file gives collision later on.
      if(skip){
        continue;
      }
      
      *PushListElement(list) = name;
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
    for(ListedStruct<String>* ptr = list->head; ptr; ptr = ptr->next){
      String elem = ptr->elem;
      arr[index].type = STRING("iptr");
      arr[index].name = elem;
      
      index += 1;
    }

    PushListElement(elems)->typeAndNames = arr;
  }
  
  return PushArrayFromList(out,elems);
}

String GenerateVerilogParameterization(FUInstance* inst,Arena* out){
  FUDeclaration* decl = inst->declaration;
  Array<String> parameters = decl->parameters;
  int size = parameters.size;

  if(size == 0){
    return {};
  }
  
  auto mark = MarkArena(out);
  
  auto builder = StartString(out);
  PushString(out,"#(");
  bool insertedOnce = false;
  for(int i = 0; i < size; i++){
    ParameterValue v = inst->parameterValues[i];
    String parameter = parameters[i];
    
    if(!v.val.size){
      continue;
    }

    if(insertedOnce){
      PushString(out,",");
    }
    PushString(out,".%.*s(%.*s)",UNPACK_SS(parameter),UNPACK_SS(v.val));
    insertedOnce = true;
  }
  PushString(out,")");

  if(!insertedOnce){
    PopMark(mark);
    return {};
  }

  return EndString(builder);
}

void OutputCircuitSource(FUDeclaration* decl,FILE* file,Arena* temp,Arena* temp2){
  Accelerator* accel = decl->fixedDelayCircuit;

  BLOCK_REGION(temp);
  BLOCK_REGION(temp2);
  
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

  // TODO:
  // All these should be moved off
  // There is no reason why OutputCircuitSource should ever need to take 2 arenas
  // AccelInfo being recalculated again is not good. We are recalculating stuff too much
  // program is still fast, but its a sign of poor architecture
  AccelInfo info = CalculateAcceleratorInfo(accel,true,temp,temp2);
  //VersatComputedValues val = ComputeVersatValues(accel,false);
  VersatComputedValues val = ComputeVersatValues(&info,false);
  Array<String> memoryMasks = ExtractMemoryMasks(info,temp);

  Array<String> parameters = PushArray<String>(temp,nodes.Size());
  for(int i = 0; i < nodes.Size(); i++){
    parameters[i] = GenerateVerilogParameterization(nodes.Get(i),temp);
  }

  Array<InstanceInfo*> topLevelUnits = GetAllSameLevelUnits(&info,0,0,temp);
  // #{set configStart accel.baseConfig.configOffsets.offsets[id]}

  TemplateSetCustom("topLevel",MakeValue(&topLevelUnits));
  TemplateSetCustom("parameters",MakeValue(&parameters));
  TemplateSetNumber("unitsMapped",val.unitsMapped);
  TemplateSetNumber("numberConnections",info.numberConnections);

  TemplateSetCustom("inputDecl",MakeValue(BasicDeclaration::input));
  TemplateSetCustom("outputDecl",MakeValue(BasicDeclaration::output));
  TemplateSetCustom("arch",MakeValue(&globalOptions));
  TemplateSetCustom("accel",MakeValue(decl));
  TemplateSetCustom("memoryMasks",MakeValue(&memoryMasks));
  TemplateSetNumber("delaySize",DELAY_SIZE);
 
  TemplateSetCustom("instances",MakeValue(&nodes));

  ProcessTemplate(file,BasicTemplates::acceleratorTemplate,temp,temp2);
}

void OutputIterativeSource(FUDeclaration* decl,FILE* file,Arena* temp,Arena* temp2){
  Accelerator* accel = decl->fixedDelayCircuit;

  BLOCK_REGION(temp);
  BLOCK_REGION(temp2);

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
   
  //VersatComputedValues val = ComputeVersatValues(accel,false);
  AccelInfo info = CalculateAcceleratorInfo(accel,true,temp,temp2); // TODO: Calculating info just for the computedData calculation is a bit wasteful.

  VersatComputedValues val = ComputeVersatValues(&info,false);

  Array<String> memoryMasks = ExtractMemoryMasks(info,temp);

  Hashmap<StaticId,StaticData>* staticUnits = CollectStaticUnits(accel,decl,temp);
  
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

  ProcessTemplate(file,BasicTemplates::iterativeTemplate,temp,temp2);
}

void OutputVerilatorWrapper(FUDeclaration* type,Accelerator* accel,String outputPath,Arena* temp,Arena* temp2){
  BLOCK_REGION(temp);
  BLOCK_REGION(temp2);

  ClearTemplateEngine(); // Make sure that we do not reuse data

  AccelInfo info = CalculateAcceleratorInfo(accel,true,temp,temp2);
  Array<Wire> allConfigsHeaderSide = ExtractAllConfigs(info.infos[0].info,temp,temp2);

  // We need to bundle config + static (type->config) only contains config, but not static
  auto arr = StartArray<Wire>(temp);
  for(Wire& config : type->configs){
    *arr.PushElem() = config;
  }
  Array<Wire> allConfigsVerilatorSide = EndArray(arr);
  TemplateSetCustom("allConfigsVerilatorSide",MakeValue(&allConfigsVerilatorSide));

  int index = 0;
  Array<Wire> allStaticsVerilatorSide = PushArray<Wire>(temp,999); // TODO: Correct size
  
  Hashmap<StaticId,StaticData>* staticUnits = CollectStaticUnits(accel,type,temp);  
  for(Pair<StaticId,StaticData*> p : staticUnits){
    for(Wire& config : p.second->configs){
      allStaticsVerilatorSide[index] = config;
      allStaticsVerilatorSide[index].name = ReprStaticConfig(p.first,&config,temp);
      index += 1;
    }
  }
  allStaticsVerilatorSide.size = index;
  TemplateSetCustom("allStaticsVerilatorSide",MakeValue(&allStaticsVerilatorSide));
  
  Array<TypeStructInfoElement> structuredConfigs = ExtractStructuredConfigs(info.infos[0].info,temp,temp2);
  
  TemplateSetNumber("delays",info.delays);
  TemplateSetCustom("structuredConfigs",MakeValue(&structuredConfigs));
  Array<String> statesHeaderSide = ExtractStates(info.infos[0].info,temp);
  
  int totalExternalMemory = ExternalMemoryByteSize(type->externalMemory);

  TemplateSetCustom("configsHeader",MakeValue(&allConfigsHeaderSide));
  TemplateSetCustom("statesHeader",MakeValue(&statesHeaderSide));
  TemplateSetCustom("namedStates",MakeValue(&statesHeaderSide));
  TemplateSetNumber("totalExternalMemory",totalExternalMemory);
  TemplateSetCustom("opts",MakeValue(&globalOptions));
  TemplateSetCustom("type",MakeValue(type));
  TemplateSetBool("trace",globalDebug.outputVCD);

  String wrapperPath = PushString(temp,"%.*s/wrapper.cpp",UNPACK_SS(outputPath));
  FILE* output = OpenFileAndCreateDirectories(wrapperPath,"w",FilePurpose_SOFTWARE);
  DEFER_CLOSE_FILE(output);

  CompiledTemplate* templ = CompileTemplate(versat_wrapper_template,"wrapper",temp,temp2);
  ProcessTemplate(output,templ,temp,temp2);
}

// TODO: Move all this stuff to a better place
#include <filesystem>
namespace fs = std::filesystem;

String GetRelativePathThatGoesFromSourceToTarget(String sourcePath,String targetPath,Arena* out,Arena* temp){
  fs::path source(StaticFormat("%.*s",UNPACK_SS(sourcePath)));
  fs::path target(StaticFormat("%.*s",UNPACK_SS(targetPath)));

  fs::path res = fs::relative(target,source);

  return PushString(out,"%s",res.c_str());
}

void OutputVerilatorMake(String topLevelName,String versatDir,Arena* temp,Arena* temp2){
  BLOCK_REGION(temp);
  BLOCK_REGION(temp2);
  
  String outputPath = globalOptions.softwareOutputFilepath;
  String verilatorMakePath = PushString(temp,"%.*s/VerilatorMake.mk",UNPACK_SS(outputPath));
  FILE* output = OpenFileAndCreateDirectories(verilatorMakePath,"w",FilePurpose_MAKEFILE);
  DEFER_CLOSE_FILE(output);
  
  TemplateSetBool("traceEnabled",globalDebug.outputVCD);
  CompiledTemplate* comp = CompileTemplate(versat_makefile_template,"makefile",temp,temp2);
    
  fs::path outputFSPath = StaticFormat("%.*s",UNPACK_SS(outputPath));
  fs::path srcLocation = fs::current_path();
  fs::path fixedPath = fs::weakly_canonical(outputFSPath / srcLocation);

  String srcDir = PushString(temp,"%.*s/src",UNPACK_SS(outputPath));

  String relativePath = GetRelativePathThatGoesFromSourceToTarget(globalOptions.softwareOutputFilepath,globalOptions.hardwareOutputFilepath,temp,temp2);

  Array<String> allFilenames = PushArray<String>(temp,globalOptions.verilogFiles.size);
  for(int i = 0; i <  globalOptions.verilogFiles.size; i++){
    String filepath  =  globalOptions.verilogFiles[i];
    fs::path path(StaticFormat("%.*s",UNPACK_SS(filepath)));
    allFilenames[i] = PushString(temp,"%s",path.filename().c_str());
  }

  TemplateSetArray("allFilenames","String",UNPACK_SS(allFilenames));
  
  TemplateSetCustom("arch",MakeValue(&globalOptions));
  TemplateSetString("srcDir",srcDir);
  TemplateSetString("versatDir",versatDir);
  TemplateSetString("verilatorRoot",globalOptions.verilatorRoot);
  TemplateSetNumber("bitWidth",globalOptions.databusAddrSize);
  TemplateSetString("generatedUnitsLocation",relativePath);
  TemplateSetArray("verilogFiles","String",UNPACK_SS(globalOptions.verilogFiles));
  TemplateSetArray("extraSources","String",UNPACK_SS(globalOptions.extraSources));
  TemplateSetArray("includePaths","String",UNPACK_SS(globalOptions.includePaths));
  TemplateSetString("typename",topLevelName);
  TemplateSetString("rootPath",STRING(fixedPath.c_str()));
  ProcessTemplate(output,comp,temp,temp2);
}

// TODO: A little bit hardforced. Multiplexer info needs to be revised.
Array<Array<MuxInfo>> GetAllMuxInfo(AccelInfo* info,Arena* out,Arena* temp){
  Set<SameMuxEntities>* knownSharedIndexes = PushSet<SameMuxEntities>(temp,99);
  
  for(InstanceInfo& f : info->infos[0].info){
    if(f.isMergeMultiplexer){
      knownSharedIndexes->Insert({f.configPos.value(),&f});
    }
  }

  Array<SameMuxEntities> listOfSharedIndexes = PushArrayFromSet(temp,knownSharedIndexes);
  Array<Array<MuxInfo>> result = PushArray<Array<MuxInfo>>(out,info->infos.size);

  for(int i = 0; i < result.size; i++){
    result[i] = PushArray<MuxInfo>(out,info->infos[i].muxConfigs.size);

    Assert(listOfSharedIndexes.size == info->infos[i].muxConfigs.size);

    for(int ii = 0; ii < info->infos[i].muxConfigs.size; ii++){
      MuxInfo r = {};

      SameMuxEntities ent = listOfSharedIndexes[ii];
      
      r.val = info->infos[i].muxConfigs[ii];
      r.configIndex = ent.configPos;  //associatedInfo.first->configPos.value();
      r.info = ent.info;
      r.name = r.info->fullName;
      
      result[i][ii] = r;
    }
  }
  
  return result;
}

// NOTE: The one thing that we can be sure of, is that the arrangement of the structs is always the same.
//       The difference is wether the unit contains certain members (or padding)
//       and wether the members are union or not.

// What is the format that we want?
// Because of merge, we can have structures that are different even though they represent the same module.
// That means that a structure might point to two different but same "type" strucutures.
// The difference in type is given by the 

// We set the iter to merge N.
// We calculate the members for merge N.
// We set iter to merge N + 1.
// We calculate members for merge N + 1.
// If they are the same, we can collapse them into a single one.
// Otherwise, need to create a union.

// Let's start simple. Let's generate the view for merge 0, then generate the view for merge 1 and go from there.
// The problem of individual merges is that we then need to join them back together in order to resolve the union part.
//

// What do I want?
// I want to make the generated structs more data oriented, so I can then make functions to extract stuff, like the expression to access a member.
// I also want to make the structures as simple as possible.
// Structs are arrays of elements which are multiple or one member.

// Because of this, I think that I need to change StructInfo to contain the multiple types associated to the merges.
// Therefore, a StructInfo must have an array for each merge index.
// We are going to make duplicates, but it is fine for now. We handle those later.

// For an accelerator that contains multiple units, the top level should just be a 

// Now that I think about it, only the top structure "cares" about merge configs, in relation to the fact that we have to put "unions".
// That means that we just need to put the merge multiplexers on the top struct (I think. Need to generate a test that uses multiple merges and modules and then start making everything work properly).

StructInfo* GenerateConfigStruct(AccelInfoIterator iter,Array<Partition> partitions,String topName,Arena* out,Arena* temp){
  StructInfo result = {};
  InstanceInfo* parent = iter.GetParentUnit();
 
  if(parent && parent->isMerge){
    result.name = iter.GetMergeName();
  } else if(parent){
    result.name = parent->decl->name;
  } else {
    result.name = topName;
  }
  
  auto list = PushArenaList<StructElement>(temp);
  for(AccelInfoIterator it = iter; it.IsValid(); it = it.Next()){
      
    InstanceInfo* unit = it.CurrentUnit();
    StructElement elem = {};
      
    if(!unit->configPos.has_value()){
      continue;
    }

    elem.name = unit->baseName;
    if(unit->isMerge){
      for(int i = 0; i < iter.MergeSize(); i++){
        AccelInfoIterator inside = it.StepInsideOnly();
        inside.mergeIndex = i;

        StructInfo* subInfo = GenerateConfigStruct(inside,partitions,{},out,temp);
        elem.name = inside.GetMergeName();
        elem.childStruct = subInfo;
        elem.pos = unit->configPos.value();
        elem.size = unit->configSize;
        elem.isMergeMultiplexer = unit->isMergeMultiplexer;

        *list->PushElem() = elem;
      }
      continue;
    } else if(unit->isComposite){
      AccelInfoIterator inside = it.StepInsideOnly();
      StructInfo* subInfo = GenerateConfigStruct(inside,partitions,{},out,temp);
      elem.childStruct = subInfo;
    } else {
      StructInfo* simpleSubInfo = PushStruct<StructInfo>(out);
      simpleSubInfo->type = unit->decl;
      simpleSubInfo->name = unit->decl->name;
      elem.childStruct = simpleSubInfo;
    }
      
    elem.pos = unit->configPos.value();
    elem.size = unit->configSize;
    elem.isMergeMultiplexer = unit->isMergeMultiplexer;
      
    *list->PushElem() = elem;
  }
      
  result.elements = PushArrayFromList(out,list);

  StructInfo* res = PushStruct<StructInfo>(out);
  *res = result;
  
  return res;
}

size_t HashStructInfo(StructInfo* info){
  return std::hash<StructInfo>()(*info);
}

Array<StructInfo*> ExtractStructs(StructInfo* structInfo,Arena* out,Arena* temp){
  BLOCK_REGION(temp);

  TrieMap<StructInfo,StructInfo*>* info = PushTrieMap<StructInfo,StructInfo*>(temp);

  auto Recurse = [](auto Recurse,StructInfo* top,TrieMap<StructInfo,StructInfo*>* map) -> void {
    if(!top){
      return;
    }

    for(StructElement elem : top->elements){
      Recurse(Recurse,elem.childStruct,map);
    }
    map->InsertIfNotExist(*top,top);
  };

  if(structInfo->elements.size == 1){
    for(StructElement elem : structInfo->elements){
      Recurse(Recurse,elem.childStruct,info);
    }
  } else {
    // For merge top level units we must also store the struct info since we need to generate the top level struct that contains the union of the merge types.
    // NOTE: Do not know if this is the best approach, or if we should just have this function call everytime and in later functions we detect that top level is not merge and we do not generate that structure.
    Recurse(Recurse,structInfo,info);
  }
  
  Array<StructInfo*> res = PushArrayFromTrieMapData(out,info);

  return res;
}

#if 1
Array<TypeStructInfo> GenerateStructs(Array<StructInfo*> info,Arena* out,Arena* temp){
  ArenaList<TypeStructInfo>* list = PushArenaList<TypeStructInfo>(temp);

  for(StructInfo* structInfo : info){
    // Simple struct for simple types
    if(structInfo->type){
      Array<Wire> configs = structInfo->type->configs;
      int size = configs.size;

      TypeStructInfo* type = list->PushElem();
      
      type->name = structInfo->name;
      type->entries = PushArray<TypeStructInfoElement>(out,size);

      for(int i = 0; i < size; i++){
        type->entries[i].typeAndNames = PushArray<SingleTypeStructElement>(out,1);
        type->entries[i].typeAndNames[0].name = configs[i].name;
        type->entries[i].typeAndNames[0].type = STRING("iptr");
        type->entries[i].typeAndNames[0].arraySize = 0;
      }
    } else {
      TypeStructInfo* type = list->PushElem();
      type->name = structInfo->name;

      int size = structInfo->elements.size;

      int maxPos = 0;
      for(int i = 0; i < size; i++){
        StructElement elem = structInfo->elements[i];
        maxPos = std::max(maxPos,elem.pos);
      }
      
      Array<int> amountOfEntriesAtPos = PushArray<int>(temp,maxPos+1);
      for(int i = 0; i < size; i++){
        StructElement elem = structInfo->elements[i];
        amountOfEntriesAtPos[elem.pos] += 1;
      }

      int amountOfDifferent = 0;
      for(int amount : amountOfEntriesAtPos){
        if(amount){
          amountOfDifferent += 1;
        }
      }
        
      Array<int> entryIndex = PushArray<int>(temp,maxPos+1);
      Memset(entryIndex,-1);
      Array<int> indexes = PushArray<int>(temp,maxPos+1);
      
      type->entries = PushArray<TypeStructInfoElement>(out,amountOfDifferent);
      int indexPos = 0;
      for(int i = 0; i < size; i++){
        StructElement elem = structInfo->elements[i];
        int pos = elem.pos;

        int index = entryIndex[elem.pos];
        if(index == -1){
          entryIndex[elem.pos] = indexPos++;
          index = entryIndex[elem.pos];
        }

        if(!type->entries[index].typeAndNames.size){
          int amount = amountOfEntriesAtPos[pos];
          type->entries[index].typeAndNames = PushArray<SingleTypeStructElement>(out,amount);
        }

        int subIndex = indexes[pos]++;
        type->entries[index].typeAndNames[subIndex].name = elem.name;
        type->entries[index].typeAndNames[subIndex].type = PushString(out,"%.*sConfig",UNPACK_SS(elem.childStruct->name));
        type->entries[index].typeAndNames[subIndex].arraySize = 0;
      }

    }
  }
  
  Array<TypeStructInfo> res = PushArrayFromList(out,list);
  return res;
}
#endif


void OutputVersatSource(Accelerator* accel,const char* hardwarePath,const char* softwarePath,bool isSimple,Arena* temp,Arena* temp2){
  BLOCK_REGION(temp);
  BLOCK_REGION(temp2);
  
  ClearTemplateEngine(); // Make sure that we do not reuse data

  // No need for templating, small file
  FILE* c = OpenFileAndCreateDirectories(PushString(temp,"%s/versat_defs.vh",hardwarePath),"w",FilePurpose_VERILOG_INCLUDE);
  FILE* f = OpenFileAndCreateDirectories(PushString(temp,"%s/versat_undefs.vh",hardwarePath),"w",FilePurpose_VERILOG_INCLUDE);
  DEFER_CLOSE_FILE(c);
  DEFER_CLOSE_FILE(f);
  
  if(!c || !f){
    printf("Error creating file, check if filepath is correct: %s\n",hardwarePath);
    return;
  }

  AccelInfo info = CalculateAcceleratorInfo(accel,true,temp,temp2);

  Array<InstanceInfo*> topLevelUnits = GetAllSameLevelUnits(&info,0,0,temp);
  
  VersatComputedValues val = ComputeVersatValues(&info,false);
  CheckSanity(info.infos[0].info,temp);
  
  Array<Partition> partitions = GenerateInitialPartitions(accel,temp);

  AccelInfoIterator iter = StartIteration(&info);
  StructInfo* structInfo = GenerateConfigStruct(iter,partitions,accel->name,temp,temp2);
  
  Array<StructInfo*> allStructs = ExtractStructs(structInfo,temp,temp2);
  Array<TypeStructInfo> structs = GenerateStructs(allStructs,temp,temp2);

  // What is the best way of looking at the data?
  // For each struct, I need to see, for a given configPos, all the types and names used.
  // If only one, then direct type.
  // If multiple, then union.
  
  printf("ADDR_W - %d\n",val.memoryConfigDecisionBit + 1);
  if(val.nUnitsIO){
    printf("HAS_AXI - True\n");
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
  
  // Output configuration file
  //Array<FUInstance*> nodes = ListToArray(accel->allocated,temp2);
  Pool<FUInstance> nodes = accel->allocated;
  
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
  int index = 0;
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
    ProcessTemplate(f,BasicTemplates::externalInstTemplate,temp,temp2);
  }

  {
    FILE* f = OpenFileAndCreateDirectories(PushString(temp,"%s/versat_internal_memory_wires.vh",hardwarePath),"w",FilePurpose_VERILOG_INCLUDE);
    DEFER_CLOSE_FILE(f);
    ProcessTemplate(f,BasicTemplates::internalWiresTemplate,temp,temp2);
  }

  {
    FILE* f = OpenFileAndCreateDirectories(PushString(temp,"%s/versat_external_memory_port.vh",hardwarePath),"w",FilePurpose_VERILOG_INCLUDE);
    DEFER_CLOSE_FILE(f);
    ProcessTemplate(f,BasicTemplates::externalPortTemplate,temp,temp2);
  }

  {
    FILE* f = OpenFileAndCreateDirectories(PushString(temp,"%s/versat_external_memory_internal_portmap.vh",hardwarePath),"w",FilePurpose_VERILOG_INCLUDE);
    DEFER_CLOSE_FILE(f);
    ProcessTemplate(f,BasicTemplates::externalInternalPortmapTemplate,temp,temp2);
 }

  Hashmap<StaticId,StaticData>* staticUnits = CollectStaticUnits(accel,nullptr,temp);
  TemplateSetCustom("staticUnits",MakeValue(staticUnits));
  
  Array<String> memoryMasks = ExtractMemoryMasks(info,temp);

  Array<String> parameters = PushArray<String>(temp,nodes.Size());
  for(int i = 0; i < nodes.Size(); i++){
    parameters[i] = GenerateVerilogParameterization(nodes.Get(i),temp);
  }

  TemplateSetCustom("allUnits",MakeValue(&topLevelUnits));
  TemplateSetCustom("parameters",MakeValue(&parameters));
  TemplateSetCustom("arch",MakeValue(&globalOptions));
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
  TemplateSetNumber("configurationBits",val.configurationBits);
  TemplateSetNumber("versatConfig",val.versatConfigs);
  TemplateSetNumber("versatState",val.versatStates);
  TemplateSetNumber("memoryAddressBits",val.memoryAddressBits);
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
  
  {
    FILE* s = OpenFileAndCreateDirectories(PushString(temp,"%s/versat_instance.v",hardwarePath),"w",FilePurpose_VERILOG_CODE);
    DEFER_CLOSE_FILE(s);

    if(!s){
      printf("Error creating file, check if filepath is correct: %s\n",hardwarePath);
      return;
    }

    ProcessTemplate(s,BasicTemplates::topAcceleratorTemplate,temp,temp2);
  }
  
  {
    FILE* s = OpenFileAndCreateDirectories(PushString(temp,"%s/versat_configurations.v",hardwarePath),"w",FilePurpose_VERILOG_CODE);
    DEFER_CLOSE_FILE(s);

    if(!s){
      printf("Error creating file, check if filepath is correct: %s\n",hardwarePath);
      return;
    }

    ProcessTemplate(s,BasicTemplates::topConfigurationsTemplate,temp,temp2);
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

  TemplateSetNumber("memoryMappedBase",1 << val.memoryConfigDecisionBit);
  TemplateSetNumber("nConfigs",val.nConfigs);
  TemplateSetNumber("nStates",val.nStates);
  TemplateSetNumber("nStatics",val.nStatics);

  // MARK

#if 0
  Array<TypeStructInfo> configStructures = GetConfigStructInfo(accel,temp2,temp);
  TemplateSetCustom("configStructures",MakeValue(&configStructures));
#else
  TemplateSetCustom("configStructures",MakeValue(&structs));
#endif
  
  Array<TypeStructInfo> addressStructures = GetMemMappedStructInfo(accel,temp2,temp);
  TemplateSetCustom("addressStructures",MakeValue(&addressStructures));
  
  {
    DynamicArray<int> arr = StartArray<int>(temp);
    for(InstanceInfo& t : info.infos[0].info){
      if(!t.isComposite){
        for(int d : t.delay){
          *arr.PushElem() = d;
        }
      }
    }
    Array<int> delays = EndArray(arr);
    TemplateSetCustom("delay",MakeValue(&delays));

    Array<Array<int>> allDelays = PushArray<Array<int>>(temp,info.infos.size);
    if(info.infos.size >= 2){
      int i = 0;
      for(int ii = 0; ii <  info.infos.size; ii++){
        Array<InstanceInfo> allInfos = info.infos[ii].info;
        DynamicArray<int> arr = StartArray<int>(temp);
        for(InstanceInfo& t : allInfos){
          if(!t.isComposite){
            for(int d : t.delay){
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

    DynamicArray<DifferenceArray> diffArray = StartArray<DifferenceArray>(temp2);
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
    TemplateSetCustom("allStatics",MakeValue(&allStaticsVerilatorSide));
    
    Array<TypeStructInfoElement> structuredConfigs = ExtractStructuredConfigs(info.infos[0].info,temp,temp2);
    
    Array<String> allStates = ExtractStates(info.infos[0].info,temp2);
    Array<Pair<String,int>> allMem = ExtractMem(info.infos[0].info,temp2);
    
    TemplateSetNumber("delays",val.nDelays);
    TemplateSetCustom("structuredConfigs",MakeValue(&structuredConfigs));
    TemplateSetCustom("namedStates",MakeValue(&allStates));
    TemplateSetCustom("namedMem",MakeValue(&allMem));

    TemplateSetBool("outputChangeDelay",false);
    TemplateSetString("accelName",accel->name);
    
    Array<String> names = Map(info.infos,temp,[](MergePartition p){return p.name;});
    TemplateSetCustom("mergeNames",MakeValue(&names));

    AccelInfoIterator topIter = StartIteration(&info);

    for(int i = 0; i < topIter.MergeSize(); i++){
      topIter.mergeIndex = i;

      for(AccelInfoIterator iter = topIter; iter.IsValid(); iter = iter.Step()){
        InstanceInfo* info = iter.CurrentUnit();

        if(info->isMergeMultiplexer){
          printf("%.*s\n",UNPACK_SS(info->name));
        }
      }
    }
    
    Array<Array<MuxInfo>> result = GetAllMuxInfo(&info,temp,temp2);

    TemplateSetCustom("mergeMux",MakeValue(&result));

    GrowableArray<String> builder = StartGrowableArray<String>(temp2);
    for(AddressGenDef* def : savedAddressGen){
      String res = InstantiateAddressGen(*def,temp,temp2);
      *builder.PushElem() = res;
    }
    Array<String> content = EndArray(builder);

    TemplateSetCustom("addrGen",MakeValue(&content));
    
    FILE* f = OpenFileAndCreateDirectories(PushString(temp,"%s/versat_accel.h",softwarePath),"w",FilePurpose_SOFTWARE);
    DEFER_CLOSE_FILE(f);
    ProcessTemplate(f,BasicTemplates::acceleratorHeaderTemplate,temp,temp2);
  }
}

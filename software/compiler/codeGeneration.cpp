#include "codeGeneration.hpp"

#include "accelerator.hpp"
#include "declaration.hpp"
#include "logger.hpp"
#include "memory.hpp"
#include "utils.hpp"
#include "utilsCore.hpp"
#include "versat.hpp"
#include "templateData.hpp"
#include "templateEngine.hpp"
#include "textualRepresentation.hpp"
#include "globals.hpp"

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

  for(Pair<FUDeclaration*,bool> p : seen){
    Assert(p.second);
  }

  return result;
}

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

  for(Pair<FUDeclaration*,bool> p : seen){
    Assert(p.second);
  }

  return result;
}

Array<TypeStructInfoElement> GenerateStructFromType(FUDeclaration* decl,Arena* out,Arena* temp){
  if(decl->type == FUDeclarationType_SPECIAL){
    return {};
  }

  if(decl->type == FUDeclarationType_SINGLE){
    int size = decl->baseConfig.configs.size;
    Array<TypeStructInfoElement> entries = PushArray<TypeStructInfoElement>(out,size);

    for(int i = 0; i < size; i++){
      entries[i].typeAndNames = PushArray<SingleTypeStructElement>(out,1);
      entries[i].typeAndNames[0].type = STRING("iptr");
      entries[i].typeAndNames[0].name = decl->baseConfig.configs[i].name;
    }
    return entries;
  }

  BLOCK_REGION(temp);

  CalculatedOffsets& offsets = decl->baseConfig.configOffsets;
  Accelerator* accel = decl->fixedDelayCircuit;
  int configSize = offsets.max;

  Array<int> configAmount = PushArray<int>(temp,configSize);
  Memset(configAmount,0);
  int i = 0;
  FOREACH_LIST_INDEXED(InstanceNode*,node,accel->allocated,i){
    FUDeclaration* decl = node->inst->declaration;
    
    int configOffset = offsets.offsets[i];
    if(configOffset < 0 || configOffset >= 0x40000000){
      continue;
    }

    int numberConfigs = decl->baseConfig.configs.size;
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
  Memset(configSeen,0);
  i = 0;
  int index = 0;
  FOREACH_LIST_INDEXED(InstanceNode*,node,accel->allocated,i){
    FUDeclaration* decl = node->inst->declaration;

    int configOffset = offsets.offsets[i];
    if(configOffset < 0 || configOffset >= 0x40000000){
      continue;
    }

    Assert(offsets.offsets[i] >= 0);

    int index = mapOffsetToIndex->GetOrFail(configOffset);
    entries[index].typeAndNames[configSeen[index]].type = PushString(out,"%.*sConfig",UNPACK_SS(decl->name));
    entries[index].typeAndNames[configSeen[index]].name = node->inst->name;
    configSeen[index] += 1;
  }

  return entries;
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
  FOREACH_LIST(InstanceNode*,node,decl->fixedDelayCircuit->allocated){
    FUDeclaration* decl = node->inst->declaration;

    if(!(decl->memoryMapBits.has_value())){
      continue;
    }

    memoryMapped += 1;
  }

  Array<TypeStructInfoElement> entries = PushArray<TypeStructInfoElement>(out,memoryMapped);

  int i = 0;
  int index = 0;
  FOREACH_LIST_INDEXED(InstanceNode*,node,decl->fixedDelayCircuit->allocated,i){
    FUDeclaration* decl = node->inst->declaration;

    if(!(decl->memoryMapBits.has_value())){
      continue;
    }

    entries[index].typeAndNames = PushArray<SingleTypeStructElement>(out,1);
    entries[index].typeAndNames[0].type = PushString(out,"%.*sAddr",UNPACK_SS(decl->name));
    entries[index].typeAndNames[0].name = node->inst->name;
    index += 1;
  }
  return entries;
}

struct SubTypesInfo{
  FUDeclaration* type;
  FUDeclaration* mergeTop;
  ConfigurationInfo* info;
  bool isFromMerged;
  bool containsMerged;
};

template<> struct std::hash<SubTypesInfo>{
   std::size_t operator()(SubTypesInfo const& s) const noexcept{
     std::size_t res = std::hash<void*>()(s.type) + std::hash<void*>()(s.mergeTop) + std::hash<bool>()(s.isFromMerged) + std::hash<void*>()(s.info) + std::hash<bool>()(s.containsMerged);
     return res;
   }
};

bool operator==(const SubTypesInfo i0,const SubTypesInfo i1){
  bool res = Memcmp(&i0,&i1,1);
  return res;
}

Array<SubTypesInfo> GetSubTypesInfo(Accelerator* accel,Arena* out,Arena* temp){
  BLOCK_REGION(temp);
  
  Array<FUDeclaration*> typesUsed = ConfigSubTypes(accel,out,temp);
  typesUsed = SortTypesByConfigDependency(typesUsed,temp,out);

  Set<SubTypesInfo>* info = PushSet<SubTypesInfo>(temp,99);
  
  for(FUDeclaration* type : typesUsed){
    if(type->type == FUDeclarationType_MERGED){
      for(int i = 0; i < type->configInfo.size; i++){
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

Array<TypeStructInfo> GetConfigStructInfo(Accelerator* accel,Arena* out,Arena* temp){
  BLOCK_REGION(temp);

  GetSubTypesInfo(accel,out,temp);
  Array<FUDeclaration*> typesUsed = ConfigSubTypes(accel,out,temp);
  typesUsed = SortTypesByConfigDependency(typesUsed,temp,out);

  Array<SubTypesInfo> subTypesInfo =  GetSubTypesInfo(accel,out,temp);
  
  Array<TypeStructInfo> structures = PushArray<TypeStructInfo>(out,typesUsed.size + 99); // TODO: Small hack to handle merge for now.
  Memset(structures,{});
  int index = 0;
  for(SubTypesInfo subType : subTypesInfo){
    if(subType.isFromMerged){
      FUDeclaration* decl = subType.mergeTop;
      int maxOffset = decl->baseConfig.configOffsets.max; // TODO: This should be equal for all merged types, no need to duplicate data
      ConfigurationInfo& info = *subType.info;
      CalculatedOffsets& offsets = info.configOffsets;
        
      Array<bool> seenIndex = PushArray<bool>(temp,offsets.max);
      Memset(seenIndex,false);
        
      int i = 0;
      FOREACH_LIST_INDEXED(InstanceNode*,node,decl->fixedDelayCircuit->allocated,i){
        int config = offsets.offsets[i];
          
        if(config >= 0 && info.unitBelongs[i]){
          int nConfigs = node->inst->declaration->baseConfig.configs.size;
          for(int ii = 0; ii < nConfigs; ii++){
            seenIndex[config + ii] = true;
          }
        }
      }
        
      ArenaList<TypeStructInfoElement>* list = PushArenaList<TypeStructInfoElement>(temp);
        
      int unused = 0;
      int configNeedToSee = 0;
      while(1){
        while(configNeedToSee < maxOffset && seenIndex[configNeedToSee] == false){
          TypeStructInfoElement* elem = PushListElement(list);
          elem->typeAndNames = PushArray<SingleTypeStructElement>(out,1);
          elem->typeAndNames[0].type = STRING("iptr");
          elem->typeAndNames[0].name = PushString(out,"unused%d",unused++);
          configNeedToSee += 1;
        }

        if(configNeedToSee >= maxOffset){
          break;
        }

        int i = 0;
        FOREACH_LIST_INDEXED(InstanceNode*,node,decl->fixedDelayCircuit->allocated,i){
          int config = offsets.offsets[i];
          if(configNeedToSee == config){
            int nConfigs = node->inst->declaration->baseConfig.configs.size;
              
            TypeStructInfoElement* elem = PushListElement(list);
            elem->typeAndNames = PushArray<SingleTypeStructElement>(out,1);
            elem->typeAndNames[0].type = PushString(out,"%.*sConfig",UNPACK_SS(node->inst->declaration->name));
            elem->typeAndNames[0].name = info.baseName[i]; //node->inst->name;
              
            configNeedToSee += nConfigs;
          }
        }
      }

      Array<TypeStructInfoElement> elem = PushArrayFromList(out,list);
      structures[index].name = PushString(out,"%.*s",UNPACK_SS(info.name));
      structures[index].entries = elem;
      index += 1;
    } else if(subType.containsMerged){
      FUDeclaration* decl = subType.type;

      Array<TypeStructInfoElement> merged = PushArray<TypeStructInfoElement>(out,1);

      merged[0].typeAndNames = PushArray<SingleTypeStructElement>(out,decl->configInfo.size);
      for(int i = 0; i < decl->configInfo.size; i++){
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

Array<MemoryAddressMask> CalculateAcceleratorMemoryMasks(AccelInfo info,Arena* out){
  DynamicArray<MemoryAddressMask> arr = StartArray<MemoryAddressMask>(out);
  for(InstanceInfo& in : info.infos[0]){
    if(in.level == 0 && in.memMapped.has_value()){
      MemoryAddressMask* data = arr.PushElem();
      *data = {};
      data->memoryMaskSize = info.memMappedBitsize - in.memMappedBitSize.value();
      data->memoryMask = data->memoryMaskBuffer;

      for(int i = 0; i < data->memoryMaskSize; i++){
        data->memoryMaskBuffer[i] = in.memMappedMask.value()[i];
      }
    }
  }
  return EndArray(arr);
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
      String name = PushString(out,"%.*s_%.*s",UNPACK_SS(in.fullName),UNPACK_SS(in.decl->baseConfig.configs[i].name));

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

void OutputCircuitSource(FUDeclaration* decl,Accelerator* accel,FILE* file,Arena* temp,Arena* temp2){
  BLOCK_REGION(temp);
  BLOCK_REGION(temp2);
  
  ClearTemplateEngine(); // Make sure that we do not reuse data
  
  Assert(globalDebug.outputAccelerator); // Because FILE is created outside, code should not call this function if flag is set
  
  Array<InstanceNode*> nodes = ListToArray(accel->allocated,temp);
  for(InstanceNode* node : nodes){
    if(node->inst->declaration->nIOs){
      node->inst->parameters = STRING("#(.AXI_ADDR_W(AXI_ADDR_W),.AXI_DATA_W(AXI_DATA_W),.LEN_W(LEN_W))"); // TODO: placeholder hack.
    }
  }

  VersatComputedValues val = ComputeVersatValues(accel,false);
  AccelInfo info = CalculateAcceleratorInfoNoDelay(accel,true,temp,temp2);
  Array<MemoryAddressMask> memoryMasks = CalculateAcceleratorMemoryMasks(info,temp);

  TemplateSetNumber("unitsMapped",val.unitsMapped);
  TemplateSetNumber("numberConnections",info.numberConnections);

  TemplateSetCustom("inputDecl",MakeValue(BasicDeclaration::input));
  TemplateSetCustom("outputDecl",MakeValue(BasicDeclaration::output));
  TemplateSetCustom("arch",MakeValue(&globalOptions));
  TemplateSetCustom("accel",MakeValue(decl));
  TemplateSetCustom("memoryMasks",MakeValue(&memoryMasks));
 
  TemplateSetCustom("instances",MakeValue(&nodes));

  ProcessTemplate(file,BasicTemplates::acceleratorTemplate,temp,temp2);
}

void OutputIterativeSource(FUDeclaration* decl,Accelerator* accel,FILE* file,Arena* temp,Arena* temp2){
  BLOCK_REGION(temp);
  BLOCK_REGION(temp2);

  ClearTemplateEngine(); // Make sure that we do not reuse data

  Assert(globalDebug.outputAccelerator); // Because FILE is created outside, code should not call this function if flag is set

  // MARKED
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    FUInstance* inst = ptr->inst;
    if(inst->declaration->nIOs){
      inst->parameters = STRING("#(.AXI_ADDR_W(AXI_ADDR_W),.AXI_DATA_W(AXI_DATA_W),.LEN_W(LEN_W))"); // TODO: placeholder hack.
    }
  }
   
  VersatComputedValues val = ComputeVersatValues(accel,false);
  AccelInfo info = CalculateAcceleratorInfo(accel,true,temp,temp2); // TODO: Calculating info just for the computedData calculation is a bit wasteful.
  Array<MemoryAddressMask> memoryMasks = CalculateAcceleratorMemoryMasks(info,temp);

  TemplateSetCustom("staticUnits",MakeValue(decl->staticUnits));
  TemplateSetCustom("accel",MakeValue(decl));
  TemplateSetCustom("memoryMasks",MakeValue(&memoryMasks));
  TemplateSetCustom("instances",MakeValue(accel->allocated));
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
  Array<Wire> allConfigsHeaderSide = ExtractAllConfigs(info.baseInfo,temp,temp2);

#if 1
  // We need to bundle config + static (type->config) only contains config, but not static
  Array<Wire> allConfigsVerilatorSide = PushArray<Wire>(temp,999); // TODO: Correct size
  {
    int index = 0;
    for(Wire& config : type->baseConfig.configs){
      allConfigsVerilatorSide[index++] = config;
    }
    allConfigsVerilatorSide.size = index;
  }
  TemplateSetCustom("allConfigsVerilatorSide",MakeValue(&allConfigsVerilatorSide));

  int index = 0;
  Array<Wire> allStaticsVerilatorSide = PushArray<Wire>(temp,999); // TODO: Correct size
  for(Pair<StaticId,StaticData> p : type->staticUnits){
    for(Wire& config : p.second.configs){
      allStaticsVerilatorSide[index] = config;
      allStaticsVerilatorSide[index].name = ReprStaticConfig(p.first,&config,temp);
      index += 1;
    }
  }
  allStaticsVerilatorSide.size = index;
  TemplateSetCustom("allStaticsVerilatorSide",MakeValue(&allStaticsVerilatorSide));
#endif

  //UnitValues val = CalculateAcceleratorValues(versat,accel);
  Array<TypeStructInfoElement> structuredConfigs = ExtractStructuredConfigs(info.baseInfo,temp,temp2);

  TemplateSetNumber("delays",info.delays);
  TemplateSetCustom("structuredConfigs",MakeValue(&structuredConfigs));
  Array<String> statesHeaderSide = ExtractStates(info.baseInfo,temp);
  
  int totalExternalMemory = ExternalMemoryByteSize(type->externalMemory);

  TemplateSetCustom("configsHeader",MakeValue(&allConfigsHeaderSide));
  TemplateSetCustom("statesHeader",MakeValue(&statesHeaderSide));
  TemplateSetNumber("totalExternalMemory",totalExternalMemory);
  TemplateSetCustom("opts",MakeValue(&globalOptions));
  TemplateSetCustom("type",MakeValue(type));
  TemplateSetBool("trace",globalDebug.outputVCD);
  
  FILE* output = OpenFileAndCreateDirectories(StaticFormat("%.*s/wrapper.cpp",UNPACK_SS(outputPath)),"w");
  CompiledTemplate* templ = CompileTemplate(versat_wrapper_template,"wrapper",temp,temp2);
  ProcessTemplate(output,templ,temp,temp2);
  fclose(output);
}

#include <filesystem>
namespace fs = std::filesystem;

void OutputVerilatorMake(String topLevelName,String versatDir,Arena* temp,Arena* temp2){
  BLOCK_REGION(temp);
  BLOCK_REGION(temp2);
  
  String outputPath = globalOptions.softwareOutputFilepath;
  FILE* output = OpenFileAndCreateDirectories(StaticFormat("%.*s/VerilatorMake.mk",UNPACK_SS(outputPath)),"w");
  
  TemplateSetBool("traceEnabled",globalDebug.outputVCD);
  CompiledTemplate* comp = CompileTemplate(versat_makefile_template,"makefile",temp,temp2);
    
  fs::path outputFSPath = StaticFormat("%.*s",UNPACK_SS(outputPath));
  fs::path srcLocation = fs::current_path();
  fs::path fixedPath = fs::weakly_canonical(outputFSPath / srcLocation);

  String srcDir = PushString(temp,"%.*s/src",UNPACK_SS(outputPath));
    
  TemplateSetCustom("arch",MakeValue(&globalOptions));
  TemplateSetString("srcDir",srcDir);
  TemplateSetString("versatDir",versatDir);
  TemplateSetString("verilatorRoot",globalOptions.verilatorRoot);
  TemplateSetNumber("bitWidth",globalOptions.addrSize);
  TemplateSetString("generatedUnitsLocation",globalOptions.hardwareOutputFilepath);
  TemplateSetArray("verilogFiles","String",UNPACK_SS(globalOptions.verilogFiles));
  TemplateSetArray("extraSources","String",UNPACK_SS(globalOptions.extraSources));
  TemplateSetArray("includePaths","String",UNPACK_SS(globalOptions.includePaths));
  TemplateSetString("typename",topLevelName);
  TemplateSetString("rootPath",STRING(fixedPath.c_str()));
  ProcessTemplate(output,comp,temp,temp2);
}

void OutputVersatSource(Accelerator* accel,const char* hardwarePath,const char* softwarePath,bool isSimple,Arena* temp,Arena* temp2){
  BLOCK_REGION(temp);
  BLOCK_REGION(temp2);
  
  ClearTemplateEngine(); // Make sure that we do not reuse data

  // No need for templating, small file
  FILE* c = OpenFileAndCreateDirectories(StaticFormat("%s/versat_defs.vh",hardwarePath),"w");
  FILE* f = OpenFileAndCreateDirectories(StaticFormat("%s/versat_undefs.vh",hardwarePath),"w");

  if(!c || !f){
    printf("Error creating file, check if filepath is correct: %s\n",hardwarePath);
    return;
  }

  VersatComputedValues val = ComputeVersatValues(accel,globalOptions.useDMA);
  AccelInfo info = CalculateAcceleratorInfo(accel,true,temp,temp2);
  CheckSanity(info.baseInfo,temp);

  //UnitValues unit = CalculateAcceleratorValues(versat,accel);
  
  printf("ADDR_W - %d\n",val.memoryConfigDecisionBit + 1);
  if(val.nUnitsIO){
    printf("HAS_AXI - True\n");
  }

  fprintf(c,"`define NUMBER_UNITS %d\n",Size(accel->allocated));
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
  
  fclose(c);
  fclose(f);

  // Output configuration file
  Array<InstanceNode*> nodes = ListToArray(accel->allocated,temp2);

  //ReorganizeAccelerator(accel,temp2); // TODO: Reorganize accelerator needs to go. Just call CalculateOrder when needed. Need to check how iterative work for this case, though
  DAGOrderNodes order = CalculateDAGOrder(accel->allocated,temp);
  Array<InstanceNode*> ordered = order.instances;
  
  for(InstanceNode* node : order.instances){
    if(node->inst->declaration->nIOs){
      node->inst->parameters = STRING("#(.AXI_ADDR_W(AXI_ADDR_W),.AXI_DATA_W(AXI_DATA_W),.LEN_W(LEN_W))"); // TODO: placeholder hack.
    }
  }

  Array<ExternalMemoryInterface> external = PushArray<ExternalMemoryInterface>(temp,val.externalMemoryInterfaces);
  int index = 0;
  int externalIndex = 0;
  for(InstanceInfo& in : info.baseInfo){
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
      default: NOT_IMPLEMENTED("Implement as needed");
      }
    }
  }

  int staticStart = 0;
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    FUDeclaration* decl = ptr->inst->declaration;
    for(Wire& wire : decl->baseConfig.configs){
      staticStart += wire.bitSize;
    }
  }

  // All dependent on external
  TemplateSetCustom("external",MakeValue(&external));
  {
    FILE* f = OpenFileAndCreateDirectories(StaticFormat("%s/versat_external_memory_inst.vh",hardwarePath),"w");
    ProcessTemplate(f,BasicTemplates::externalInstTemplate,temp,temp2);
    fclose(f);
  }

  {
    FILE* f = OpenFileAndCreateDirectories(StaticFormat("%s/versat_internal_memory_wires.vh",hardwarePath),"w");
    ProcessTemplate(f,BasicTemplates::internalWiresTemplate,temp,temp2);
    fclose(f);
  }

  {
    FILE* f = OpenFileAndCreateDirectories(StaticFormat("%s/versat_external_memory_port.vh",hardwarePath),"w");
    ProcessTemplate(f,BasicTemplates::externalPortTemplate,temp,temp2);
    fclose(f);
  }

  {
    FILE* f = OpenFileAndCreateDirectories(StaticFormat("%s/versat_external_memory_internal_portmap.vh",hardwarePath),"w");
    ProcessTemplate(f,BasicTemplates::externalInternalPortmapTemplate,temp,temp2);
    fclose(f);
  }
  
  Hashmap<StaticId,StaticData>* staticUnits = PushHashmap<StaticId,StaticData>(temp,val.nStatics);
  int staticIndex = 0; // staticStart; TODO: For now, test with static info beginning at zero
  for(InstanceInfo& info : info.baseInfo){
    FUDeclaration* decl = info.decl;
    if(info.isStatic){
      FUDeclaration* parentDecl = info.parent;

      StaticId id = {};
      id.name = info.name;
      id.parent = parentDecl;

      GetOrAllocateResult res = staticUnits->GetOrAllocate(id);

      if(res.alreadyExisted){
      } else {
        StaticData data = {};
        data.offset = staticIndex;
        data.configs = decl->baseConfig.configs;

        *res.data = data;
        staticIndex += decl->baseConfig.configs.size;
      }
    }
  }

  TemplateSetCustom("staticUnits",MakeValue(staticUnits));
  
  Array<MemoryAddressMask> memoryMasks = CalculateAcceleratorMemoryMasks(info,temp);

  TemplateSetCustom("versatValues",MakeValue(&val));
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
  TemplateSetBool("useDMA",globalOptions.useDMA);
  TemplateSetCustom("opts",MakeValue(&globalOptions));
  
  {
    FILE* s = OpenFileAndCreateDirectories(StaticFormat("%s/versat_instance.v",hardwarePath),"w");

    if(!s){
      printf("Error creating file, check if filepath is correct: %s\n",hardwarePath);
      return;
    }

    ProcessTemplate(s,BasicTemplates::topAcceleratorTemplate,temp,temp2);
    fclose(s);
  }
  
  TemplateSetBool("isSimple",isSimple);
  if(isSimple){
    FUInstance* inst = nullptr; // TODO: Should probably separate isSimple to a separate function, because otherwise we are recalculating stuff that we already know.
    FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
      if(CompareString(ptr->inst->name,STRING("TOP"))){
        inst = ptr->inst;
        break;
      }
    }
    Assert(inst);

    Accelerator* accel = inst->declaration->fixedDelayCircuit;
    FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
      if(CompareString(ptr->inst->name,STRING("simple"))){
        inst = ptr->inst;
        break;
      }
    }
    Assert(inst);

    TemplateSetNumber("simpleInputs",inst->declaration->NumberInputs());
    TemplateSetNumber("simpleOutputs",inst->declaration->NumberOutputs());
  }

  TemplateSetNumber("memoryMappedBase",1 << val.memoryConfigDecisionBit);
  TemplateSetNumber("nConfigs",val.nConfigs);
  TemplateSetNumber("nStatics",val.nStatics);

  Array<TypeStructInfo> configStructures = GetConfigStructInfo(accel,temp2,temp);
  TemplateSetCustom("configStructures",MakeValue(&configStructures));
  
  Array<TypeStructInfo> addressStructures = GetMemMappedStructInfo(accel,temp2,temp);
  TemplateSetCustom("addressStructures",MakeValue(&addressStructures));
  
  {
    DynamicArray<int> arr = StartArray<int>(temp);
    for(InstanceInfo& t : info.baseInfo){
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
      for(Array<InstanceInfo> allInfos : info.infos){
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
    
#if 1
    int index = 0;
    Array<Wire> allStaticsVerilatorSide = PushArray<Wire>(temp,999); // TODO: Correct size
    for(Pair<StaticId,StaticData> p : staticUnits){
      for(Wire& config : p.second.configs){
        allStaticsVerilatorSide[index] = config;
        allStaticsVerilatorSide[index].name = ReprStaticConfig(p.first,&config,temp);
        index += 1;
      }
    }
    allStaticsVerilatorSide.size = index;
    TemplateSetCustom("allStatics",MakeValue(&allStaticsVerilatorSide));
#endif

    Array<TypeStructInfoElement> structuredConfigs = ExtractStructuredConfigs(info.baseInfo,temp,temp2);

    Array<String> allStates = ExtractStates(info.baseInfo,temp2);
    Array<Pair<String,int>> allMem = ExtractMem(info.baseInfo,temp2);

    TemplateSetNumber("delays",val.nDelays);
    TemplateSetCustom("structuredConfigs",MakeValue(&structuredConfigs));
    TemplateSetCustom("namedStates",MakeValue(&allStates));
    TemplateSetCustom("namedMem",MakeValue(&allMem));

    TemplateSetString("accelName",accel->name);
    TemplateSetString("mergeMuxName",STRING(""));
    
    for(InstanceInfo in : info.baseInfo){
      if(in.isMergeMultiplexer){
        String str = PushString(temp,"ACCEL_%.*s_%.*s",UNPACK_SS(in.fullName),UNPACK_SS(in.decl->baseConfig.configs[0].name));
        TemplateSetString("mergeMuxName",str);
      }
    }

    TemplateSetCustom("mergeNames",MakeValue(&info.names));

    FILE* f = OpenFileAndCreateDirectories(StaticFormat("%s/versat_accel.h",softwarePath),"w");
    ProcessTemplate(f,BasicTemplates::acceleratorHeaderTemplate,temp,temp2);
    fclose(f);
  }
}

#include "configurations.hpp"
#include "debugVersat.hpp"
#include "declaration.hpp"
#include "memory.hpp"
#include "type.hpp"
#include "utils.hpp"
#include "utilsCore.hpp"
#include "verilogParsing.hpp"
#include "versat.hpp"
#include "textualRepresentation.hpp"
#include "acceleratorStats.hpp"
#include "templateData.hpp"

#include "templateEngine.hpp"

#include "debug.hpp"

static Hashmap<Type*,Array<bool>>* fieldsPerTypeSeen = nullptr;
static Arena testInst = {};
static Arena* testArena = &testInst;

static void CheckUnusedFieldsOrTypes(CompiledTemplate* tpl,Arena* temp,Arena* temp2){
  if(fieldsPerTypeSeen == nullptr){
    testInst = InitArena(Megabyte(1));
    fieldsPerTypeSeen = PushHashmap<Type*,Array<bool>>(testArena,99);
  }
  
  BLOCK_REGION(temp);
  BLOCK_REGION(temp2);
  Array<TemplateRecord> records = RecordTypesAndFieldsUsed(tpl,temp,temp2);

  for(TemplateRecord& r : records){
    if(r.type == TemplateRecordType_FIELD){
      Type* structType = r.structType;
      GetOrAllocateResult res = fieldsPerTypeSeen->GetOrAllocate(structType);

      Array<bool>* arr = res.data;
      if(!res.alreadyExisted){
        *arr = PushArray<bool>(testArena,structType->members.size);
        Memset(*arr,false);
      }

      for(int i = 0; i < structType->members.size; i++){
        Member m = structType->members[i];
        if(CompareString(m.name,r.fieldName)){
          (*arr)[i] = true;
          break;
        }
      }
    }
  }

  for(Pair<Type*,Array<bool>> p : fieldsPerTypeSeen){
    Type* structType = p.first;
    Array<bool>& arr = p.second;

    printf("%.*s\n",UNPACK_SS(structType->name));
    for(int i = 0; i < arr.size; i++){
      Member m = structType->members[i];
      if(arr[i]){
        printf("  Used: %.*s\n",UNPACK_SS(m.name));
      } else {
        printf("  Not:  %.*s\n",UNPACK_SS(m.name));
      }        
    }
  }
}

static void CheckIfTemplateUsesAllValues(CompiledTemplate* tpl,Arena* temp,Arena* temp2){
  BLOCK_REGION(temp);
  BLOCK_REGION(temp2);
  Array<TemplateRecord> records = RecordTypesAndFieldsUsed(tpl,temp,temp2);

  Hashmap<String,Value>* valuesUsed = GetAllTemplateValues();
  Hashmap<String,bool>* seen = PushHashmap<String,bool>(temp2,valuesUsed->nodesUsed);

  for(Pair<String,Value> p : valuesUsed){
    seen->Insert(p.first,false);
  }
  
  for(TemplateRecord r : records){
    if(r.type == TemplateRecordType_IDENTIFER){
      seen->Insert(r.identifierName,true);
    }
  }

  for(Pair<String,bool> p : seen){
    if(p.second){
      printf("Seen: %.*s\n",UNPACK_SS(p.first));
    }
    if(!p.second){
      printf("Not : %.*s\n",UNPACK_SS(p.first));
    }
  }
}

// TODO: Can reuse SortTypes functions by receiving the Array of Arrays as an argument. Useful for memory and probably state and so on.
static Array<FUDeclaration*> SortTypesByConfigDependency(Array<FUDeclaration*> types,Arena* out,Arena* temp){
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

static Array<FUDeclaration*> SortTypesByMemDependency(Array<FUDeclaration*> types,Arena* out,Arena* temp){
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

static Array<TypeStructInfoElement> GenerateStructFromType(FUDeclaration* decl,Arena* out,Arena* temp){
  if(decl->type == FUDeclaration::SPECIAL){
    return {};
  }

  if(decl->type == FUDeclaration::SINGLE){
    int size = decl->configInfo.configs.size;
    Array<TypeStructInfoElement> entries = PushArray<TypeStructInfoElement>(out,size);

    for(int i = 0; i < size; i++){
      entries[i].typeAndNames = PushArray<SingleTypeStructElement>(out,1);
      entries[i].typeAndNames[0].type = STRING("iptr");
      entries[i].typeAndNames[0].name = decl->configInfo.configs[i].name;
    }
    return entries;
  }

  BLOCK_REGION(temp);

  CalculatedOffsets& offsets = decl->configInfo.configOffsets;
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

    int numberConfigs = decl->configInfo.configs.size;
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
  if(decl->type == FUDeclaration::SINGLE){
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
  FOREACH_LIST(InstanceNode*,node,decl->baseCircuit->allocated){
    FUDeclaration* decl = node->inst->declaration;

    if(!(decl->memoryMapBits.has_value())){
      continue;
    }

    memoryMapped += 1;
  }

  Array<TypeStructInfoElement> entries = PushArray<TypeStructInfoElement>(out,memoryMapped);

  int i = 0;
  int index = 0;
  FOREACH_LIST_INDEXED(InstanceNode*,node,decl->baseCircuit->allocated,i){
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

Array<TypeStructInfo> GetConfigStructInfo(Accelerator* accel,Arena* out,Arena* temp){
  BLOCK_REGION(temp);

  Array<FUDeclaration*> typesUsed = ConfigSubTypes(accel,out,temp);
  typesUsed = SortTypesByConfigDependency(typesUsed,temp,out);

  Array<TypeStructInfo> structures = PushArray<TypeStructInfo>(out,typesUsed.size + 3); // TODO: Small hack to handle merge for now.
  int index = 0;
  for(auto& decl : typesUsed){
    if(decl->mergeInfo.size){
      CalculatedOffsets& mergedOffsets = decl->configInfo.configOffsets;
      int maxOffset = mergedOffsets.max;
      for(MergeInfo& info : decl->mergeInfo){
        CalculatedOffsets& offsets = info.config.configOffsets;
        
        Array<bool> seenIndex = PushArray<bool>(temp,offsets.max);
        Memset(seenIndex,false);
        
        int i = 0;
        FOREACH_LIST_INDEXED(InstanceNode*,node,decl->fixedDelayCircuit->allocated,i){
          int config = offsets.offsets[i];
          
          if(config >= 0){
            int nConfigs = node->inst->declaration->configInfo.configs.size;
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
              int nConfigs = node->inst->declaration->configInfo.configs.size;
              
              TypeStructInfoElement* elem = PushListElement(list);
              elem->typeAndNames = PushArray<SingleTypeStructElement>(out,1);
              elem->typeAndNames[0].type = PushString(out,"%.*sConfig",UNPACK_SS(node->inst->declaration->name));
              elem->typeAndNames[0].name = info.baseName[i]; //node->inst->name;
              
              configNeedToSee += nConfigs;
            }
          }
        }

        Array<TypeStructInfoElement> elem = PushArrayFromList(out,list);
        structures[index].name = info.name;
        structures[index].entries = elem;
        index += 1;
      }

      Array<TypeStructInfoElement> merged = PushArray<TypeStructInfoElement>(out,1);

      merged[0].typeAndNames = PushArray<SingleTypeStructElement>(out,decl->mergeInfo.size);
      for(int i = 0; i < decl->mergeInfo.size; i++){
        merged[0].typeAndNames[i].type = PushString(out,"%.*sConfig",UNPACK_SS(decl->mergeInfo[i].name));
        merged[0].typeAndNames[i].name = decl->mergeInfo[i].name;
      }

      structures[index].name = PushString(out,"%.*s",UNPACK_SS(decl->name));
      structures[index].entries = merged;
      index += 1;
    } else {
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

void OutputCircuitSource(Versat* versat,FUDeclaration* decl,Accelerator* accel,FILE* file,Arena* temp,Arena* temp2){
  BLOCK_REGION(temp);
  BLOCK_REGION(temp2);

  ClearTemplateEngine(); // Make sure that we do not reuse data
  
  Assert(versat->debug.outputAccelerator); // Because FILE is created outside, code should not call this function if flag is set

  VersatComputedValues val = ComputeVersatValues(versat,accel,false);

  int size = Size(accel->allocated);
  Array<InstanceNode*> nodes = ListToArray(accel->allocated,size,temp);

  Array<InstanceNode*> ordered = PushArray<InstanceNode*>(temp,size);
  int i = 0;
  FOREACH_LIST_INDEXED(OrderedInstance*,ptr,accel->ordered,i){
    ordered[i] = ptr->node;

    FUInstance* inst = ptr->node->inst;
    if(inst->declaration->nIOs){
      inst->parameters = STRING("#(.AXI_ADDR_W(AXI_ADDR_W),.AXI_DATA_W(AXI_DATA_W),.LEN_W(LEN_W))"); // TODO: placeholder hack.
    }
  }

  {
    iptr memoryPos = 0;
    FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
      FUInstance* inst = ptr->inst;
      FUDeclaration* decl = inst->declaration;

      if(decl->memoryMapBits.has_value()) {
        memoryPos = AlignBitBoundary(memoryPos,decl->memoryMapBits.value());
        inst->memMapped = (int*) memoryPos;
        memoryPos += 1 << decl->memoryMapBits.value();
      }
    }
  }

  Array<InstanceInfo> info = TransformGraphIntoArray(accel,true,temp,temp2); // TODO: Calculating info just for the computedData calculation is a bit wasteful.
  ComputedData computedData = CalculateVersatComputedData(info,val,temp);

  TemplateSetCustom("inputDecl",MakeValue(BasicDeclaration::input));
  TemplateSetCustom("outputDecl",MakeValue(BasicDeclaration::output));
  TemplateSetCustom("arch",MakeValue(&versat->opts));
  TemplateSetCustom("accel",MakeValue(decl));
  TemplateSetCustom("versatValues",MakeValue(&val));
  TemplateSetCustom("versatData",MakeValue(&computedData.data));
  TemplateSetCustom("external",MakeValue(&computedData.external));
  TemplateSetCustom("instances",MakeValue(&nodes));
  TemplateSetCustom("ordered",MakeValue(&ordered));
  TemplateSetNumber("unitsMapped",val.unitsMapped);
  TemplateSetNumber("memoryAddressBits",val.memoryAddressBits);

  ProcessTemplate(file,BasicTemplates::acceleratorTemplate,temp,temp2);
}

void OutputIterativeSource(Versat* versat,FUDeclaration* decl,Accelerator* accel,FILE* file,Arena* temp,Arena* temp2){
  BLOCK_REGION(temp);
  BLOCK_REGION(temp2);

  ClearTemplateEngine(); // Make sure that we do not reuse data

  Assert(versat->debug.outputAccelerator); // Because FILE is created outside, code should not call this function if flag is set

  VersatComputedValues val = ComputeVersatValues(versat,accel,false);

  int size = Size(accel->allocated);
  Array<InstanceNode*> nodes = ListToArray(accel->allocated,size,temp);

  Array<InstanceNode*> ordered = PushArray<InstanceNode*>(temp,size);
  int i = 0;
  FOREACH_LIST_INDEXED(OrderedInstance*,ptr,accel->ordered,i){
    ordered[i] = ptr->node;

    FUInstance* inst = ptr->node->inst;
    if(inst->declaration->nIOs){
      inst->parameters = STRING("#(.AXI_ADDR_W(AXI_ADDR_W),.AXI_DATA_W(AXI_DATA_W),.LEN_W(LEN_W))"); // TODO: placeholder hack.
    }
  }

  {
    iptr memoryPos = 0;
    FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
      FUInstance* inst = ptr->inst;
      FUDeclaration* decl = inst->declaration;

      if(decl->memoryMapBits.has_value()) {
        memoryPos = AlignBitBoundary(memoryPos,decl->memoryMapBits.value());
        inst->memMapped = (int*) memoryPos;
        memoryPos += 1 << decl->memoryMapBits.value();
      }
    }
  }

  Array<InstanceInfo> info = TransformGraphIntoArray(accel,true,temp,temp2); // TODO: Calculating info just for the computedData calculation is a bit wasteful.
  ComputedData computedData = CalculateVersatComputedData(info,val,temp);
  //ComputedData computedData = CalculateVersatComputedData(accel->allocated,val,temp);
  TemplateSetCustom("staticUnits",MakeValue(decl->staticUnits));
  TemplateSetCustom("accel",MakeValue(decl));
  TemplateSetCustom("versatData",MakeValue(&computedData.data));
  TemplateSetCustom("external",MakeValue(&computedData.external));
  TemplateSetCustom("instances",MakeValue(accel->allocated));
  TemplateSetNumber("unitsMapped",val.unitsMapped);
  TemplateSetCustom("inputDecl",MakeValue(BasicDeclaration::input));
  TemplateSetCustom("outputDecl",MakeValue(BasicDeclaration::output));
  TemplateSetNumber("memoryAddressBits",val.memoryAddressBits);
    
  int lat = decl->lat;
  TemplateSetNumber("special",lat);

  ProcessTemplate(file,BasicTemplates::iterativeTemplate,&versat->temp,&versat->permanent);
}

void OutputVerilatorWrapper(Versat* versat,FUDeclaration* type,Accelerator* accel,String outputPath,Arena* temp,Arena* temp2){
  BLOCK_REGION(temp);
  BLOCK_REGION(temp2);

  ClearTemplateEngine(); // Make sure that we do not reuse data

  Array<InstanceInfo> info = TransformGraphIntoArray(accel,true,temp,temp2);
  Array<Wire> allConfigsHeaderSide = ExtractAllConfigs(info,&versat->permanent,&versat->temp);

  // We need to bundle config + static (type->config) only contains config, but not static
  Array<Wire> allConfigsVerilatorSide = PushArray<Wire>(temp,999); // TODO: Correct size
  {
    int index = 0;
    for(Wire& config : type->configInfo.configs){
      allConfigsVerilatorSide[index++] = config;
    }
    for(Pair<StaticId,StaticData> p : type->staticUnits){
      for(Wire& config : p.second.configs){
        allConfigsVerilatorSide[index] = config;
        allConfigsVerilatorSide[index].name = ReprStaticConfig(p.first,&config,&versat->permanent);
        index += 1;
      }
    }
    allConfigsVerilatorSide.size = index;
  }

  // Extract states with the expected TOP level name (not module name)
  Array<String> statesHeaderSide = ExtractStates(info,temp);
  
  int totalExternalMemory = ExternalMemoryByteSize(type->externalMemory);

  TemplateSetCustom("allConfigsVerilatorSide",MakeValue(&allConfigsVerilatorSide));
  TemplateSetCustom("configsHeader",MakeValue(&allConfigsHeaderSide));
  TemplateSetCustom("statesHeader",MakeValue(&statesHeaderSide));
  TemplateSetNumber("totalExternalMemory",totalExternalMemory);
  TemplateSetCustom("opts",MakeValue(&versat->opts));
  TemplateSetCustom("type",MakeValue(type));
  TemplateSetBool("trace",versat->debug.outputVCD);
  
  FILE* output = OpenFileAndCreateDirectories(StaticFormat("%.*s/wrapper.cpp",UNPACK_SS(outputPath)),"w");
  CompiledTemplate* templ = CompileTemplate(versat_wrapper_template,"wrapper",&versat->permanent,&versat->temp);
  ProcessTemplate(output,templ,temp,temp2);
  fclose(output);
}

#include <filesystem>
namespace fs = std::filesystem;

void OutputVerilatorMake(Versat* versat,String topLevelName,String versatDir,Options* opts,Arena* temp,Arena* temp2){
  BLOCK_REGION(temp);
  BLOCK_REGION(temp2);
  
  String outputPath = opts->softwareOutputFilepath;
  FILE* output = OpenFileAndCreateDirectories(StaticFormat("%.*s/VerilatorMake.mk",UNPACK_SS(outputPath)),"w");
  
  TemplateSetBool("traceEnabled",versat->debug.outputVCD);
  CompiledTemplate* comp = CompileTemplate(versat_makefile_template,"makefile",temp,temp2);
    
  fs::path outputFSPath = StaticFormat("%.*s",UNPACK_SS(outputPath));
  fs::path srcLocation = fs::current_path();
  fs::path fixedPath = fs::weakly_canonical(outputFSPath / srcLocation);

  String srcDir = PushString(temp,"%.*s/src",UNPACK_SS(outputPath));

  //Array<String> allSourcesNeeded = // TODO: It would be better if we generated a list with all the sources needed and at their correct location. Would be easier to debug any problems from missing files if we removed all the useless ones. Maybe even speed up verilator compile time.
    
  TemplateSetCustom("arch",MakeValue(&versat->opts));
  TemplateSetString("srcDir",srcDir);
  TemplateSetString("versatDir",versatDir);
  TemplateSetString("verilatorRoot",opts->verilatorRoot);
  TemplateSetNumber("bitWidth",opts->addrSize);
  TemplateSetString("generatedUnitsLocation",versat->opts->hardwareOutputFilepath);
  TemplateSetArray("verilogFiles","String",UNPACK_SS(opts->verilogFiles));
  TemplateSetArray("extraSources","String",UNPACK_SS(opts->extraSources));
  TemplateSetArray("includePaths","String",UNPACK_SS(opts->includePaths));
  TemplateSetString("typename",topLevelName);
  TemplateSetString("rootPath",STRING(fixedPath.c_str()));
  ProcessTemplate(output,comp,temp,temp2);
}

void OutputVersatSource(Versat* versat,Accelerator* accel,const char* hardwarePath,const char* softwarePath,String accelName,bool isSimple,Arena* temp,Arena* temp2){
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

  VersatComputedValues val = ComputeVersatValues(versat,accel,versat->opts->useDMA);
  UnitValues unit = CalculateAcceleratorValues(versat,accel);

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

  if(versat->opts->architectureHasDatabus){
    fprintf(c,"`define VERSAT_ARCH_HAS_IO 1\n");
    fprintf(f,"`undef  VERSAT_ARCH_HAS_IO\n");
  }

  if(unit.inputs || unit.outputs){
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

  if(versat->opts->exportInternalMemories){
    fprintf(c,"`define VERSAT_EXPORT_EXTERNAL_MEMORY\n");
    fprintf(f,"`undef  VERSAT_EXPORT_EXTERNAL_MEMORY\n");
  }
  
  fclose(c);
  fclose(f);

  // Output configuration file
  int size = Size(accel->allocated);
  Array<InstanceNode*> nodes = ListToArray(accel->allocated,size,temp2);

  ReorganizeAccelerator(accel,temp2); // TODO: Reorganize accelerator needs to go. Just call CalculateOrder when needed. Need to check how iterative work for this case, though
  Array<InstanceNode*> ordered = PushArray<InstanceNode*>(temp2,size);
  
  int i = 0;
  FOREACH_LIST_INDEXED(OrderedInstance*,ptr,accel->ordered,i){
    ordered[i] = ptr->node;

    FUInstance* inst = ptr->node->inst;
    if(inst->declaration->nIOs){
      inst->parameters = STRING("#(.AXI_ADDR_W(AXI_ADDR_W),.AXI_DATA_W(AXI_DATA_W),.LEN_W(LEN_W))"); // TODO: placeholder hack.
    }
  }

  Array<InstanceInfo> info = TransformGraphIntoArray(accel,true,temp,temp2);
  CheckSanity(info,temp);

  ComputedData computedData = CalculateVersatComputedData(info,val,temp);

  if(versat->opts->exportInternalMemories){
    int index = 0;
    for(ExternalMemoryInterface inter : computedData.external){
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
      default: NOT_IMPLEMENTED;
      }
    }
  }

  int staticStart = 0;
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    FUDeclaration* decl = ptr->inst->declaration;
    for(Wire& wire : decl->configInfo.configs){
      staticStart += wire.bitSize;
    }
  }

  // All dependent on external
  TemplateSetCustom("external",MakeValue(&computedData.external));
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

  Array<InstanceInfo> test = info;
  
  Hashmap<StaticId,StaticData>* staticUnits = PushHashmap<StaticId,StaticData>(temp,val.nStatics);
  int staticIndex = 0; // staticStart; TODO: For now, test with static info beginning at zero
  for(InstanceInfo& info : test){
    FUDeclaration* decl = info.decl;
    if(info.isStatic){
      FUDeclaration* parentDecl = info.parentDeclaration;

      StaticId id = {};
      id.name = info.name;
      id.parent = parentDecl;

      GetOrAllocateResult res = staticUnits->GetOrAllocate(id);

      if(res.alreadyExisted){
      } else {
        StaticData data = {};
        data.offset = staticIndex;
        data.configs = decl->configInfo.configs;

        *res.data = data;
        staticIndex += decl->configInfo.configs.size;
      }
    }
  }

  TemplateSetCustom("staticUnits",MakeValue(staticUnits));
  
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
  TemplateSetNumber("nInputs",unit.inputs);
  TemplateSetNumber("nOutputs",unit.outputs);
  TemplateSetCustom("ordered",MakeValue(&ordered));
  TemplateSetCustom("versatData",MakeValue(&computedData.data));
  TemplateSetCustom("instances",MakeValue(&nodes));
  TemplateSetNumber("staticStart",staticStart);
  TemplateSetBool("useDMA",versat->opts->useDMA);
  TemplateSetCustom("opts",MakeValue(&versat->opts));
  
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
  TemplateSetNumber("memoryMappedBase",1 << val.memoryConfigDecisionBit);
  TemplateSetNumber("nConfigs",val.nConfigs);
  TemplateSetNumber("nStatics",val.nStatics);

  Array<TypeStructInfo> nonMergedStructures = GetConfigStructInfo(accel,temp2,temp); // MARKED
  TemplateSetCustom("nonMergedStructures",MakeValue(&nonMergedStructures));

  Array<TypeStructInfo> addressStructures = GetMemMappedStructInfo(accel,temp2,temp);
  TemplateSetCustom("addressStructures",MakeValue(&addressStructures));
  
  {
    Array<InstanceInfo> onlySome = ExtractFromInstanceInfo(test,temp);
    
    Byte* mark = MarkArena(temp);
    for(InstanceInfo& t : test){
      if(!t.isComposite){
        for(int d : t.delay){
          *PushStruct<int>(temp) = d;
        }
      }
    }
    Array<int> delays = PointArray<int>(temp,mark);
    TemplateSetCustom("delay",MakeValue(&delays));

#if 0
    printf("ALL");
    NEWLINE();
    PrintAll(onlySome,temp);
#endif

    Array<Wire> orderedConfigs = ExtractAllConfigs(test,temp,temp2);
    Array<String> allStates = ExtractStates(test,temp2);
    Array<Pair<String,int>> allMem = ExtractMem(test,temp2);
    
    TemplateSetCustom("namedStates",MakeValue(&allStates));
    TemplateSetCustom("namedMem",MakeValue(&allMem));
    TemplateSetCustom("orderedConfigs",MakeValue(&orderedConfigs));

    TemplateSetString("accelType",accelName);
    TemplateSetBool("doingMerged",false);
    Array<String> empty = {};
    TemplateSetCustom("mergedConfigs0",MakeValue(&empty));
    TemplateSetCustom("mergedConfigs1",MakeValue(&empty));

    FOREACH_LIST(InstanceNode*,node,accel->allocated){
      if(node->inst->declaration->mergeInfo.size){
        printf("here\n");
      }
    }

    FILE* f = OpenFileAndCreateDirectories(StaticFormat("%s/versat_accel.h",softwarePath),"w");
    ProcessTemplate(f,BasicTemplates::acceleratorHeaderTemplate,temp,temp2);
    fclose(f);
  }
}

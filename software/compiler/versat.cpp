#include "versat.hpp"

#include <new>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <unordered_map>

#include "printf.h"

#include "thread.hpp"
#include "type.hpp"
#include "debugVersat.hpp"
#include "parser.hpp"
#include "configurations.hpp"
#include "utils.hpp"
#include "utilsCore.hpp"
#include "verilogParsing.hpp"
#include "templateEngine.hpp"
#include "textualRepresentation.hpp"
#include "templateData.hpp"

namespace BasicTemplates{
  CompiledTemplate* acceleratorTemplate;
  CompiledTemplate* topAcceleratorTemplate;
  CompiledTemplate* dataTemplate;
  CompiledTemplate* wrapperTemplate;
  CompiledTemplate* acceleratorHeaderTemplate;
  CompiledTemplate* externalInternalPortmapTemplate;
  CompiledTemplate* externalPortTemplate;
  CompiledTemplate* externalInstTemplate;
  CompiledTemplate* internalWiresTemplate;
  CompiledTemplate* iterativeTemplate;
}

static Value HexValue(Value in,Arena* out){
  static char buffer[128];

  int number = ConvertValue(in,ValueType::NUMBER,nullptr).number;

  int size = sprintf(buffer,"0x%x",number);

  Value res = {};
  res.type = ValueType::STRING;
  res.str = PushString(out,String{buffer,size});

  return res;
}

static Value EscapeString(Value val,Arena* out){
  Assert(val.type == ValueType::SIZED_STRING || val.type == ValueType::STRING);
  Assert(val.isTemp);

  String str = val.str;

  String escaped = PushString(out,str);
  char* view = (char*) escaped.data;

  for(int i = 0; i < escaped.size; i++){
    char ch = view[i];

    if(   (ch >= 'a' && ch <= 'z')
          || (ch >= 'A' && ch <= 'Z')
          || (ch >= '0' && ch <= '9')){
      continue;
    } else {
      view[i] = '_'; // Replace any foreign symbol with a underscore
    }
  }

  Value res = val;
  res.str = escaped;

  return res;
}

// MARK TODO: Small fix for common template. Works for now 
void SetIncludeHeader(CompiledTemplate* tpl,String name);

Versat* InitVersat(){
  static Versat versatInst = {};
  static bool doneOnce = false;

  Assert(!doneOnce); // For now, only allow one Versat instance
  doneOnce = true;

  InitDebug();
  RegisterTypes();

  //InitThreadPool(16);
  Versat* versat = &versatInst;

  versat->debug.outputAccelerator = true;
  versat->debug.outputVersat = true;

  versat->debug.dotFormat = GRAPH_DOT_FORMAT_NAME;

#ifdef x86
  versat->temp = InitArena(Megabyte(256));
#else
  versat->temp = InitArena(Gigabyte(8));
#endif // x86
  versat->permanent = InitArena(Megabyte(256));

  InitializeTemplateEngine(&versat->permanent);

  CompiledTemplate* commonTpl = CompileTemplate(versat_common_template,"common",&versat->permanent,&versat->temp);
  SetIncludeHeader(commonTpl,STRING("common"));

  // Technically, if more than 1 versat in the future, could move this outside
  BasicTemplates::acceleratorTemplate = CompileTemplate(versat_accelerator_template,"accel",&versat->permanent,&versat->temp);
  BasicTemplates::topAcceleratorTemplate = CompileTemplate(versat_top_instance_template,"top",&versat->permanent,&versat->temp);
  BasicTemplates::acceleratorHeaderTemplate = CompileTemplate(versat_header_template,"header",&versat->permanent,&versat->temp);
  BasicTemplates::externalInternalPortmapTemplate = CompileTemplate(external_memory_internal_portmap_template,"ext_internal_port",&versat->permanent,&versat->temp);
  BasicTemplates::externalPortTemplate = CompileTemplate(external_memory_port_template,"ext_port",&versat->permanent,&versat->temp);
  BasicTemplates::externalInstTemplate = CompileTemplate(external_memory_inst_template,"ext_inst",&versat->permanent,&versat->temp);
  BasicTemplates::iterativeTemplate = CompileTemplate(versat_iterative_template,"iter",&versat->permanent,&versat->temp);
  BasicTemplates::internalWiresTemplate = CompileTemplate(versat_internal_memory_wires_template,"internal wires",&versat->permanent,&versat->temp);
  
  RegisterPipeOperation(STRING("MemorySize"),[](Value val,Arena* out){
    ExternalMemoryInterface* inter = (ExternalMemoryInterface*) val.custom;
    int byteSize = ExternalMemoryByteSize(inter);
    return MakeValue(byteSize);
  });
  RegisterPipeOperation(STRING("Hex"),HexValue);
  RegisterPipeOperation(STRING("Identify"),EscapeString);
  RegisterPipeOperation(STRING("Type"),[](Value val,Arena* out){
    Type* type = val.type;

    return MakeValue(type->name);
  });
  
  versat->declarations.Clear(false);

  return versat;
}

int EvalRange(ExpressionRange range,Array<ParameterExpression> expressions){
  // TODO: Right now nullptr indicates that the wire does not exist. Do not know if it's worth to make it more explicit or not. Appears to be fine for now.
  if(range.top == nullptr || range.bottom == nullptr){
	Assert(range.top == range.bottom); // They must both be nullptr if one is
	return 0;
  }

  int top = Eval(range.top,expressions).number;
  int bottom = Eval(range.bottom,expressions).number;
  int size = (top - bottom) + 1;
  Assert(size > 0);

  return size;
}

// TODO: Need to remake this function and probably ModuleInfo has the versat compiler change is made
FUDeclaration* RegisterModuleInfo(Versat* versat,ModuleInfo* info){
  Arena* arena = &versat->permanent;

  FUDeclaration decl = {};
  
  // Check same name
  for(FUDeclaration* decl : versat->declarations){
    if(CompareString(decl->name,info->name)){
      LogFatal(LogModule::TOP_SYS,"Found a module with a same name (%.*s). Cannot proceed",UNPACK_SS(info->name));
    }
  }

  Array<Wire> configs = PushArray<Wire>(arena,info->configs.size);
  Array<Wire> states = PushArray<Wire>(arena,info->states.size);
  Array<ExternalMemoryInterface> external = PushArray<ExternalMemoryInterface>(arena,info->externalInterfaces.size);
  int memoryMapBits = 0;
  //int databusAddrSize = 0;

  region(arena){
    Array<ParameterExpression> instantiated = PushArray<ParameterExpression>(arena,info->defaultParameters.size);

    for(int i = 0; i < instantiated.size; i++){
      ParameterExpression def = info->defaultParameters[i];

      // TODO: Make this more generic. Probably gonna need to take a look at a FUDeclaration as a instance of a more generic FUType construct. FUDeclaration has constant values, the FUType stores the expressions to compute them.
      // Override databus size for current architecture
      if(CompareString(def.name,STRING("AXI_ADDR_W"))){
        Expression* expr = PushStruct<Expression>(arena);

        expr->type = Expression::LITERAL;
        expr->id = def.name;
        expr->val = MakeValue(versat->opts->addrSize);

        def.expr = expr;
      }

      if(CompareString(def.name,STRING("AXI_DATA_W"))){
        Expression* expr = PushStruct<Expression>(arena);

        expr->type = Expression::LITERAL;
        expr->id = def.name;
        expr->val = MakeValue(versat->opts->dataSize);

        def.expr = expr;
      }

      // Override length. Use 20 for now
      if(CompareString(def.name,STRING("LEN_W"))){
        Expression* expr = PushStruct<Expression>(arena);

        expr->type = Expression::LITERAL;
        expr->id = def.name;
        expr->val = MakeValue(16);

        def.expr = expr;
      }

      instantiated[i] = def;
    }

    for(int i = 0; i < info->configs.size; i++){
      WireExpression& wire = info->configs[i];

      int size = EvalRange(wire.bitSize,instantiated);

      configs[i].name = info->configs[i].name;
      configs[i].bitSize = size;
      configs[i].isStatic = false;
    }

    for(int i = 0; i < info->states.size; i++){
      WireExpression& wire = info->states[i];

      int size = EvalRange(wire.bitSize,instantiated);

      states[i].name = info->states[i].name;
      states[i].bitSize = size;
      states[i].isStatic = false;
    }

    for(int i = 0; i < info->externalInterfaces.size; i++){
      ExternalMemoryInterfaceExpression& expr = info->externalInterfaces[i];

      external[i].interface = expr.interface;
      external[i].type = expr.type;

      switch(expr.type){
      case ExternalMemoryType::TWO_P:{
        external[i].tp.bitSizeIn = EvalRange(expr.tp.bitSizeIn,instantiated);
        external[i].tp.bitSizeOut = EvalRange(expr.tp.bitSizeOut,instantiated);
        external[i].tp.dataSizeIn = EvalRange(expr.tp.dataSizeIn,instantiated);
        external[i].tp.dataSizeOut = EvalRange(expr.tp.dataSizeOut,instantiated);
      }break;
      case ExternalMemoryType::DP:{
        for(int ii = 0; ii < 2; ii++){
          external[i].dp[ii].bitSize = EvalRange(expr.dp[ii].bitSize,instantiated);
          external[i].dp[ii].dataSizeIn = EvalRange(expr.dp[ii].dataSizeIn,instantiated);
          external[i].dp[ii].dataSizeOut = EvalRange(expr.dp[ii].dataSizeOut,instantiated);
        }
      }break;
      default: NOT_IMPLEMENTED("Implemented as needed");
      }
    }

    memoryMapBits = EvalRange(info->memoryMappedBits,instantiated);
    //databusAddrSize = EvalRange(info->databusAddrSize,instantiated);
  }

  decl.name = info->name;
  decl.inputDelays = info->inputDelays;
  decl.outputLatencies = info->outputLatencies;
  decl.configInfo.configs = configs;
  decl.configInfo.states = states;
  decl.externalMemory = external;
  decl.configInfo.delayOffsets.max = info->nDelays;
  decl.nIOs = info->nIO;
  if(info->memoryMapped) decl.memoryMapBits = memoryMapBits;
  //decl.databusAddrSize = databusAddrSize; // TODO: How to handle different units with different databus address sizes??? For now do nothing. Do not even know if it's worth to bother with this, I think that all units should be force to use AXI_ADDR_W for databus addresses.
  //decl.memoryMapBits = info->memoryMapped;
  decl.implementsDone = info->hasDone;
  decl.signalLoop = info->signalLoop;

  if(info->isSource){
    decl.delayType = decl.delayType | DelayType::DelayType_SINK_DELAY;
  }

  decl.configInfo.configOffsets.max = info->configs.size;
  decl.configInfo.stateOffsets.max = info->states.size;

  FUDeclaration* res = RegisterFU(versat,decl);

  return res;
}

// Bunch of functions that could go to accelerator stats
// Returns the values when looking at a unit individually
UnitValues CalculateIndividualUnitValues(FUInstance* inst){
  UnitValues res = {};

  FUDeclaration* type = inst->declaration;

  res.outputs = type->outputLatencies.size; // Common all cases
  if(IsTypeHierarchical(type)){
    Assert(inst->declaration->fixedDelayCircuit);
	//} else if(type->type == FUDeclaration::ITERATIVE){
	//   Assert(inst->declaration->fixedDelayCircuit);
	//   res.delays = 1;
	//   res.extraData = sizeof(int);
  } else {
    res.configs = type->configInfo.configs.size;
    res.states = type->configInfo.states.size;
    res.delays = type->configInfo.delayOffsets.max;
    res.extraData = type->configInfo.extraDataOffsets.max;
  }

  return res;
}

// Returns the values when looking at subunits as well. For simple units, same as individual unit values
// For composite units, same as calculate accelerator values + calculate individual unit values
UnitValues CalculateAcceleratorUnitValues(Versat* versat,FUInstance* inst){
  UnitValues res = {};

  FUDeclaration* type = inst->declaration;

  if(!IsTypeHierarchical(type)){
    res = CalculateIndividualUnitValues(inst);
  }

  return res;
}

UnitValues CalculateAcceleratorValues(Versat* versat,Accelerator* accel){
  ArenaMarker marker(&accel->versat->temp);

  UnitValues val = {};

  std::vector<bool> seenShared;

  Hashmap<StaticId,int>* staticSeen = PushHashmap<StaticId,int>(&accel->versat->temp,1000);

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
        val.configs += type->configInfo.configs.size;
        val.sharedUnits += 1;
      }

      seenShared[inst->sharedIndex] = true;
    } else if(!inst->isStatic){ // Shared cannot be static
         val.configs += type->configInfo.configs.size;
    }

    if(type->memoryMapBits.has_value()){
      val.isMemoryMapped = true;

      memoryMappedDWords = AlignBitBoundary(memoryMappedDWords,type->memoryMapBits.value());
      memoryMappedDWords += (1 << type->memoryMapBits.value());
    }

    if(type == BasicDeclaration::input){
      val.inputs += 1;
    }

    val.states += type->configInfo.states.size;
    val.delays += type->configInfo.delayOffsets.max;
    val.ios += type->nIOs;
    val.extraData += type->configInfo.extraDataOffsets.max;

    if(type->externalMemory.size){
      val.externalMemoryInterfaces += type->externalMemory.size;
	  val.externalMemoryByteSize = ExternalMemoryByteSize(type->externalMemory);
    }

    val.signalLoop |= type->signalLoop;
  }

  val.memoryMappedBits = log2i(memoryMappedDWords);

  FUInstance* outputInstance = GetOutputInstance(accel->allocated);
  if(outputInstance){
    FOREACH_LIST(Edge*,edge,accel->edges){
      if(edge->units[0].inst == outputInstance){
        val.outputs = std::max(val.outputs - 1,edge->units[0].port) + 1;
      }
      if(edge->units[1].inst == outputInstance){
        val.outputs = std::max(val.outputs - 1,edge->units[1].port) + 1;
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
      val.statics += inst->declaration->configInfo.configs.size;
    }

    if(IsTypeHierarchical(inst->declaration)){
      for(Pair<StaticId,StaticData> pair : inst->declaration->staticUnits){
        int* possibleFind = staticSeen->Get(pair.first);

        if(!possibleFind){
          staticSeen->Insert(pair.first,1);
          val.statics += pair.second.configs.size;
        }
      }
    }
  }

  return val;
}

void FillDeclarationWithAcceleratorValues(Versat* versat,FUDeclaration* decl,Accelerator* accel){
  Arena* permanent = &versat->permanent;
  Arena* temp = &versat->temp;
  BLOCK_REGION(temp);

  UnitValues val = CalculateAcceleratorValues(versat,accel);

  bool anySavedConfiguration = false;
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    FUInstance* inst = ptr->inst;
    anySavedConfiguration |= inst->savedConfiguration;
  }

  decl->configInfo.delayOffsets.max = val.delays;
  decl->nIOs = val.ios;
  decl->configInfo.extraDataOffsets.max = val.extraData;
  decl->nStaticConfigs = val.statics;
  if(val.isMemoryMapped){
    decl->memoryMapBits = val.memoryMappedBits;
  }

  decl->inputDelays = PushArray<int>(permanent,val.inputs);

  decl->externalMemory = PushArray<ExternalMemoryInterface>(permanent,val.externalMemoryInterfaces);
  int externalIndex = 0;
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    Array<ExternalMemoryInterface> arr = ptr->inst->declaration->externalMemory;
    for(int i = 0; i < arr.size; i++){
      decl->externalMemory[externalIndex] = arr[i];
      decl->externalMemory[externalIndex].interface = externalIndex;
      externalIndex += 1;
    }
  }

  for(int i = 0; i < val.inputs; i++){
    FUInstance* input = GetInputInstance(accel->allocated,i);

    if(!input){
      continue;
    }

    decl->inputDelays[i] = input->baseDelay;
  }

  decl->outputLatencies = PushArray<int>(permanent,val.outputs);
  FUInstance* outputInstance = GetOutputInstance(accel->allocated);
  if(outputInstance){
    for(int i = 0; i < decl->outputLatencies.size; i++){
      decl->outputLatencies[i] = outputInstance->baseDelay;
    }
  }

  decl->configInfo.configs = PushArray<Wire>(permanent,val.configs);
  decl->configInfo.states = PushArray<Wire>(permanent,val.states);

  Hashmap<int,int>* staticsSeen = PushHashmap<int,int>(temp,val.sharedUnits);

  int configIndex = 0;
  int stateIndex = 0;

  // TODO: This could be done based on the config offsets.
  //       Otherwise we are still basing code around static and shared logic when calculating offsets already does that for us.
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    FUInstance* inst = ptr->inst;
    FUDeclaration* d = inst->declaration;

    if(!inst->isStatic){
      if(inst->sharedEnable){
        if(staticsSeen->InsertIfNotExist(inst->sharedIndex,0)){
          for(Wire& wire : d->configInfo.configs){
            decl->configInfo.configs[configIndex].name = PushString(permanent,"%.*s_%.*s",UNPACK_SS(inst->name),UNPACK_SS(wire.name));
            decl->configInfo.configs[configIndex++].bitSize = wire.bitSize;
          }
        }
      } else {
        for(Wire& wire : d->configInfo.configs){
          decl->configInfo.configs[configIndex].name = PushString(permanent,"%.*s_%.*s",UNPACK_SS(inst->name),UNPACK_SS(wire.name));
          decl->configInfo.configs[configIndex++].bitSize = wire.bitSize;
        }
      }
    }

    for(Wire& wire : d->configInfo.states){
      decl->configInfo.states[stateIndex].name = PushString(permanent,"%.*s_%.2d",UNPACK_SS(wire.name),stateIndex);
      decl->configInfo.states[stateIndex++].bitSize = wire.bitSize;
    }
  }

  decl->signalLoop = val.signalLoop;
  decl->configInfo.configOffsets = CalculateConfigurationOffset(accel,MemType::CONFIG,permanent);
  decl->configInfo.stateOffsets = CalculateConfigurationOffset(accel,MemType::STATE,permanent);
  decl->configInfo.delayOffsets = CalculateConfigurationOffset(accel,MemType::DELAY,permanent);
  Assert(decl->configInfo.configOffsets.max == val.configs);
  Assert(decl->configInfo.stateOffsets.max == val.states);
  Assert(decl->configInfo.delayOffsets.max == val.delays);

  decl->configInfo.extraDataOffsets = CalculateConfigurationOffset(accel,MemType::EXTRA,permanent);
  Assert(decl->configInfo.extraDataOffsets.max == val.extraData);
}

// Problem: RegisterSubUnit is basically extracting all the information
// from the graph and then is filling the FUDeclaration with all this info.
// When it comes to merge, we do not want to do this because the information that we want might not
// corresponds to the actual graph 1-to-1.
// Might also be better for Flatten if we separated the process so that we do not have to deal with
// share configs and static configs and stuff like that.

FUDeclaration* RegisterSubUnit(Versat* versat,Accelerator* circuit){
  Arena* permanent = &versat->permanent;
  Arena* temp = &versat->temp;
  BLOCK_REGION(temp);

  // Disabled for now.
#if 0
  if(IsCombinatorial(circuit)){
    circuit = Flatten(versat,circuit,99);
  }
#endif

  String name = circuit->name;
  
  FUDeclaration decl = {};
  decl.type = FUDeclaration::COMPOSITE;
  decl.name = name;

  String basePath = PushString(temp,"debug/%.*s/base.dot",UNPACK_SS(name));
  String base2Path = PushString(temp,"debug/%.*s/base2.dot",UNPACK_SS(name));

  OutputGraphDotFile(versat,circuit,false,basePath,temp);
  decl.baseCircuit = CopyAccelerator(versat,circuit,nullptr);
  OutputGraphDotFile(versat,decl.baseCircuit,false,base2Path,temp);
  ReorganizeAccelerator(circuit,temp);

  CalculateDelayResult delays = CalculateDelay(versat,circuit,temp);

  decl.calculatedDelays = PushArray<int>(permanent,delays.nodeDelay->nodesUsed);
  Memset(decl.calculatedDelays,0);
  int index = 0;
  for(Pair<InstanceNode*,int> p : delays.nodeDelay){
    if(p.first->inst->declaration->configInfo.delayOffsets.max > 0){
      decl.calculatedDelays[index] = p.second;
      index += 1;
    }
  }
  
  region(temp){
    String beforePath = PushString(temp,"debug/%.*s/beforeFixDelay.dot",UNPACK_SS(name));
    String afterPath = PushString(temp,"debug/%.*s/afterFixDelay.dot",UNPACK_SS(name));

    OutputGraphDotFile(versat,circuit,false,beforePath,temp);
    FixDelays(versat,circuit,delays.edgesDelay);
    OutputGraphDotFile(versat,circuit,false,afterPath,temp);
  }

  ReorganizeAccelerator(circuit,temp);
  decl.fixedDelayCircuit = circuit;

#if 1
  // Maybe this check should be done elsewhere
  FOREACH_LIST(InstanceNode*,ptr,circuit->allocated){
    if(ptr->multipleSamePortInputs){
      printf("Multiple inputs: %.*s\n",UNPACK_SS(name));
      break;
    }
  }
#endif
  
  // Need to calculate versat data here.
  FillDeclarationWithAcceleratorValues(versat,&decl,circuit);

  // TODO: Change unit delay type inference. Only care about delay type to upper levels.
  // Type source only if a source unit is connected to out. Type sink only if there is a input to sink connection
  bool hasSourceDelay = false;
  bool hasSinkDelay = false;
  bool implementsDone = false;

  FOREACH_LIST(InstanceNode*,ptr,circuit->allocated){
    FUInstance* inst = ptr->inst;
    if(inst->declaration->type == FUDeclaration::SPECIAL){
      continue;
    }

    if(inst->declaration->implementsDone){
      implementsDone = true;
    }
    if(ptr->type == InstanceNode::TAG_SINK){
      hasSinkDelay = CHECK_DELAY(inst,DelayType_SINK_DELAY);
    }
    if(ptr->type == InstanceNode::TAG_SOURCE){
      hasSourceDelay = CHECK_DELAY(inst,DelayType_SOURCE_DELAY);
    }
    if(ptr->type == InstanceNode::TAG_SOURCE_AND_SINK){
      hasSinkDelay = CHECK_DELAY(inst,DelayType_SINK_DELAY);
      hasSourceDelay = CHECK_DELAY(inst,DelayType_SOURCE_DELAY);
    }
  }

 if(hasSourceDelay){
    decl.delayType = (DelayType) ((int)decl.delayType | (int) DelayType::DelayType_SOURCE_DELAY);
  }
  if (hasSinkDelay){
    decl.delayType = (DelayType) ((int)decl.delayType | (int) DelayType::DelayType_SINK_DELAY);
  }

  decl.implementsDone = implementsDone;

  decl.staticUnits = PushHashmap<StaticId,StaticData>(permanent,1000); // TODO: Set correct number of elements
  int staticOffset = 0;
  // Start by collecting all the existing static allocated units in subinstances
  FOREACH_LIST(InstanceNode*,ptr,circuit->allocated){
    FUInstance* inst = ptr->inst;
    if(IsTypeHierarchical(inst->declaration)){

      for(auto pair : inst->declaration->staticUnits){
        StaticData newData = pair.second;
        newData.offset = staticOffset;

        if(decl.staticUnits->InsertIfNotExist(pair.first,newData)){
          staticOffset += newData.configs.size;
        }
      }
    }
  }

  FUDeclaration* res = RegisterFU(versat,decl);
  res->baseCircuit->name = name;
  res->fixedDelayCircuit->name = name;

  // Add this units static instances (needs be done after Registering the declaration because the parent is a pointer to the declaration)
  FOREACH_LIST(InstanceNode*,ptr,circuit->allocated){
    FUInstance* inst = ptr->inst;
    if(inst->isStatic){
      StaticId id = {};
      id.name = inst->name;
      id.parent = res;

      StaticData data = {};
      data.configs = inst->declaration->configInfo.configs;
      data.offset = staticOffset;

      if(res->staticUnits->InsertIfNotExist(id,data)){
        staticOffset += inst->declaration->configInfo.configs.size;
      }
    }
  }

  return res;
}

struct Connection{
  InstanceNode* input;
  InstanceNode* mux;
  PortNode unit;
};

FUDeclaration* RegisterIterativeUnit(Versat* versat,Accelerator* accel,FUInstance* inst,int latency,String name){
  InstanceNode* node = GetInstanceNode(accel,(FUInstance*) inst);

  Assert(node);

  Arena* temp = &versat->temp;
  BLOCK_REGION(temp);

  Array<Array<PortNode>> inputs = GetAllInputs(node,temp);

  for(Array<PortNode>& arr : inputs){
    Assert(arr.size <= 2); // Do not know what to do if size bigger than 2.
  }

  static String muxNames[] = {STRING("Mux1"),STRING("Mux2"),STRING("Mux3"),STRING("Mux4"),STRING("Mux5"),STRING("Mux6"),STRING("Mux7"),
                               STRING("Mux8"),STRING("Mux9"),STRING("Mux10"),STRING("Mux11"),STRING("Mux12"),STRING("Mux13"),STRING("Mux14"),STRING("Mux15"),
                               STRING("Mux16"),STRING("Mux17"),STRING("Mux18"),STRING("Mux19"),STRING("Mux20"),STRING("Mux21"),STRING("Mux22"),STRING("Mux23"),
                               STRING("Mux24"),STRING("Mux25"),STRING("Mux26"),STRING("Mux27"),STRING("Mux28"),STRING("Mux29"),STRING("Mux30"),STRING("Mux31"),STRING("Mux32"),STRING("Mux33"),STRING("Mux34"),
                               STRING("Mux35"),STRING("Mux36"),STRING("Mux37"),STRING("Mux38"),STRING("Mux39"),STRING("Mux40"),STRING("Mux41"),STRING("Mux42"),STRING("Mux43"),STRING("Mux44"),STRING("Mux45"),STRING("Mux46"),STRING("Mux47"),STRING("Mux48"),STRING("Mux49"),STRING("Mux50"),STRING("Mux51"),STRING("Mux52"),STRING("Mux53"),STRING("Mux54"),STRING("Mux55")};

  FUDeclaration* type = BasicDeclaration::timedMultiplexer;

  String beforePath = PushString(temp,"debug/%.*s/iterativeBefore.dot",UNPACK_SS(name));
  OutputGraphDotFile(versat,accel,false,beforePath,temp);

  Array<Connection> conn = PushArray<Connection>(temp,inputs.size);
  Memset(conn,{});

  int muxInserted = 0;
  for(int i = 0; i < inputs.size; i++){
    Array<PortNode>& arr = inputs[i];
    if(arr.size != 2){
      continue;
    }

    PortNode first = arr[0];

    if(first.node->inst->declaration != BasicDeclaration::input){
      SWAP(arr[0],arr[1]);
    }

    Assert(arr[0].node->inst->declaration == BasicDeclaration::input);

    Assert(i < ARRAY_SIZE(muxNames));
    FUInstance* mux = CreateFUInstance(accel,type,muxNames[i]);
    InstanceNode* muxNode = GetInstanceNode(accel,(FUInstance*) mux);

    RemoveConnection(accel,arr[0].node,arr[0].port,node,i);
    RemoveConnection(accel,arr[1].node,arr[1].port,node,i);

    ConnectUnitsGetEdge(arr[0],(PortNode){muxNode,0},0);
    ConnectUnitsGetEdge(arr[1],(PortNode){muxNode,1},0);

    ConnectUnitsGetEdge((PortNode){muxNode,0},(PortNode){node,i},0);

    conn[i].input = arr[0].node;
    conn[i].unit = PortNode{arr[1].node,arr[1].port};
    conn[i].mux = muxNode;

    muxInserted += 1;
  }

  FUDeclaration* bufferType = BasicDeclaration::buffer;
  int buffersAdded = 0;
  for(int i = 0; i < inputs.size; i++){
    if(!conn[i].mux){
      continue;
    }

    InstanceNode* unit = conn[i].unit.node;
    Assert(i < unit->inst->declaration->inputDelays.size);

    if(unit->inst->declaration->inputDelays[i] == 0){
      continue;
    }

    FUInstance* buffer = (FUInstance*) CreateFUInstance(accel,bufferType,PushString(&versat->permanent,"Buffer%d",buffersAdded++));

    buffer->bufferAmount = unit->inst->declaration->inputDelays[i] - BasicDeclaration::buffer->outputLatencies[0];
    Assert(buffer->bufferAmount >= 0);
    SetStatic(accel,buffer);

    if(buffer->config){
      buffer->config[0] = buffer->bufferAmount;
    }

    InstanceNode* bufferNode = GetInstanceNode(accel,buffer);

    InsertUnit(accel,(PortNode){conn[i].mux,0},conn[i].unit,(PortNode){bufferNode,0});
  }

  String afterPath = PushString(temp,"debug/%.*s/iterativeBefore.dot",UNPACK_SS(name));
  OutputGraphDotFile(versat,accel,true,afterPath,temp);

  FUDeclaration declaration = {};

  declaration = *inst->declaration; // By default, copy everything from unit
  declaration.name = PushString(&versat->permanent,name);
  declaration.type = FUDeclaration::ITERATIVE;

  FillDeclarationWithAcceleratorValues(versat,&declaration,accel);

  // Kinda of a hack, for now
#if 0
  int min = std::min(node->inst->declaration->inputDelays.size,declaration.inputDelays.size);
  for(int i = 0; i < min; i++){
    declaration.inputDelays[i] = node->inst->declaration->inputDelays[i];
  }
#endif

  // TODO: We are not checking connections here, we are just assuming that unit is directly connected to out.
  //       Probably a bad ideia but still do not have an example which would make it not ideal
  for(int i = 0; i < declaration.outputLatencies.size; i++){
    declaration.outputLatencies[i] = latency * (node->inst->declaration->outputLatencies[i] + 1);
  }

  // Values from iterative declaration
  ReorganizeIterative(accel,temp);
  declaration.baseCircuit = accel;
  declaration.fixedDelayCircuit = accel;

#if 0
  Accelerator* f = CopyAccelerator(versat,accel,nullptr);
  ReorganizeIterative(f,temp);

  // TODO: HACK, for now
  FUDeclaration temp = {};
  temp.name = name;
  f->subtype = &temp;

  auto delays = CalculateDelay(versat,f,temp);
  FixDelays(versat,f,delays);

  OutputGraphDotFile(versat,f,true,"debug/iterative2.dot");

  f->ordered = nullptr;
  ReorganizeIterative(f,temp);

  declaration.fixedDelayCircuit = f;
#endif

  declaration.staticUnits = PushHashmap<StaticId,StaticData>(&versat->permanent,1000); // TODO: Set correct number of elements
  int staticOffset = 0;
  // Start by collecting all the existing static allocated units in subinstances
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    FUInstance* inst = ptr->inst;
    if(IsTypeHierarchical(inst->declaration)){

      for(auto pair : inst->declaration->staticUnits){
        StaticData newData = pair.second;
        newData.offset = staticOffset;

        if(declaration.staticUnits->InsertIfNotExist(pair.first,newData)){
          staticOffset += newData.configs.size;
        }
      }
    }
  }

  FUDeclaration* registeredType = RegisterFU(versat,declaration);
  registeredType->lat = node->inst->declaration->outputLatencies[0];
  registeredType->calculatedDelays = PushArray<int>(&versat->permanent,99);
  Memset(registeredType->calculatedDelays,0);
  
  // Add this units static instances (needs be done after Registering the declaration because the parent is a pointer to the declaration)
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    FUInstance* inst = ptr->inst;
    if(inst->isStatic){
      StaticId id = {};
      id.name = inst->name;
      id.parent = registeredType;

      StaticData data = {};
      data.configs = inst->declaration->configInfo.configs;
      data.offset = staticOffset;

      if(registeredType->staticUnits->InsertIfNotExist(id,data)){
        staticOffset += inst->declaration->configInfo.configs.size;
      }
    }
  }

  return registeredType;
}

FUInstance* CreateOrGetInput(Accelerator* accel,String name,int portNumber){
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    FUInstance* inst = ptr->inst;
    if(inst->declaration == BasicDeclaration::input && inst->portIndex == portNumber){
      return inst;
    }
  }

  FUInstance* inst = (FUInstance*) CreateFUInstance(accel,BasicDeclaration::input,name);
  inst->portIndex = portNumber;

  return inst;
}

FUInstance* CreateOrGetOutput(Accelerator* accel){
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    FUInstance* inst = ptr->inst;
    if(inst->declaration == BasicDeclaration::output){
      return inst;
    }
  }

  FUInstance* inst = (FUInstance*) CreateFUInstance(accel,BasicDeclaration::output,STRING("out"));
  return inst;
}

int GetInputPortNumber(FUInstance* inputInstance){
    Assert(inputInstance->declaration == BasicDeclaration::input);

  return ((FUInstance*) inputInstance)->portIndex;
}

void PrintDeclaration(FILE* out,FUDeclaration* decl,Arena* temp,Arena* temp2){
  BLOCK_REGION(temp);
  BLOCK_REGION(temp2);

  fprintf(out,"Declaration: %.*s\n",UNPACK_SS(decl->name));
  fprintf(out,"Inputs: %d\n",decl->inputDelays.size);
  fprintf(out,"  ");
  for(int i = 0; i < decl->inputDelays.size; i++){
    if(i != 0) fprintf(out,",");
    fprintf(out,"%d",decl->inputDelays[i]);
  }
  fprintf(out,"\n");

  fprintf(out,"Outputs: %d\n",decl->outputLatencies.size);
  fprintf(out,"  ");
  for(int i = 0; i < decl->outputLatencies.size; i++){
    if(i != 0) fprintf(out,",");
    fprintf(out,"%d",decl->outputLatencies[i]);
  }
  fprintf(out,"\n");

  if(decl->fixedDelayCircuit){
    Array<InstanceInfo> info = TransformGraphIntoArray(decl->fixedDelayCircuit,true,temp,temp2);

    PrintAll(out,info,temp);
  } else {
    String type = DelayTypeToString(decl->delayType);
    fprintf(out,"Type: %.*s\n",UNPACK_SS(type));
  }
}

bool CheckValidName(String name){
  for(int i = 0; i < name.size; i++){
    char ch = name[i];

    bool allowed = (ch >= 'a' && ch <= 'z')
                   ||  (ch >= 'A' && ch <= 'Z')
                   ||  (ch >= '0' && ch <= '9' && i != 0)
                   ||  (ch == '_')
                   ||  (ch == '.' && i != 0)  // For now allow it, despite the fact that it should only be used internally by Versat
                   ||  (ch == '/' && i != 0); // For now allow it, despite the fact that it should only be used internally by Versat

    if(!allowed){
      return false;
    }
  }
  return true;
}

bool IsTypeHierarchical(FUDeclaration* decl){
   bool res = (decl->fixedDelayCircuit != nullptr);
   return res;
}


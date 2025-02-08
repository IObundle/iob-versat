#include "versat.hpp"

#include <new>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <unordered_map>

#include "globals.hpp"
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
#include "declaration.hpp"

namespace BasicTemplates{
  CompiledTemplate* acceleratorTemplate;
  CompiledTemplate* topAcceleratorTemplate;
  CompiledTemplate* topConfigurationsTemplate;
  CompiledTemplate* dataTemplate;
  CompiledTemplate* wrapperTemplate;
  CompiledTemplate* acceleratorHeaderTemplate;
  CompiledTemplate* externalInternalPortmapTemplate;
  CompiledTemplate* externalPortTemplate;
  CompiledTemplate* externalInstTemplate;
  CompiledTemplate* internalWiresTemplate;
  CompiledTemplate* iterativeTemplate;
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
FUDeclaration* RegisterModuleInfo(ModuleInfo* info){
  TEMP_REGION(temp,nullptr);

  Arena* perm = globalPermanent;
  
  // Check same name
  for(FUDeclaration* decl : globalDeclarations){
    if(CompareString(decl->name,info->name)){
      LogFatal(LogModule::TOP_SYS,"Found a module with a same name (%.*s). Cannot proceed",UNPACK_SS(info->name));
    }
  }

  FUDeclaration decl = {};

  Array<Wire> configs = PushArray<Wire>(perm,info->configs.size);
  Array<Wire> states = PushArray<Wire>(perm,info->states.size);
  Array<ExternalMemoryInterface> external = PushArray<ExternalMemoryInterface>(perm,info->externalInterfaces.size);
  int memoryMapBits = 0;

  Array<ParameterExpression> instantiated = PushArray<ParameterExpression>(perm,info->defaultParameters.size);

  decl.parameters = Extract(info->defaultParameters,perm,&ParameterExpression::name);
  //decl.parameters = Map(info->defaultParameters,perm,[](ParameterExpression p) -> String{
  //  return p.name;
  //});
  
  for(int i = 0; i < instantiated.size; i++){
    ParameterExpression def = info->defaultParameters[i];

    // TODO: Make this more generic. Probably gonna need to take a look at a FUDeclaration as a instance of a more generic FUType construct. FUDeclaration has constant values, the FUType stores the expressions to compute them.
    // Override databus size for current architecture
    if(CompareString(def.name,STRING("AXI_ADDR_W"))){
      Expression* expr = PushStruct<Expression>(temp);

      expr->type = Expression::LITERAL;
      expr->id = def.name;
      expr->val = MakeValue(globalOptions.databusAddrSize);

      def.expr = expr;
    }

    if(CompareString(def.name,STRING("AXI_DATA_W"))){
      Expression* expr = PushStruct<Expression>(temp);

      expr->type = Expression::LITERAL;
      expr->id = def.name;
      expr->val = MakeValue(globalOptions.databusDataSize);

      def.expr = expr;
    }

    // Override length. Use 20 for now
    if(CompareString(def.name,STRING("LEN_W"))){
      Expression* expr = PushStruct<Expression>(temp);

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
    configs[i].stage = info->configs[i].stage;
  }

  for(int i = 0; i < info->states.size; i++){
    WireExpression& wire = info->states[i];

    int size = EvalRange(wire.bitSize,instantiated);

    states[i].name = info->states[i].name;
    states[i].bitSize = size;
    states[i].isStatic = false;
    states[i].stage = info->states[i].stage;
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
    }
  }

  memoryMapBits = EvalRange(info->memoryMappedBits,instantiated);
  
  decl.name = info->name;

  decl.info.infos = PushArray<MergePartition>(globalPermanent,1);
  decl.info.infos[0].inputDelays = info->inputDelays;
  decl.info.infos[0].outputLatencies = info->outputLatencies;
  
  decl.configs = configs;
  decl.states = states;
  decl.externalMemory = external;
  decl.numberDelays = info->nDelays;
  decl.nIOs = info->nIO;

  if(info->memoryMapped) decl.memoryMapBits = memoryMapBits;

  decl.implementsDone = info->hasDone;
  decl.signalLoop = info->signalLoop;

  if(info->isSource){
    decl.delayType = decl.delayType | DelayType::DelayType_SINK_DELAY;
  }

  FUDeclaration* res = RegisterFU(decl);

  return res;
}

void FillDeclarationWithAcceleratorValues(FUDeclaration* decl,Accelerator* accel){
  TEMP_REGION(temp,nullptr);
  TEMP_REGION(temp2,temp);

  Arena* perm = globalPermanent;

  // TODO: This function was mostly used to calculate the configInfo stuff, which now is gone.
  //       Need to see how much of this code is also useful for the merge stuff.
  //       (If after merge puts the correct values inside the accelInfo struct, if we can just call this function to compute the remaining data that needs to be computed).
  //       We can also move some of this computation to the accelInfo struct. Just see how things play out.
  
  AccelInfo val = CalculateAcceleratorInfo(accel,true,perm);
  decl->info = val;
  
#if 0
  DynamicArray<String> baseNames = StartArray<String>(perm);
  for(FUInstance* ptr : accel->allocated){
    FUInstance* inst = ptr;
    *baseNames.PushElem() = inst->name;
  }
  Array<String> baseName = EndArray(baseNames);
#endif
  
  decl->nIOs = val.nIOs;
  if(val.isMemoryMapped){
    decl->memoryMapBits = val.memoryMappedBits;
  }

  decl->externalMemory = PushArray<ExternalMemoryInterface>(perm,val.externalMemoryInterfaces);
  int externalIndex = 0;
  for(FUInstance* ptr : accel->allocated){
    Array<ExternalMemoryInterface> arr = ptr->declaration->externalMemory;
    for(int i = 0; i < arr.size; i++){
      decl->externalMemory[externalIndex] = arr[i];
      decl->externalMemory[externalIndex].interface = externalIndex;
      externalIndex += 1;
    }
  }
    
  decl->configs = PushArray<Wire>(perm,val.configs);
  decl->states = PushArray<Wire>(perm,val.states);
  
  Hashmap<int,int>* staticsSeen = PushHashmap<int,int>(temp,val.sharedUnits);

  int configIndex = 0;
  int stateIndex = 0;
  
  // TODO: This could be done based on the config offsets.
  //       Otherwise we are still basing code around static and shared logic when calculating offsets already does that for us.
  for(FUInstance* ptr : accel->allocated){
    FUInstance* inst = ptr;
    FUDeclaration* d = inst->declaration;

    if(!inst->isStatic){
      if(inst->sharedEnable){
        if(staticsSeen->InsertIfNotExist(inst->sharedIndex,0)){
          for(Wire& wire : d->configs){
            decl->configs[configIndex] = wire;
            decl->configs[configIndex++].name = PushString(perm,"%.*s_%.*s",UNPACK_SS(inst->name),UNPACK_SS(wire.name));
          }
        }
      } else {
        for(Wire& wire : d->configs){
          decl->configs[configIndex] = wire;

          decl->configs[configIndex++].name = PushString(perm,"%.*s_%.*s",UNPACK_SS(inst->name),UNPACK_SS(wire.name));
        }
      }
    }

    for(Wire& wire : d->states){
      decl->states[stateIndex] = wire;

      decl->states[stateIndex].name = PushString(perm,"%.*s_%.2d",UNPACK_SS(wire.name),stateIndex);
      stateIndex += 1;
    }
  }

  decl->signalLoop = val.signalLoop;

  Array<bool> belongArray = PushArray<bool>(perm,accel->allocated.Size());
  Memset(belongArray,true);

  decl->numberDelays = val.delays;
}

void FillDeclarationWithDelayType(FUDeclaration* decl){
  // TODO: Change unit delay type inference. Only care about delay type to upper levels.
  // Type source only if a source unit is connected to out. Type sink only if there is a input to sink connection
  bool hasSourceDelay = false;
  bool hasSinkDelay = false;
  bool implementsDone = false;

  for(FUInstance* ptr : decl->fixedDelayCircuit->allocated){
    FUInstance* inst = ptr;
    if(inst->declaration->type == FUDeclarationType_SPECIAL){
      continue;
    }

    if(inst->declaration->implementsDone){
      implementsDone = true;
    }
    if(ptr->type == NodeType_SINK){
      // TODO: There was a problem caused by a unit getting marked as a sink and source when it should have been marked as compute.
      //       This occured in the Variety1 test. Disabled this check caused no problem in any other test and solved our Variety1 problem. We probably want to do a full check of everything related to these calculations.
      // NOTE: In fact, even removed all the other checks only causes one or two tests to fail. This approach is maybe not needed fully and we probably could simplify some stuff from here.
      //hasSinkDelay = CHECK_DELAY(inst,DelayType_SINK_DELAY);
    }
    if(ptr->type == NodeType_SOURCE){
      hasSourceDelay = CHECK_DELAY(inst,DelayType_SOURCE_DELAY);
    }
    if(ptr->type == NodeType_SOURCE_AND_SINK){
      hasSinkDelay = CHECK_DELAY(inst,DelayType_SINK_DELAY);
      hasSourceDelay = CHECK_DELAY(inst,DelayType_SOURCE_DELAY);
    }
  }

  if(hasSourceDelay){
    decl->delayType = (DelayType) ((int)decl->delayType | (int) DelayType::DelayType_SOURCE_DELAY);
  }
  if (hasSinkDelay){
    decl->delayType = (DelayType) ((int)decl->delayType | (int) DelayType::DelayType_SINK_DELAY);
  }

  decl->implementsDone = implementsDone;
}

FUDeclaration* RegisterSubUnit(Accelerator* circuit,SubUnitOptions options){
  TEMP_REGION(temp,nullptr);
  TEMP_REGION(temp2,temp);

  Arena* permanent = globalPermanent;

    // Disabled for now.
#if 0
  if(IsCombinatorial(circuit)){
    circuit = Flatten(versat,circuit,99);
  }
#endif
  
  String name = circuit->name;
  FUDeclaration decl = {};
  FUDeclaration* res = RegisterFU(decl);
  res->type = FUDeclarationType_COMPOSITE;
  res->name = name;

  // Default parameters given to all modules. Parameters need a proper revision, but need to handle parameters going up in the hierarchy
  res->parameters = PushArray<String>(permanent,6);
  res->parameters[0] = STRING("ADDR_W");
  res->parameters[1] = STRING("DATA_W");
  res->parameters[2] = STRING("DELAY_W");
  res->parameters[3] = STRING("AXI_ADDR_W");
  res->parameters[4] = STRING("AXI_DATA_W");
  res->parameters[5] = STRING("LEN_W");
  
  // TODO: Need to add back the OutputDebugDotGraph calls
  if(options == SubUnitOptions_BAREBONES){
    res->baseCircuit = CopyAccelerator(circuit,AcceleratorPurpose_BASE,true,nullptr); 

    CalculateDelayResult delays = CalculateDelay(circuit,temp);

    region(temp){
      FixDelays(circuit,delays.edgesDelay);
    }

    res->fixedDelayCircuit = circuit;
    res->fixedDelayCircuit->name = res->name;
  } else {
    res->baseCircuit = CopyAccelerator(circuit,AcceleratorPurpose_BASE,true,nullptr);

    Pair<Accelerator*,SubMap*> p = Flatten(res->baseCircuit,99);
  
    res->flattenedBaseCircuit = p.first;
    res->flattenMapping = p.second;
  
    CalculateDelayResult delays = CalculateDelay(circuit,temp);

    region(temp){
      //OutputDebugDotGraph(circuit,STRING("BeforeFixDelay.dot"),temp);
      FixDelays(circuit,delays.edgesDelay);
      //OutputDebugDotGraph(circuit,STRING("AfterFixDelay.dot"),temp);
    }

    res->fixedDelayCircuit = circuit;
  }

  FillDeclarationWithAcceleratorValues(res,res->fixedDelayCircuit);
  FillDeclarationWithDelayType(res);

#if 1
    // TODO: Maybe this check should be done elsewhere
    for(FUInstance* ptr : circuit->allocated){
      if(ptr->multipleSamePortInputs){
        printf("Multiple inputs: %.*s\n",UNPACK_SS(name));
        break;
      }
    }
#endif
  
  res->staticUnits = PushHashmap<StaticId,StaticData>(permanent,1000); // TODO: Set correct number of elements

  // Start by collecting all the existing static allocated units in subinstances
  for(FUInstance* ptr : res->fixedDelayCircuit->allocated){
    FUInstance* inst = ptr;
    if(IsTypeHierarchical(inst->declaration)){
      for(auto pair : inst->declaration->staticUnits){
        StaticData newData = *pair.second;
        res->staticUnits->InsertIfNotExist(pair.first,newData);
      }
    }
    if(inst->isStatic){
      StaticId id = {};
      id.name = inst->name;
      id.parent = res;

      StaticData data = {};
      data.configs = inst->declaration->configs;
      res->staticUnits->InsertIfNotExist(id,data);
    }
  }

  return res;
}

struct Connection{
  FUInstance* input;
  FUInstance* mux;
  PortInstance unit;
};

FUDeclaration* RegisterIterativeUnit(Accelerator* accel,FUInstance* inst,int latency,String name){
  TEMP_REGION(temp,nullptr);
  TEMP_REGION(temp2,temp);

  Arena* perm = globalPermanent;
  FUInstance* node = inst;

  Assert(node);

  Array<Array<PortInstance>> inputs = GetAllInputs(node,temp);

  for(Array<PortInstance>& arr : inputs){
    Assert(arr.size <= 2); // Do not know what to do if size bigger than 2.
  }

  static String muxNames[] = {STRING("Mux1"),STRING("Mux2"),STRING("Mux3"),STRING("Mux4"),STRING("Mux5"),STRING("Mux6"),STRING("Mux7"),
                               STRING("Mux8"),STRING("Mux9"),STRING("Mux10"),STRING("Mux11"),STRING("Mux12"),STRING("Mux13"),STRING("Mux14"),STRING("Mux15"),
                               STRING("Mux16"),STRING("Mux17"),STRING("Mux18"),STRING("Mux19"),STRING("Mux20"),STRING("Mux21"),STRING("Mux22"),STRING("Mux23"),
                               STRING("Mux24"),STRING("Mux25"),STRING("Mux26"),STRING("Mux27"),STRING("Mux28"),STRING("Mux29"),STRING("Mux30"),STRING("Mux31"),STRING("Mux32"),STRING("Mux33"),STRING("Mux34"),
                               STRING("Mux35"),STRING("Mux36"),STRING("Mux37"),STRING("Mux38"),STRING("Mux39"),STRING("Mux40"),STRING("Mux41"),STRING("Mux42"),STRING("Mux43"),STRING("Mux44"),STRING("Mux45"),STRING("Mux46"),STRING("Mux47"),STRING("Mux48"),STRING("Mux49"),STRING("Mux50"),STRING("Mux51"),STRING("Mux52"),STRING("Mux53"),STRING("Mux54"),STRING("Mux55")};

  FUDeclaration* type = BasicDeclaration::timedMultiplexer;

#if 0
  String beforePath = PushDebugPath(temp,name,STRING("iterativeBefore.dot"));
  OutputGraphDotFile(accel,false,beforePath,temp);
#endif
  
  Array<Connection> conn = PushArray<Connection>(temp,inputs.size);
  Memset(conn,{});

  int muxInserted = 0;
  for(int i = 0; i < inputs.size; i++){
    Array<PortInstance>& arr = inputs[i];
    if(arr.size != 2){
      continue;
    }

    PortInstance first = arr[0];

    if(first.inst->declaration != BasicDeclaration::input){
      SWAP(arr[0],arr[1]);
    }

    Assert(arr[0].inst->declaration == BasicDeclaration::input);

    Assert(i < ARRAY_SIZE(muxNames));
    FUInstance* mux = CreateFUInstance(accel,type,muxNames[i]);

    RemoveConnection(accel,arr[0].inst,arr[0].port,node,i);
    RemoveConnection(accel,arr[1].inst,arr[1].port,node,i);

    ConnectUnits(arr[0],(PortInstance){mux,0},0);
    ConnectUnits(arr[1],(PortInstance){mux,1},0);

    ConnectUnits((PortInstance){mux,0},(PortInstance){node,i},0);

    conn[i].input = arr[0].inst;
    conn[i].unit = PortInstance{arr[1].inst,arr[1].port};
    conn[i].mux = mux;

    muxInserted += 1;
  }

  FUDeclaration* bufferType = BasicDeclaration::buffer;
  int buffersAdded = 0;
  for(int i = 0; i < inputs.size; i++){
    if(!conn[i].mux){
      continue;
    }

    FUInstance* unit = conn[i].unit.inst;
    Assert(i < unit->declaration->NumberInputs());

    if(unit->declaration->info.infos[0].inputDelays[i] == 0){
      continue;
    }

    FUInstance* buffer = CreateFUInstance(accel,bufferType,PushString(perm,"Buffer%d",buffersAdded++));

    buffer->bufferAmount = unit->declaration->info.infos[0].inputDelays[i] - BasicDeclaration::buffer->info.infos[0].outputLatencies[0];
    Assert(buffer->bufferAmount >= 0);
    SetStatic(accel,buffer);

    InsertUnit(accel,(PortInstance){conn[i].mux,0},conn[i].unit,(PortInstance){buffer,0});
  }

#if 0
  String afterPath = PushDebugPath(temp,name,STRING("iterativeBefore.dot"));
  OutputGraphDotFile(accel,true,afterPath,temp);
#endif
  
  FUDeclaration declaration = {};

  declaration = *inst->declaration; // By default, copy everything from unit
  declaration.name = PushString(perm,name);
  declaration.type = FUDeclarationType_ITERATIVE;

  FillDeclarationWithAcceleratorValues(&declaration,accel);

  // Kinda of a hack, for now
#if 0
  int min = std::min(node->inst->declaration->inputDelays.size,declaration.inputDelays.size);
  for(int i = 0; i < min; i++){
    declaration.inputDelays[i] = node->inst->declaration->inputDelays[i];
  }
#endif

  // TODO: We are not checking connections here, we are just assuming that unit is directly connected to out.
  //       Probably a bad idea but still do not have an example which would make it not ideal
  NOT_IMPLEMENTED("Need to give a second look after cleaning up the changes to AccelInfo");
  for(int i = 0; i < declaration.NumberOutputs(); i++){
    //declaration.baseConfig.outputLatencies[i] = latency * (node->declaration->baseConfig.outputLatencies[i] + 1);
  }

  // Values from iterative declaration
  declaration.baseCircuit = accel;
  declaration.fixedDelayCircuit = accel;

  declaration.staticUnits = PushHashmap<StaticId,StaticData>(perm,1000); // TODO: Set correct number of elements
  // Start by collecting all the existing static allocated units in subinstances
  for(FUInstance* ptr : accel->allocated){
    FUInstance* inst = ptr;
    if(IsTypeHierarchical(inst->declaration)){

      for(auto pair : inst->declaration->staticUnits){
        StaticData newData = *pair.second;
        declaration.staticUnits->InsertIfNotExist(pair.first,newData);
      }
    }
  }

  FUDeclaration* registeredType = RegisterFU(declaration);
  //registeredType->lat = node->declaration->baseConfig.outputLatencies[0];
  
  // Add this units static instances (needs be done after Registering the declaration because the parent is a pointer to the declaration)
  for(FUInstance* ptr : accel->allocated){
    FUInstance* inst = ptr;
    if(inst->isStatic){
      StaticId id = {};
      id.name = inst->name;
      id.parent = registeredType;

      StaticData data = {};
      data.configs = inst->declaration->configs;
      registeredType->staticUnits->InsertIfNotExist(id,data);
    }
  }

  return registeredType;
}

FUInstance* CreateOrGetInput(Accelerator* accel,String name,int portNumber){
  for(FUInstance* ptr : accel->allocated){
    FUInstance* inst = ptr;
    if(inst->declaration == BasicDeclaration::input && inst->portIndex == portNumber){
      return inst;
    }
  }

  FUInstance* inst = (FUInstance*) CreateFUInstance(accel,BasicDeclaration::input,name);
  inst->portIndex = portNumber;

  return inst;
}

FUInstance* CreateOrGetOutput(Accelerator* accel){
  for(FUInstance* ptr : accel->allocated){
    FUInstance* inst = ptr;
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


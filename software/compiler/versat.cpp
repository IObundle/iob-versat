#include "versat.hpp"

#include "globals.hpp"

#include "declaration.hpp"
#include "symbolic.hpp"
#include "templateEngine.hpp"
#include "utils.hpp"
#include "utilsCore.hpp"

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

SymbolicExpression* SymbolicExpressionFromVerilog(Expression* topExpr,Arena* out){
  FULL_SWITCH(topExpr->type){
  case Expression::UNDEFINED: {
    Assert(false);
  } break;
  case Expression::OPERATION: {
    SymbolicExpression* left = SymbolicExpressionFromVerilog(topExpr->expressions[0],out);
    SymbolicExpression* right = SymbolicExpressionFromVerilog(topExpr->expressions[1],out);
    
    switch(topExpr->op[0]){
    case '+':{
      return SymbolicAdd(left,right,out);
    } break;
    case '-':{
      return SymbolicSub(left,right,out);
    } break;
    case '*':{
      return SymbolicMult(left,right,out);
    } break;
    case '/':{
      return SymbolicDiv(left,right,out);
    } break;
    default:{
      // TODO: Better error message
      NOT_IMPLEMENTED("");
    } break;
    } 
  } break;
  case Expression::IDENTIFIER: {
    return PushVariable(out,topExpr->id);
  } break;
  case Expression::FUNCTION: {
    // TODO: Better error message and we probably can do more stuff here
    NOT_IMPLEMENTED("");
  } break;
  case Expression::LITERAL: {
    return PushLiteral(out,topExpr->val.number);
  } break;
} END_SWITCH();
  
  return {};
}

// TODO: Need to remake this function and probably ModuleInfo has the versat compiler change is made
FUDeclaration* RegisterModuleInfo(ModuleInfo* info,Arena* out){
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
  
  Array<ParameterExpression> instantiated = PushArray<ParameterExpression>(perm,info->defaultParameters.size);

  decl.parameters = PushArray<Parameter>(perm,instantiated.size);
  for(int i = 0; i < instantiated.size; i++){
    decl.parameters[i].name = info->defaultParameters[i].name;
    decl.parameters[i].valueExpr = SymbolicExpressionFromVerilog(info->defaultParameters[i].expr,perm);
  }
    
  for(int i = 0; i < instantiated.size; i++){
    ParameterExpression def = info->defaultParameters[i];

#if 1
    // TODO: We cannot remove this because the external memories are instantiated based on this value and we currently have no way of exporting this value.
    //       This essentially means that the accelerator is still dependent on the setup phase, but this only affects the datapath size and the datapath size is mostly a setup phase thing anyway, so it is not the worst. 
    if(CompareString(def.name,STRING("AXI_DATA_W"))){
      Expression* expr = PushStruct<Expression>(temp);

      expr->type = Expression::LITERAL;
      expr->id = def.name;
      expr->val = MakeValue(globalOptions.databusDataSize);

      def.expr = expr;
    }
#endif

    instantiated[i] = def;
  }

  auto literalOne = PushLiteral(temp,1);
  
  for(int i = 0; i < info->configs.size; i++){
    WireExpression& wire = info->configs[i];

    int size = EvalRange(wire.bitSize,instantiated);

    configs[i].name = info->configs[i].name;
    configs[i].bitSize = size;
    configs[i].isStatic = false;
    configs[i].stage = info->configs[i].stage;

    SymbolicExpression* top = SymbolicExpressionFromVerilog(wire.bitSize.top,temp);
    SymbolicExpression* bottom = SymbolicExpressionFromVerilog(wire.bitSize.bottom,temp);
    configs[i].sizeExpr = Normalize(SymbolicAdd(SymbolicSub(top,bottom,temp),literalOne,temp),out);
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
  
  decl.name = info->name;

  decl.info.infos = PushArray<MergePartition>(globalPermanent,1);
  decl.info.infos[0].inputDelays = info->inputDelays;
  decl.info.infos[0].outputLatencies = info->outputLatencies;
  
  decl.configs = configs;
  decl.states = states;
  decl.externalMemory = external;
  decl.numberDelays = info->nDelays;
  decl.nIOs = info->nIO;

  if(info->memoryMapped) {
    decl.memoryMapBits = EvalRange(info->memoryMappedBits,instantiated);
  }

  decl.implementsDone = info->hasDone;
  decl.signalLoop = info->signalLoop;

  if(info->isSource){
    decl.delayType = decl.delayType | DelayType::DelayType_SINK_DELAY;
  }

  // TODO: Is this something generic that we could move into meta code?
  for(AddressGenWireNames_GenType gen : AddressGenWireNames){
    bool isType = true;
    for(String str : gen.names){
      bool foundOne = false;
      for(Wire wire : configs){
        if(CompareString(str,wire.name)){
          foundOne = true;
          break;
        }
      }
      if(!foundOne){
        isType = false;
        break;
      }
    }

    if(isType){
      decl.supportedAddressGenType = (AddressGenType) (decl.supportedAddressGenType | gen.type);
    }
  }
  
  FUDeclaration* res = RegisterFU(decl);

  return res;
}

void FillDeclarationWithAcceleratorValues(FUDeclaration* decl,Accelerator* accel,Arena* out){
  TEMP_REGION(temp,out);
  TEMP_REGION(temp2,temp);

  // TODO: This function was mostly used to calculate the configInfo stuff, which now is gone.
  //       Need to see how much of this code is also useful for the merge stuff.
  //       (If after merge puts the correct values inside the accelInfo struct, if we can just call this function to compute the remaining data that needs to be computed).
  //       We can also move some of this computation to the accelInfo struct. Just see how things play out.
  
  AccelInfo val = CalculateAcceleratorInfo(accel,true,out);
  decl->info = val;
 
  decl->nIOs = val.nIOs;
  if(val.isMemoryMapped){
    decl->memoryMapBits = val.memoryMappedBits;
  }

  decl->externalMemory = PushArray<ExternalMemoryInterface>(out,val.externalMemoryInterfaces);
  int externalIndex = 0;
  for(FUInstance* ptr : accel->allocated){
    Array<ExternalMemoryInterface> arr = ptr->declaration->externalMemory;
    for(int i = 0; i < arr.size; i++){
      decl->externalMemory[externalIndex] = arr[i];
      decl->externalMemory[externalIndex].interface = externalIndex;
      externalIndex += 1;
    }
  }
    
  decl->configs = PushArray<Wire>(out,val.configs);
  decl->states = PushArray<Wire>(out,val.states);
  
  Hashmap<int,int>* staticsSeen = PushHashmap<int,int>(temp,val.sharedUnits);

  int configIndex = 0;
  for(AccelInfoIterator iter = StartIteration(&val); iter.IsValid(); iter = iter.Step()){
    InstanceInfo* unit = iter.CurrentUnit();
    bool isSimple = !unit->isComposite;

    Hashmap<String,SymbolicExpression*>* params = GetParametersOfUnit(unit->inst,temp);
    
    if(unit->isGloballyStatic){
      continue;
    }

    for(int i = 0; i < unit->individualWiresGlobalConfigPos.size; i++){
      int globalPos = unit->individualWiresGlobalConfigPos[i];

      if(globalPos < 0){
        continue;
      }
      
      if(!Empty(decl->configs[globalPos].name)){
        continue;
      }

      FUDeclaration* d = unit->decl;
      Wire wire = d->configs[i];
      decl->configs[configIndex] = wire;

      // TODO: We need to do this for state as well. And we probably want to make this more explicit.
      decl->configs[configIndex].sizeExpr = ReplaceVariables(wire.sizeExpr,params,out);
      decl->configs[configIndex++].name = PushString(out,"%.*s_%.*s",UNPACK_SS(unit->name),UNPACK_SS(wire.name));
    }
  }
  
  // TODO: This could be done based on the config offsets.
  //       Otherwise we are still basing code around static and shared logic when calculating offsets already does that for us.
  int stateIndex = 0;
  for(FUInstance* ptr : accel->allocated){
    FUInstance* inst = ptr;
    FUDeclaration* d = inst->declaration;

    for(Wire& wire : d->states){
      decl->states[stateIndex] = wire;

      decl->states[stateIndex].name = PushString(out,"%.*s_%.2d",UNPACK_SS(wire.name),stateIndex);
      stateIndex += 1;
    }
  }

  decl->signalLoop = val.signalLoop;

  Array<bool> belongArray = PushArray<bool>(out,accel->allocated.Size());
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

  res->parameters = PushArray<Parameter>(permanent,6);
  res->parameters[0] = {STRING("ADDR_W"),PushLiteral(permanent,32)};
  res->parameters[1] = {STRING("DATA_W"),PushLiteral(permanent,32)};
  res->parameters[2] = {STRING("DELAY_W"),PushLiteral(permanent,7)};
  res->parameters[3] = {STRING("AXI_ADDR_W"),PushLiteral(permanent,32)};
  res->parameters[4] = {STRING("AXI_DATA_W"),PushLiteral(permanent,32)};
  res->parameters[5] = {STRING("LEN_W"),PushLiteral(permanent,20)};
  
  if(circuit->allocated.Size() == 0){
    res->baseCircuit = circuit;
    res->fixedDelayCircuit = circuit;

    return res;
  } else if(options == SubUnitOptions_BAREBONES){
    // TODO: Need to add back the OutputDebugDotGraph calls
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
      FixDelays(circuit,delays.edgesDelay);
    }

    res->fixedDelayCircuit = circuit;
  }

  FillDeclarationWithAcceleratorValues(res,res->fixedDelayCircuit,permanent);
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
  for(FUInstance* inst : res->fixedDelayCircuit->allocated){
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
      data.decl = inst->declaration;
      data.configs = PushArray<Wire>(permanent,inst->declaration->configs.size);

      // We need to instantiate the verilog parameters in here.
      Hashmap<String,SymbolicExpression*>* params = GetParametersOfUnit(inst,temp);
      for(int i = 0; i <  inst->declaration->configs.size; i++){
        Wire w  =  inst->declaration->configs[i];

        data.configs[i] = w;
        data.configs[i].sizeExpr = ReplaceVariables(w.sizeExpr,params,permanent);
      }

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

  FillDeclarationWithAcceleratorValues(&declaration,accel,globalPermanent);

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
      data.decl = inst->declaration;
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

bool IsGlobalParameter(String name){
  if(CompareString(name,"AXI_ADDR_W") ||
     CompareString(name,"AXI_DATA_W") ||
     //CompareString(name,"ADDR_W")     ||
     CompareString(name,"DATA_W")     ||
     CompareString(name,"LEN_W")      ||
     CompareString(name,"DELAY_W")){
    return true;
  }

  return false;
}

Hashmap<String,SymbolicExpression*>* GetParametersOfUnit(FUInstance* inst,Arena* out){
  FUDeclaration* decl = inst->declaration;

  auto map = PushHashmap<String,SymbolicExpression*>(out,decl->parameters.size);
  
  for(int i = 0; i < decl->parameters.size; i++){
    Parameter param = decl->parameters[i];
    ParameterValue val = inst->parameterValues[i];

    if(IsGlobalParameter(param.name)){
      map->Insert(param.name,PushVariable(out,param.name));
    } else {
      if(Empty(val.val)){
        map->Insert(param.name,param.valueExpr);
      } else {
        map->Insert(param.name,ParseSymbolicExpression(val.val,out));
      }
    }
  }

  return map;
}

#define DELAY_SIZE 7

Array<WireInformation> CalculateWireInformation(Pool<FUInstance> nodes,Hashmap<StaticId,StaticData>* staticUnits,int addrOffset,Arena* out){
  TEMP_REGION(temp,out);
  
  auto list = PushArenaList<WireInformation>(temp);

  SymbolicExpression* expr = PushLiteral(temp,0);
  
  int configBit = 0;
  int addr = addrOffset;
  for(auto n : nodes){
    for(Wire w : n->declaration->configs){
      WireInformation info = {};
      info.wire = w;
      info.configBitStart = configBit;
      configBit += w.bitSize;
      info.addr = 4 * addr++;

      info.bitExpr = Normalize(expr,out);
      
      expr = SymbolicAdd(info.bitExpr,w.sizeExpr,temp);
      *list->PushElem() = info;
    }
  }
  for(Pair<StaticId,StaticData*> n : staticUnits){
    FUDeclaration* decl = n.second->decl;
    Hashmap<String,SymbolicExpression*>* defaultParams = PushHashmap<String,SymbolicExpression*>(temp,decl->parameters.size);

    for(Parameter p : decl->parameters){
      if(IsGlobalParameter(p.name)){
        defaultParams->Insert(p.name,PushVariable(out,p.name));
      } else {
        defaultParams->Insert(p.name,p.valueExpr);
      }
    }
    
    for(Wire w : n.data->configs){
      WireInformation info = {};
      info.wire = w;
      info.wire.name = ReprStaticConfig(n.first,&w,out);
      info.wire.sizeExpr = ReplaceVariables(w.sizeExpr,defaultParams,out);

      info.configBitStart = configBit;
      configBit += w.bitSize;
      info.addr = 4 * addr++;
      info.isStatic = true;

      info.bitExpr = Normalize(expr,out);

      expr = SymbolicAdd(info.bitExpr,info.wire.sizeExpr,temp);

      *list->PushElem() = info;
    }
  }
  int delaysInserted = 0;
  for(auto n : nodes){
    for(int i = 0; i < n->declaration->NumberDelays(); i++){
      Wire wire = {};
      wire.bitSize = DELAY_SIZE;
      wire.name = PushString(out,"Delay%d",delaysInserted++);
      wire.stage = VersatStage_COMPUTE;

      WireInformation info = {};
      info.wire = wire;
      info.wire.sizeExpr = PushVariable(out,S8("DELAY_W"));
      
      info.configBitStart = configBit;
      configBit += DELAY_SIZE;
      info.addr = 4 * addr++;

      info.bitExpr = Normalize(expr,out);

      expr = SymbolicAdd(info.bitExpr,PushVariable(temp,S8("DELAY_W")),temp);

      *list->PushElem() = info;
    }
  }
  
  return PushArrayFromList(out,list);
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


#include "versat.hpp"

#include "embeddedData.hpp"
#include "globals.hpp"

#include "declaration.hpp"
#include "symbolic.hpp"
#include "templateEngine.hpp"
#include "utils.hpp"
#include "utilsCore.hpp"
#include "verilogParsing.hpp"

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

// TODO: Need to remake this function and probably ModuleInfo as the versat compiler change is made
Opt<FUDeclaration*> RegisterModuleInfo(ModuleInfo* info,Arena* out){
  TEMP_REGION(temp,nullptr);

  Arena* perm = globalPermanent;
  
  // Check same name
  for(FUDeclaration* decl : globalDeclarations){
    if(CompareString(decl->name,info->name)){
      printf("Found a module with a same name (%.*s). Cannot proceed",UN(info->name));
      return {};
    }
  }

  FUDeclaration decl = {};

  Array<Wire> configs = PushArray<Wire>(perm,info->configs.size);
  Array<Wire> states = PushArray<Wire>(perm,info->states.size);
  Array<ExternalMemoryInterface> external = PushArray<ExternalMemoryInterface>(perm,info->externalInterfaces.size);
  Array<ExternalMemorySymbolic> extSym = PushArray<ExternalMemorySymbolic>(perm,info->externalInterfaces.size);
  
  Array<ParameterExpression> instantiated = PushArray<ParameterExpression>(perm,info->defaultParameters.size);

  decl.parameters = PushArray<Parameter>(perm,instantiated.size);
  for(int i = 0; i < instantiated.size; i++){
    decl.parameters[i].name = info->defaultParameters[i].name;
    decl.parameters[i].valueExpr = SymbolicExpressionFromVerilog(info->defaultParameters[i].expr,perm);
    decl.parameters[i].flags = info->defaultParameters[i].flags;
  }

  decl.inputSize = PushArray<SymbolicExpression*>(out,info->inputs.size);
  decl.outputSize = PushArray<SymbolicExpression*>(out,info->outputs.size);

  for(int i = 0; i < info->inputs.size; i++){
    decl.inputSize[i] = SymbolicExpressionFromVerilog(info->inputs[i].range,out);
  }
  for(int i = 0; i < info->outputs.size; i++){
    decl.outputSize[i] = SymbolicExpressionFromVerilog(info->outputs[i].range,out);
  }
    
  for(int i = 0; i < instantiated.size; i++){
    ParameterExpression def = info->defaultParameters[i];

    // TODO: We cannot remove this because the external memories are instantiated based on this value and we currently have no way of exporting this value.
    //       This essentially means that the accelerator is still dependent on the setup phase, but this only affects the datapath size and the datapath size is mostly a setup phase thing anyway, so it is not the worst. 
    if(CompareString(def.name,"AXI_DATA_W")){
      Expression* expr = PushStruct<Expression>(temp);

      expr->type = Expression::LITERAL;
      expr->id = def.name;
      expr->val = MakeValue(globalOptions.databusDataSize);

      def.expr = expr;
    }

    instantiated[i] = def;
  }

  for(int i = 0; i < info->configs.size; i++){
    WireExpression& wire = info->configs[i];

    int size = EvalRange(wire.bitSize,instantiated);

    configs[i].name = info->configs[i].name;
    configs[i].bitSize = size;
    configs[i].stage = info->configs[i].stage;

    configs[i].sizeExpr = SymbolicExpressionFromVerilog(wire.bitSize,out);
  }

  for(int i = 0; i < info->states.size; i++){
    WireExpression& wire = info->states[i];

    int size = EvalRange(wire.bitSize,instantiated);

    states[i].name = info->states[i].name;
    states[i].bitSize = size;
    states[i].stage = info->states[i].stage;

    states[i].sizeExpr = SymbolicExpressionFromVerilog(wire.bitSize,out);
  }

  for(int i = 0; i < info->externalInterfaces.size; i++){
    ExternalMemoryInterfaceExpression& expr = info->externalInterfaces[i];

    external[i].interface = expr.interface;
    external[i].type = expr.type;

    extSym[i].interface = expr.interface;
    extSym[i].type = expr.type;
    
    switch(expr.type){
    case ExternalMemoryType::ExternalMemoryType_2P:{
      external[i].tp.bitSizeIn = EvalRange(expr.tp.bitSizeIn,instantiated);
      external[i].tp.bitSizeOut = EvalRange(expr.tp.bitSizeOut,instantiated);
      external[i].tp.dataSizeIn = EvalRange(expr.tp.dataSizeIn,instantiated);
      external[i].tp.dataSizeOut = EvalRange(expr.tp.dataSizeOut,instantiated);
    }break;
    case ExternalMemoryType::ExternalMemoryType_DP:{
      for(int ii = 0; ii < 2; ii++){
        external[i].dp[ii].bitSize = EvalRange(expr.dp[ii].bitSize,instantiated);
        external[i].dp[ii].dataSizeIn = EvalRange(expr.dp[ii].dataSizeIn,instantiated);
        external[i].dp[ii].dataSizeOut = EvalRange(expr.dp[ii].dataSizeOut,instantiated);
      }
    }break;
    }

    switch(expr.type){
    case ExternalMemoryType::ExternalMemoryType_2P:{
      extSym[i].tp.bitSizeIn = SymbolicExpressionFromVerilog(expr.tp.bitSizeIn,out);
      extSym[i].tp.bitSizeOut = SymbolicExpressionFromVerilog(expr.tp.bitSizeOut,out);
      extSym[i].tp.dataSizeIn = SymbolicExpressionFromVerilog(expr.tp.dataSizeIn,out);
      extSym[i].tp.dataSizeOut = SymbolicExpressionFromVerilog(expr.tp.dataSizeOut,out);
    }break;
    case ExternalMemoryType::ExternalMemoryType_DP:{
      for(int ii = 0; ii < 2; ii++){
        extSym[i].dp[ii].bitSize = SymbolicExpressionFromVerilog(expr.dp[ii].bitSize,out);
        extSym[i].dp[ii].dataSizeIn = SymbolicExpressionFromVerilog(expr.dp[ii].dataSizeIn,out);
        extSym[i].dp[ii].dataSizeOut = SymbolicExpressionFromVerilog(expr.dp[ii].dataSizeOut,out);
      }
    }break;
    }
  }
  

  decl.name = info->name;

  decl.info.infos = PushArray<MergePartition>(globalPermanent,1);
  decl.info.infos[0].inputDelays = Extract(info->inputs,out,&PortInfo::delay);
  decl.info.infos[0].outputLatencies = Extract(info->outputs,out,&PortInfo::delay);
  
  decl.configs = configs;
  decl.states = states;
  decl.externalMemory = external;
  decl.externalMemorySymbol = extSym;
  decl.numberDelays = info->nDelays;
  decl.info.nIOs = info->nIO;

  if(info->memoryMapped) {
    decl.info.memMapBits = EvalRange(info->memoryMappedBits,instantiated);
  }

  decl.singleInterfaces = info->singleInterfaces;
  
  if(info->isSource){
    decl.delayType = decl.delayType | DelayType::DelayType_SINK_DELAY;
  }

  auto FollowsInterface = [](Array<Wire> configs,Array<String> interfaceWireNames) -> bool{
    // No point in even checking
    if(configs.size < interfaceWireNames.size){
      return false;
    }
    
    for(String str : interfaceWireNames){
      bool found = false;
      for(Wire w : configs){
        if(CompareString(w.name,str)){
          found = true;
          break;
        }
      }

      if(!found){
        return false;
      }
    }

    return true;
  };
  
  bool isGenLike = FollowsInterface(configs,META_AddressGenBaseParameters_Members);
  bool isExternLike = FollowsInterface(configs,META_AddressVParameters_Members);
  bool isMemLike = FollowsInterface(configs,META_AddressMemParameters_Members);

  Array<String> configNames = Extract(configs,temp,&Wire::name);

  auto CountLoops = [configNames](Array<String> extraLoopsFormats){
    TEMP_REGION(temp,nullptr);
    
    int loops = 2;

    while(true){
      bool containsAllNames = true;
      for(String format : extraLoopsFormats){
        String inst = PushString(temp,format.data,loops);

        if(!Contains(configNames,inst)){
          containsAllNames = false;
          break;
        }
      }

      if(containsAllNames){
        loops += 1;
      } else {
        break;
      }
    }

    return loops - 1;
  };

  if(isExternLike){
    decl.supportedAddressGen.type = AddressGenType_READ;
    decl.supportedAddressGen.loopsSupported = CountLoops(AddressGenExtraFormat);
  } else if(isGenLike){
    decl.supportedAddressGen.type = AddressGenType_GEN;
    decl.supportedAddressGen.loopsSupported = CountLoops(AddressGenExtraFormat);
  } else if(isMemLike){
    decl.supportedAddressGen.type = AddressGenType_MEM;
    decl.supportedAddressGen.loopsSupported = CountLoops(AddressGenMemExtraFormat);
  }

  FUDeclaration* res = RegisterFU(decl);

  return res;
}

void FillDeclarationWithAcceleratorValues(FUDeclaration* decl,Accelerator* accel,Arena* out,bool calculateOrder){
  TEMP_REGION(temp,out);
  TEMP_REGION(temp2,temp);

  // TODO: This function was mostly used to calculate the configInfo stuff, which now is gone.
  //       Need to see how much of this code is also useful for the merge stuff.
  //       (If after merge puts the correct values inside the accelInfo struct, if we can just call this function to compute the remaining data that needs to be computed).
  //       We can also move some of this computation to the accelInfo struct. Just see how things play out.
  
  AccelInfo val = CalculateAcceleratorInfo(accel,true,out,calculateOrder);
  decl->info = val;

  for(int i = 0; i < val.infos.size; i++){
    for(auto iter = StartIteration(&val,i); iter.IsValid(); iter = iter.Next()){
      InstanceInfo* info = iter.CurrentUnit();

      info->parentTypeName = decl->name;
    }
  }
 
  // All the single interfaces are simple of propagating. We can just do an OR of everything.
  for(FUInstance* ptr : accel->allocated){
    decl->singleInterfaces |= ptr->declaration->singleInterfaces;
  }

  // TODO: Check the TODO in the OutputCircuitSource. Basically, because the wrapper that interacts with the Verilated unit is not capable of handling these missing interfaces, we are forcing the modules to have them for now.
  //       If we fix the wrapper generation to support the lack of these interfaces, we can remove this.
  decl->singleInterfaces |= SingleInterfaces_RUN;
  decl->singleInterfaces |= SingleInterfaces_RUNNING;
  decl->singleInterfaces |= SingleInterfaces_CLK;
  decl->singleInterfaces |= SingleInterfaces_RESET;

  // NOTE: We are "instantiating" the memories in here since we need to know the actual size of the wires in response to the actual values 
  decl->externalMemorySymbol = PushArray<ExternalMemorySymbolic>(out,val.externalMemoryInterfaces);
  {
    int externalIndex = 0;
    for(FUInstance* ptr : accel->allocated){
      Array<ExternalMemorySymbolic> arr = ptr->declaration->externalMemorySymbol;

      Hashmap<String,SymbolicExpression*>* params = GetParametersOfUnit(ptr,temp);

      for(int i = 0; i < arr.size; i++){
        ExternalMemorySymbolic sym = {};

        sym.interface = arr[i].interface;
        sym.type = arr[i].type;
          
        switch(arr[i].type){
        case ExternalMemoryType_2P:{
          sym.tp.bitSizeIn = ReplaceVariables(arr[i].tp.bitSizeIn,params,out);
          sym.tp.dataSizeIn = ReplaceVariables(arr[i].tp.dataSizeIn,params,out);
          sym.tp.bitSizeOut = ReplaceVariables(arr[i].tp.bitSizeOut,params,out);
          sym.tp.dataSizeOut = ReplaceVariables(arr[i].tp.dataSizeOut,params,out);
        } break;
        case ExternalMemoryType_DP:{
          sym.dp[0].bitSize = ReplaceVariables(arr[i].dp[0].bitSize,params,out);
          sym.dp[0].dataSizeIn = ReplaceVariables(arr[i].dp[0].dataSizeIn,params,out);
          sym.dp[0].dataSizeOut = ReplaceVariables(arr[i].dp[0].dataSizeOut,params,out);
          sym.dp[1].bitSize = ReplaceVariables(arr[i].dp[1].bitSize,params,out);
          sym.dp[1].dataSizeIn = ReplaceVariables(arr[i].dp[1].dataSizeIn,params,out);
          sym.dp[1].dataSizeOut = ReplaceVariables(arr[i].dp[1].dataSizeOut,params,out);
        } break;
        }

        decl->externalMemorySymbol[externalIndex] = sym;
        decl->externalMemorySymbol[externalIndex].interface = externalIndex;
        externalIndex += 1;
      }
    }
  }

  decl->externalMemory = PushArray<ExternalMemoryInterface>(out,val.externalMemoryInterfaces);
  {
    int externalIndex = 0;
    for(FUInstance* ptr : accel->allocated){
      Array<ExternalMemoryInterface> arr = ptr->declaration->externalMemory;
      for(int i = 0; i < arr.size; i++){
        decl->externalMemory[externalIndex] = arr[i];
        decl->externalMemory[externalIndex].interface = externalIndex;
        externalIndex += 1;
      }
    }
  }
    
  // nocheckin : TODO: We have AccelInfo calculate this stuff meaning that we can just 
  //                   get the data directly from there and remove this part. I think.
  decl->configs = PushArray<Wire>(out,val.configs);
  decl->states = PushArray<Wire>(out,val.states);

  int configIndex = 0;
  for(AccelInfoIterator iter = StartIteration(&val); iter.IsValid(); iter = iter.Step()){
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
      
      if(!Empty(decl->configs[globalPos].name)){
        continue;
      }

      Wire wire = unit->configs[i];
      decl->configs[configIndex] = wire;

      // TODO: We need to do this for state as well. And we probably want to make this more explicit.
      decl->configs[configIndex].sizeExpr = ReplaceVariables(wire.sizeExpr,params,out);
      decl->configs[configIndex++].name = PushString(out,"%.*s_%.*s",UN(unit->name),UN(wire.name));
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

      decl->states[stateIndex].name = PushString(out,"%.*s_%.2d",UN(wire.name),stateIndex);
      stateIndex += 1;
    }
  }

  if(val.signalLoop){
    decl->singleInterfaces |= SingleInterfaces_SIGNAL_LOOP;
  }

  Array<bool> belongArray = PushArray<bool>(out,accel->allocated.Size());
  Memset(belongArray,true);

  decl->numberDelays = val.delays;
}

void FillDeclarationWithDelayType(FUDeclaration* decl){
  // TODO: Change unit delay type inference. Only care about delay type to upper levels.
  // Type source only if a source unit is connected to out. Type sink only if there is a input to sink connection
  bool hasSourceDelay = false;
  bool hasSinkDelay = false;

  for(FUInstance* ptr : decl->fixedDelayCircuit->allocated){
    FUInstance* inst = ptr;
    if(inst->declaration->type == FUDeclarationType_SPECIAL){
      continue;
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
}

FUDeclaration* RegisterSubUnit(Accelerator* circuit,SubUnitOptions options){
  DEBUG_PATH("RegisterSubUnit");
  DEBUG_PATH(circuit->name);

  TEMP_REGION(temp,nullptr);
  TEMP_REGION(temp2,temp);

  Arena* permanent = globalPermanent;

  // We receive the circuit with all the instances. We also have any user provided param inside the instances themselves. The FUInstance contains the params.
  // Any part of this code needs to instantiate everything that is a symbolic expression with those parameters, otherwise we run into trouble. And by everything I do mean everything.

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
  res->parameters[0] = {"ADDR_W",PushLiteral(permanent,32)};
  res->parameters[1] = {"DATA_W",PushLiteral(permanent,32)};
  res->parameters[2] = {"DELAY_W",PushLiteral(permanent,7)};
  res->parameters[3] = {"AXI_ADDR_W",PushLiteral(permanent,32)};
  res->parameters[4] = {"AXI_DATA_W",PushLiteral(permanent,32)};
  res->parameters[5] = {"LEN_W",PushLiteral(permanent,20)};
  
  bool containsMerge = false;

  for(FUInstance* inst : circuit->allocated){
    if(inst->declaration->type == FUDeclarationType_MERGED){
      containsMerge = true;
    }
  }

  if(containsMerge)
    printf("Contains Merged\n");
  
  if(circuit->allocated.Size() == 0){
    res->baseCircuit = circuit;
    res->fixedDelayCircuit = circuit;

    return res;
  } else if(options == SubUnitOptions_BAREBONES){
    // TODO: Need to add back the OutputDebugDotGraph calls
    res->baseCircuit = CopyAccelerator(circuit,AcceleratorPurpose_BASE,true); 

    CalculateDelayResult delays = CalculateDelay(circuit,temp);

    region(temp){
      FixDelays(circuit,delays.edgesDelay);
    }

    res->fixedDelayCircuit = circuit;
    res->fixedDelayCircuit->name = res->name;
  } else {
    res->baseCircuit = CopyAccelerator(circuit,AcceleratorPurpose_BASE,true);

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
      printf("Multiple inputs: %.*s\n",UN(name));
      break;
    }
  }
#endif

  // Nothing can depend on data from the instances or the declaration otherwise how can we instantiate anything? We need to change the data in order to "instantiate" something.

  res->staticUnits = PushHashmap<StaticId,StaticData>(permanent,1000); // TODO: Set correct number of elements

  // We have to instantiate the static units because the parameters of the static wires depend on the parameters of the instance themselves.
  // Alternatively, can we instantiate the parameters on the AccelInfo and let it work that way?

  // NOTE: Basically, we cannot just pick the original unit wires and lift them, because these parameters might depend on the.
  // NOTE: The more important issue is that we are not instantiate parameters anywhere. 

  // If Module A() instantiates unit B with parameter C=D (whose default is F, we cannot keep carrying the F param. We need to carry the D parameter value instead.

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

  if(res->baseCircuit)
    DebugRegionOutputDotGraph(res->baseCircuit,"BaseCircuit");
  if(res->fixedDelayCircuit)
    DebugRegionOutputDotGraph(res->fixedDelayCircuit,"FixedDelay");
  if(res->flattenedBaseCircuit)
    DebugRegionOutputDotGraph(res->flattenedBaseCircuit,"FlattenedBase");

  return res;
}

#if 0
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

  // TODO: Very crude hack.
  static String muxNames[] = {"Mux1","Mux2","Mux3","Mux4","Mux5","Mux6","Mux7",
                               "Mux8","Mux9","Mux10","Mux11","Mux12","Mux13","Mux14","Mux15",
                               "Mux16","Mux17","Mux18","Mux19","Mux20","Mux21","Mux22","Mux23",
                               "Mux24","Mux25","Mux26","Mux27","Mux28","Mux29","Mux30","Mux31","Mux32","Mux33","Mux34"
                               ,"Mux35","Mux36","Mux37","Mux38","Mux39","Mux40","Mux41","Mux42","Mux43","Mux44"
                               ,"Mux45","Mux46","Mux47","Mux48","Mux49","Mux50","Mux51","Mux52","Mux53"
                               ,"Mux54","Mux55"};

  FUDeclaration* type = BasicDeclaration::timedMultiplexer;

#if 0
  String beforePath = PushDebugPath(temp,name,"iterativeBefore.dot");
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
  String afterPath = PushDebugPath(temp,name,"iterativeBefore.dot");
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
#endif

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

  FUInstance* inst = (FUInstance*) CreateFUInstance(accel,BasicDeclaration::output,"out");
  return inst;
}

int GetInputPortNumber(FUInstance* inputInstance){
    Assert(inputInstance->declaration == BasicDeclaration::input);

  return ((FUInstance*) inputInstance)->portIndex;
}

bool IsGlobalParameter(String name){
  if(CompareString(name,"AXI_ADDR_W") ||
     CompareString(name,"AXI_DATA_W") ||
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
    SymbolicExpression* val = inst->parameterValues[i].val;

    if(IsGlobalParameter(param.name)){
      map->Insert(param.name,PushVariable(out,param.name));
    } else {
      if(val){
        map->Insert(param.name,val);
      } else {
        map->Insert(param.name,param.valueExpr);
      }
    }
  }

  return map;
}

Array<WireInformation> CalculateWireInformation(Pool<FUInstance> nodes,Hashmap<StaticId,StaticData>* staticUnits,int addrOffset,Arena* out){
  TEMP_REGION(temp,out);
  
  auto list = PushList<WireInformation>(temp);

  SymbolicExpression* expr = PushLiteral(temp,0);
  
  int addr = addrOffset;
  for(auto n : nodes){
    for(Wire w : n->declaration->configs){
      WireInformation info = {};
      info.wire = w;
      info.addr = 4 * addr++; // TODO: This 4 is because we are using addresses that are byte aligned in a 32 bit system. We could make this proper instead of hardcoded.

      info.startBitExpr = Normalize(expr,out);
      
      expr = SymbolicAdd(info.startBitExpr,w.sizeExpr,temp);
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

      info.addr = 4 * addr++;
      info.isStatic = true;

      info.startBitExpr = Normalize(expr,out);

      expr = SymbolicAdd(info.startBitExpr,info.wire.sizeExpr,temp);

      *list->PushElem() = info;
    }
  }

  int defaultDelaySize = 7;
  int delaysInserted = 0;
  for(auto n : nodes){
    for(int i = 0; i < n->declaration->NumberDelays(); i++){
      Wire wire = {};
      wire.bitSize = defaultDelaySize;
      wire.name = PushString(out,"Delay%d",delaysInserted++);
      wire.stage = VersatStage_COMPUTE;

      WireInformation info = {};
      info.wire = wire;
      info.wire.sizeExpr = PushVariable(out,"DELAY_W");
      
      info.addr = 4 * addr++;

      info.startBitExpr = Normalize(expr,out);

      expr = SymbolicAdd(info.startBitExpr,PushVariable(temp,"DELAY_W"),temp);

      *list->PushElem() = info;
    }
  }
  
  return PushArray(out,list);
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


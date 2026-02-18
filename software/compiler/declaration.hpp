#pragma once

#include "utils.hpp"
#include "verilogParsing.hpp"
#include "configurations.hpp"

struct FUInstance;
struct Edge;
struct SymbolicExpression;

typedef Hashmap<FUInstance*,FUInstance*> InstanceMap;
typedef Hashmap<Edge,Edge> EdgeMap;

// NOTE: Delay type is not really needed anymore because we can figure out the delay of a unit by: wether it contains inputs and outputs, the position on the graph and if we eventually add (input and output delay) whether it contains those as well.
//       After implementing input and output delay, retire DelayType
enum DelayType {
  DelayType_BASE               = 0x0,
  DelayType_SINK_DELAY         = 0x1,
  DelayType_SOURCE_DELAY       = 0x2,
  DelayType_COMPUTE_DELAY      = 0x4
};
#define CHECK_DELAY(inst,T) ((inst->declaration->delayType & T) == T)

inline DelayType operator|(DelayType a, DelayType b)
{return static_cast<DelayType>(static_cast<int>(a) | static_cast<int>(b));}

enum FUDeclarationType{
  FUDeclarationType_SINGLE,
  FUDeclarationType_COMPOSITE,
  FUDeclarationType_SPECIAL,
  FUDeclarationType_MERGED,
  FUDeclarationType_ITERATIVE
};

struct Parameter{
  String name;
  SymbolicExpression* valueExpr;
  ParamFlags flags;
};

// TODO: A lot of duplicated data exists since the change to merge.

// TODO: There is a lot of crux between parsing and creating the FUDeclaration for composite accelerators 
//       the FUDeclaration should be composed of something that is in common to all of them.
// NOTE: A FUDeclaration represents a concrete type, although the size of stuff might depend on parameters.
//       The general structure is fixed (amount of inputs/outputs and so on) but the size is not.
struct FUDeclaration{
  String name;

  // These always exist, regardless of merge info 
  Array<Wire> configs;
  Array<Wire> states;
  
  // TODO: Need to calculate these for hierarchical and merge units. For now only works for base units.
  //       After solving this, check the TODO for the output of testbench
  Array<SymbolicExpression*> inputSize;
  Array<SymbolicExpression*> outputSize;

  AccelInfo info;
  
  int numberDelays;
  Array<Parameter> parameters;

  // TODO: Eventually remove external expression and external memory and only keep externalMemorySymbol
  Array<ExternalMemoryInterfaceExpression> externalExpressionMemory;
  Array<ExternalMemoryInterface> externalMemory;
  Array<ExternalMemorySymbolic> externalMemorySymbol;
  
  // Stores different accelerators depending on properties we want. Mostly in relation to merge, because we want to use baseCircuit when doing a merge operation.
  // TODO: I think one problem that I keep encountering is the fact that we do not want to access the original graphs anymore after registering the declaration (unless for merge purposes).
  // TODO: Basically, because of stuff like merge and such, we need to access the AccelInfo and only the AccelInfo.

  Accelerator* baseCircuit; // For merge, baseCircuit contains muxes but not buffers.
  Accelerator* fixedDelayCircuit;
  Accelerator* flattenedBaseCircuit;
  
  String operation;

  SubMap* flattenMapping;

  AddressGenInst supportedAddressGen;
  
  int lat; // TODO: For now this is only for iterative units. Would also useful to have a standardized way of computing this from the graph and then compute it when needed. 
  
  Hashmap<StaticId,StaticData>* staticUnits;
  
  // TODO: We mostly do not use this. Furthermore we could just store this info inside the FUInstances, where we store a arrayBaseName or something similar.
  //Array<Pair<String,int>> definitionArrays;
  
  FUDeclarationType type;
  DelayType delayType;

  SingleInterfaces singleInterfaces;
  bool isOperation;
  
  // Simple access functions
  int NumberInputs(){
    if(info.infos.size > 0){
      return info.infos[0].inputDelays.size;
    } else {
      return 0;
    }
  };

  int NumberOutputs(){
    if(info.infos.size > 0){
      return info.infos[0].outputLatencies.size;
    } else {
      return 0;
    }
  };

  int NumberConfigs(){return configs.size;}
  int NumberStates(){return states.size;}
  int NumberDelays(){return numberDelays;};

  int MergePartitionSize(){
    return info.infos.size;
  };

  // TODO: Probably better to see all the outputs and all the infos, at the very least in Debug mode.
  // NOTE: This only works because operations only have one output.
  bool IsCombinatorialOperation(){
    bool res = (isOperation && info.infos[0].outputLatencies[0] == 0);
    return res;
  }
  bool IsSequentialOperation(){
    bool res = (isOperation && info.infos[0].outputLatencies[0] != 0);
    return res;
  }

  bool IsModularLike(){
    bool res = (type == FUDeclarationType_COMPOSITE || type == FUDeclarationType_MERGED);
    return res;
  }
  
  Array<int> GetOutputLatencies(){
    if(info.infos.size > 0){
      return info.infos[0].outputLatencies;
    } else {
      return {};
    }
  }

  Array<int> GetInputDelays(){
    if(info.infos.size > 0){
      return info.infos[0].inputDelays;
    } else {
      return {};
    }
  }
};

// Simple operations should also be stored here.
namespace BasicDeclaration{
  extern FUDeclaration* variableBuffer;
  extern FUDeclaration* fixedBuffer;
  extern FUDeclaration* input;
  extern FUDeclaration* output;
  extern FUDeclaration* multiplexer;
  extern FUDeclaration* combMultiplexer;
  extern FUDeclaration* timedMultiplexer;
  extern FUDeclaration* stridedMerge;
  extern FUDeclaration* pipelineRegister;
}

FUDeclaration* RegisterFU(FUDeclaration declaration);
FUDeclaration* GetTypeByName(String str);
FUDeclaration* GetTypeByNameOrFail(String name);
void InitializeSimpleDeclarations();
bool HasMultipleConfigs(FUDeclaration* decl);
// Because of merge, we need units that can delay the datapath for different values depending on the datapath that is being configured.

// ======================================
// Declaration inspection

Wire* GetConfigWireByName(FUDeclaration* decl,String name);

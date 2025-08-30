#pragma once

#include "configurations.hpp"

struct FUInstance;
struct FUDeclaration;
struct Edge;
struct InstanceInfo;

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

/*
// TODO: Finish making a FUType. FUDeclaration should be the instance of a type and should contain a pointer to the type that contains all the non parameterizable values.
// NOTE: Inputs and outputs should also be parameterizable. Think multiplexers and stuff.
  struct FUType{
  String name;

  Array<int> inputDelays;
  Array<int> outputLatencies;

  Array<WireExpression> configs;
  Array<WireExpression> states;

  Array<ExternalMemoryInterfaceExpression> externalMemory;
  }
*/

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
};

// TODO: A lot of duplicated data exists since the change to merge.

// TODO: There is a lot of crux between parsing and creating the FUDeclaration for composite accelerators 
//       the FUDeclaration should be composed of something that is in common to all of them.
struct FUDeclaration{
  String name;

  // These always exist, regardless of merge info 
  Array<Wire> configs;
  Array<Wire> states;

  AccelInfo info;
  
  int numberDelays;
  Array<Parameter> parameters;
  
  Opt<int> memoryMapBits; // 0 is a valid memory map size, so optional indicates that no memory map exists
  int nIOs;

  Array<ExternalMemoryInterfaceExpression> externalExpressionMemory;
  Array<ExternalMemoryInterface> externalMemory;

  // Stores different accelerators depending on properties we want. Mostly in relation to merge, because we want to use baseCircuit when doing a merge operation.
  Accelerator* baseCircuit;
  Accelerator* fixedDelayCircuit;
  Accelerator* flattenedBaseCircuit;
  
  const char* operation;

  SubMap* flattenMapping;

  AddressGenType supportedAddressGenType;
  
  int lat; // TODO: For now this is only for iterative units. Would also useful to have a standardized way of computing this from the graph and then compute it when needed. 
  
  Hashmap<StaticId,StaticData>* staticUnits;

  Array<Pair<String,int>> definitionArrays;
  
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

  // This only works for base units.
  // TODO: When things start settling in, need to move all these calculations to a specific file.
  //       Any data that needs to be computed should have a simple function and all off these should be organized to be all in one file.
  int NumberConfigs(){return configs.size;}
  int NumberStates(){return states.size;}
  int NumberDelays(){return numberDelays;};

  int MergePartitionSize(){
    return info.infos.size;
  };

  // TODO: Probably better to see all the outputs and all the infos, at the very least in Debug mode.
  bool IsCombinatorialOperation(){
    bool res = (isOperation && info.infos[0].outputLatencies[0] == 0);
    return res;
  }
  bool IsSequentialOperation(){
    bool res = (isOperation && info.infos[0].outputLatencies[0] != 0);
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
  extern FUDeclaration* buffer;
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
bool HasVariableDelay(FUDeclaration* decl);

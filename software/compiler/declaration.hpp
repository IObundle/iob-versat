#pragma once

#include <cstdio>

#include "configurations.hpp"
#include "accelerator.hpp"
#include "verilogParsing.hpp"

//struct FUInstance;
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

static String DelayTypeToString(DelayType type){
  switch(type){
  case DelayType_BASE:{
    return STRING("DelayType_BASE");
  } break;
  case DelayType_SINK_DELAY:{
    return STRING("DelayType_SINK_DELAY");
  } break;
  case DelayType_SOURCE_DELAY: {
    return STRING("DelayType_SOURCE_DELAY");
  } break;
  case DelayType_COMPUTE_DELAY: {
    return STRING("DelayType_COMPUTE_DELAY");
  } break;
  }

  return {};
}

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

struct ConfigurationInfo{
  // Copy of MergeInfo
  String name;
  Array<String> baseName;

  // TODO: These should not be here, the wires will always be the same regardless of unit type 
  Array<Wire> configs;
  Array<Wire> states;

  Array<int> inputDelays;
  Array<int> outputLatencies;
  
  // Calculated offsets are an array that associate the given FUInstance inside the fixedDelayCircuit allocated member into the corresponding index of the configuration array. For now, static units are given a value near 0x40000000 (their configuration comes from the staticUnits hashmap). Units without any config are given a value of -1.
  CalculatedOffsets   configOffsets;
  CalculatedOffsets   stateOffsets;
  CalculatedOffsets   delayOffsets;
  Array<int>          calculatedDelays;
  Array<int>          order;
  Array<bool>         unitBelongs;
  
  Array<int>          mergeMultiplexerConfigs;
};

enum FUDeclarationType{
  FUDeclarationType_SINGLE,
  FUDeclarationType_COMPOSITE,
  FUDeclarationType_SPECIAL,
  FUDeclarationType_MERGED,
  FUDeclarationType_ITERATIVE
};

// TODO: A lot of duplicated data exists since the change to merge.

// TODO: There is a lot of crux between parsing and creating the FUDeclaration for composite accelerators 
//       the FUDeclaration should be composed of something that is in common to all of them.
struct FUDeclaration{
  String name;

  // These always exist, regardless of merge info 
  Array<Wire> configs;
  Array<Wire> states;

  ConfigurationInfo baseConfig;
  Array<ConfigurationInfo> configInfo; // Info for each merged view for all fixedDelayCircuit instances, even if they do not belong to the merged view (unitBelongs indicates such cases)

  // TODO:  The idea is to phase out baseConfig and configInfo and replace them with the InstanceInfo approach
  //        The InstanceInfo approach is then used inside the FUDeclaration

  // TODO2: Because merge permeates the entire logic, there is no point having a different member to store data.
  //        baseConfig and configInfo should have been always just configInfo, with a size of 1 if no merge and non 1 if merge.
  //        We carried the same problem for AccelInfo, where we have a baseInfo and the merged infos, but in reality we should have only one member.

  AccelInfo info;
  
  //Array<InstanceInfo> instanceInfo;
  
  Array<String> parameters; // For now, only the parameters extracted from verilog files
  
  Opt<int> memoryMapBits; // 0 is a valid memory map size, so optional indicates that no memory map exists
  int nIOs;

  // Either we have the FUDeclaration be a instantiation of a FUType
  // Or we have the memory keep the values in a expression format.
  Array<ExternalMemoryInterfaceExpression> externalExpressionMemory;
  Array<ExternalMemoryInterface> externalMemory;

  // Stores different accelerators depending on properties we want
  Accelerator* baseCircuit;
  Accelerator* fixedDelayCircuit;
  Accelerator* flattenedBaseCircuit;
  
  const char* operation;

  SubMap* flattenMapping;
  
  int lat; // TODO: For now this is only for iterative units. Would also useful to have a standardized way of computing this from the graph and then compute it when needed. 
  
  // TODO: this is probably not needed, only used for verilog generation (which could be calculated inside the code generation functions)
  //       the info is all contained inside the units themselves and inside the calculated offsets
  Hashmap<StaticId,StaticData>* staticUnits;

  Array<Pair<String,int>> definitionArrays;
  
  FUDeclarationType type;
  DelayType delayType;

  bool isOperation;
  bool implementsDone;
  bool signalLoop;

  // Simple access functions
  int NumberInputs(){return baseConfig.inputDelays.size;};
  int NumberOutputs(){return baseConfig.outputLatencies.size;};
  int NumberConfigs(){return baseConfig.configs.size;}
  int NumberStates(){return baseConfig.states.size;}
  int NumberDelays(){return baseConfig.delayOffsets.max;};
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

extern Pool<FUDeclaration> globalDeclarations;

FUDeclaration* GetTypeByName(String str);
FUDeclaration* GetTypeByNameOrFail(String name);
void InitializeSimpleDeclarations();
bool HasMultipleConfigs(FUDeclaration* decl);


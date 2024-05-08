#pragma once

#include <cstdio>

#include "configurations.hpp"
#include "verilogParsing.hpp"

struct FUInstance;

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
  default: NOT_IMPLEMENTED("Implement as needed");
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
  FUDeclaration* baseType;

  // TODO: These should not be here, the wires will always be the same regardless of unit type 
  Array<Wire> configs;
  Array<Wire> states;

  // Calculated offsets are an array that associate the given InstanceNode inside the fixedDelayCircuit allocated member into the corresponding index of the configuration array. For now, static units are given a value near 0x40000000 (their configuration comes from the staticUnits hashmap). Units without any config are given a value of -1.
  CalculatedOffsets configOffsets;
  CalculatedOffsets stateOffsets;
  CalculatedOffsets delayOffsets;
  Array<int>        calculatedDelays;
  Array<bool>       unitBelongs;
};

// Maps the sub InstanceNodes into the corresponding index of the configuration array. 
struct MergeInfo{
  String name;
  ConfigurationInfo config;
  Array<String> baseName;
  FUDeclaration* baseType;
};

enum FUDeclarationType{
  FUDeclarationType_SINGLE,
  FUDeclarationType_COMPOSITE,
  FUDeclarationType_SPECIAL,
  FUDeclarationType_MERGED,
  FUDeclarationType_ITERATIVE
};

// TODO: There is a lot of crux between parsing and creating the FUDeclaration for composite accelerators 
//       the FUDeclaration should be composed of something that is in common to all of them.
// A declaration is the instantiation of a type
struct FUDeclaration{
  String name;

  Array<int> inputDelays;
  Array<int> outputLatencies;

  Array<int> calculatedDelays;

  // TODO: There should exist a "default" configInfo, instead of doing what we are currently which is using the 0 as the default;
  Array<ConfigurationInfo> configInfo;
  
  Opt<int> memoryMapBits; // 0 is a valid memory map size, so optional indicates that no memory map exists
  int nIOs;
  
  Array<ExternalMemoryInterface> externalMemory;

  // Stores different accelerators depending on properties we want
  Accelerator* baseCircuit;
  Accelerator* fixedDelayCircuit;

  // Merged accelerator
  //Array<MergeInfo> mergeInfo;
  
  const char* operation;

  int lat; // TODO: For now this is only for iterative units. Would also useful to have a standardized way of computing this from the graph and then compute it when needed. 
  
  // TODO: this is probably not needed, only used for verilog generation (which could be calculated inside the code generation functions)
  //       the info is all contained inside the units themselves and inside the calculated offsets
  Hashmap<StaticId,StaticData>* staticUnits;

  FUDeclarationType type;
  DelayType delayType;

  bool isOperation;
  bool implementsDone;
  bool signalLoop;

// Simple access functions
  int NumberInputs(){return inputDelays.size;};
  int NumberOutputs(){return outputLatencies.size;};
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
  extern FUDeclaration* data;
}

FUDeclaration* GetTypeByName(Versat* versat,String str);

void InitializeSimpleDeclarations(Versat* versat);

FUDeclaration* RegisterFU(Versat* versat,FUDeclaration declaration);
FUDeclaration* RegisterIterativeUnit(Versat* versat,Accelerator* accel,FUInstance* inst,int latency,String name);
FUDeclaration* RegisterSubUnit(Versat* versat,String name,Accelerator* accel);

int NumberOfSubunits(FUDeclaration* decl);

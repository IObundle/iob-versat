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

  AccelInfo info;

  int ConfigInfoSize(){
    return info.infos.size;
  };
  
  //Array<InstanceInfo> instanceInfo;

  int numberDelays;
  
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
  //int NumberInputs(){return baseConfig.inputDelays.size;};
  int NumberInputs(){return info.infos[0].inputDelays.size;};
  //int NumberOutputs(){return baseConfig.outputLatencies.size;};
  int NumberOutputs(){return info.infos[0].outputLatencies.size;};
  int NumberConfigs(){return configs.size;}
  int NumberStates(){return states.size;}
  int NumberDelays(){return numberDelays;}; //baseConfig.delayOffsets.max;

  // 
  Array<int> GetOutputLatencies(){
    return info.infos[0].outputLatencies;
  }

  Array<int> GetInputDelays(){
    return info.infos[0].inputDelays;
  }

  int MaxConfigs(){
    int max = 0;
    for(AccelInfoIterator iter = StartIteration(&this->info); iter.IsValid(); iter = iter.Next()){
      InstanceInfo* unit = iter.CurrentUnit();
      if(unit->configPos.has_value()){
        max = std::max(max,unit->configPos.value());
      }
    }
    return max + 1;
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

extern Pool<FUDeclaration> globalDeclarations;

FUDeclaration* GetTypeByName(String str);
FUDeclaration* GetTypeByNameOrFail(String name);
void InitializeSimpleDeclarations();
bool HasMultipleConfigs(FUDeclaration* decl);


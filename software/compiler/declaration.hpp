#pragma once

#include <cstdio>

#include "configurations.hpp"
#include "verilogParsing.hpp"

struct FUInstance;

static const int VCD_MAPPING_SIZE = 5;
class VCDMapping{
public:
  char currentMapping[VCD_MAPPING_SIZE+1];
  int increments;

public:

  VCDMapping(){Reset();};

  void Increment(){
    for(int i = VCD_MAPPING_SIZE - 1; i >= 0; i--){
      increments += 1;
      currentMapping[i] += 1;
      if(currentMapping[i] == 'z' + 1){
        currentMapping[i] = 'a';
      } else {
        return;
      }
    }
    Assert(false && "Increase mapping space");
  }

  void Reset(){
    currentMapping[VCD_MAPPING_SIZE] = '\0';
    for(int i = 0; i < VCD_MAPPING_SIZE; i++){
      currentMapping[i] = 'a';
    }
    increments = 0;
  }

  char* Get(){
    return currentMapping;
  }
};

typedef int* (*FUFunction)(FUInstance* inst);
typedef int* (*FUUpdateFunction)(FUInstance* inst,Array<int> inputs);
typedef int (*MemoryAccessFunction)(FUInstance* inst, int address, int value,int write);
typedef void (*VCDFunction)(FUInstance*,FILE*,VCDMapping&,Array<int>,bool firstTime,bool printDefinitions);
typedef void (*SignalFunction)(FUInstance* inst);

enum DelayType {
  DELAY_TYPE_BASE               = 0x0,
  DELAY_TYPE_SINK_DELAY         = 0x1,
  DELAY_TYPE_SOURCE_DELAY       = 0x2,
  DELAY_TYPE_COMPUTE_DELAY      = 0x4
};
#define CHECK_DELAY(inst,T) ((inst->declaration->delayType & T) == T)

inline DelayType operator|(DelayType a, DelayType b)
{return static_cast<DelayType>(static_cast<int>(a) | static_cast<int>(b));}

/*
// TODO: Finish making a FUType. FUDeclaration should be the instance of a type and should contain a pointer to the type that contains all the non parameterizable values.
  struct FUType{
  String name;

  Array<int> inputDelays;
  Array<int> outputLatencies;

  Array<WireExpression> configs;
  Array<WireExpression> states;

  Array<ExternalMemoryInterfaceExpression> externalMemory;
  }
*/

// I initially extracted this because of Merge, right?
struct ConfigurationInfo{
  Array<Wire> configs;
  Array<Wire> states;

  // Calculated offsets are an array that associate the given InstanceNode inside the fixedDelayCircuit allocated member into the corresponding index of the configuration array. For now, static units are given a value near 0x40000000 (their configuration comes from the staticUnits hashmap). Units without any config are given a value of -1.
  CalculatedOffsets configOffsets;
  CalculatedOffsets stateOffsets;
  CalculatedOffsets delayOffsets;
  CalculatedOffsets extraDataOffsets; // TODO: Do not need this for merged graphs
};

// Maps the sub InstanceNodes into the corresponding index of the configuration array. 
struct MergeInfo{
  String name;
  ConfigurationInfo config;
  Array<String> baseName;
  FUDeclaration* baseType;
};

// TODO: There is a lot of crux between parsing and creating the FUDeclaration for composite accelerators 
//       the FUDeclaration should be composed of something that is in common to all of them.
// A declaration is the instantiation of a type
struct FUDeclaration{
  String name;

  Array<int> inputDelays;
  Array<int> outputLatencies;

  Array<int> calculatedDelays;
  
  ConfigurationInfo configInfo;
  
  Optional<int> memoryMapBits; // 0 is a valid memory map size, so optional indicates that no memory map exists
  int nIOs;
  int nStaticConfigs;

  Array<ExternalMemoryInterface> externalMemory;

  // Stores different accelerators depending on properties we want
  Accelerator* baseCircuit;
  Accelerator* fixedDelayCircuit;

  // Merged accelerator
  Array<MergeInfo> mergeInfo;
   
  const char* operation;

  int lat; // TODO: For now this is only for iterative units. Would also useful to have a standardized way of computing this from the graph and then compute it when needed. 
  
  // TODO: this is probably not needed, only used for verilog generation (which could be calculated inside the code generation functions)
  //       the info is all contained inside the units themselves and inside the calculated offsets
  Hashmap<StaticId,StaticData>* staticUnits;

  enum {SINGLE = 0x0,COMPOSITE = 0x1,SPECIAL = 0x2,MERGED = 0x3,ITERATIVE = 0x4} type;
  DelayType delayType;

  bool isOperation;
  bool implementsDone;
  bool signalLoop;

// Simple functions access functions
  
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

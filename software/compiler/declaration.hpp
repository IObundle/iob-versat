#ifndef INCLUDED_VERSAT_DECLARATION
#define INCLUDED_VERSAT_DECLARATION

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

// TODO: Finish making a FUType. FUDeclaration should be the instance of a type and should contain a pointer to the type that contains all the non parameterizable values.
/*
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
  Array<Wire> configs;
  Array<Wire> states;

  CalculatedOffsets configOffsets;
  CalculatedOffsets stateOffsets;
  CalculatedOffsets delayOffsets;
  CalculatedOffsets outputOffsets;    // TODO: Do not need this for merged graphs
  CalculatedOffsets extraDataOffsets; // TODO: Do not need this for merged graphs

  Array<iptr> defaultConfig;
  Array<iptr> defaultStatic;
};

struct MergeInfo{
  ConfigurationInfo config;
  FUDeclaration* baseType;
};

// TODO: There is a lot of crux between parsing and creating the FUDeclaration for composite accelerators 
//       the FUDeclaration should be composed of something that is in common to all of them.
// A declaration is the instantiation of a type
struct FUDeclaration{
  String name;

  Array<int> inputDelays;
  Array<int> outputLatencies;

  ConfigurationInfo configInfo;
  
  int nIOs;
  int memoryMapBits;
  int nStaticConfigs;

  Array<ExternalMemoryInterface> externalMemory;

  // Stores different accelerators depending on properties we want
  Accelerator* baseCircuit;
  Accelerator* fixedDelayCircuit;

  // Merged accelerator
  Array<MergeInfo> mergeInfo;

  FUFunction initializeFunction;
  FUFunction startFunction;
  FUFunction endFunction;
  FUUpdateFunction updateFunction;
  FUFunction destroyFunction;
  VCDFunction printVCD;
  MemoryAccessFunction memAccessFunction;
  SignalFunction signalFunction;
   
  const char* operation;

  // TODO: this is probably not needed, only used for verilog generation (which could be calculated inside the code generation functions)
  //       the info is all contained inside the units themselves.
  Hashmap<StaticId,StaticData>* staticUnits;

  enum {SINGLE = 0x0,COMPOSITE = 0x1,SPECIAL = 0x2,MERGED = 0x3,ITERATIVE = 0x4} type;
  DelayType delayType;

  bool isOperation;
  bool implementsDone;
  bool isMemoryMapped;
  bool signalLoop;
};

bool IsComposite(FUDeclaration* decl);

bool IsCombinatorial(FUDeclaration* decl);
bool ContainsConfigs(FUDeclaration* decl);
bool ContainsStatics(FUDeclaration* decl);

int NumberOfSubunits(FUDeclaration* decl);

// TODO: This functions could be abstracted, to receive a function pointer that receives a declaration and checks wether the unit is a config type or a mem type or etc.
//       Check the functions that sort types by dependency to also use the same function pointer technique (in codeGeneration.cpp)
Array<FUDeclaration*> ConfigSubTypes(FUDeclaration* decl,Arena* out,Arena* sub);
Array<FUDeclaration*> MemSubTypes(FUDeclaration* decl,Arena* out,Arena* sub);

#endif // INCLUDED_VERSAT_DECLARATION

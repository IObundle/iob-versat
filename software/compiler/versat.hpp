#pragma once

#include <vector>
#include <unordered_map>

#include "../common/utils.hpp"
#include "../common/memory.hpp"
#include "../common/logger.hpp"

#include "accelerator.hpp"
#include "configurations.hpp"
// Forward declarations
struct Versat;
struct Accelerator;
struct FUDeclaration;
struct IterativeUnitDeclaration;
struct InstanceNode;
struct FUInstance;
struct Versat;

enum VersatDebugFlags{
  OUTPUT_GRAPH_DOT,
  GRAPH_DOT_FORMAT,
  OUTPUT_ACCELERATORS_CODE,
  OUTPUT_VERSAT_CODE,
  USE_FIXED_BUFFERS
};

typedef uint GraphDotFormat;
enum GraphDotFormat_{
  GRAPH_DOT_FORMAT_NAME = 0x1,
  GRAPH_DOT_FORMAT_TYPE = 0x2,
  GRAPH_DOT_FORMAT_ID = 0x4,
  GRAPH_DOT_FORMAT_DELAY = 0x8,
  GRAPH_DOT_FORMAT_EXPLICIT = 0x10, // indicates which field is which when outputting name
  GRAPH_DOT_FORMAT_PORT = 0x20, // Outputs port information for edges and port instance
  GRAPH_DOT_FORMAT_LATENCY = 0x40 // Outputs latency information for edges and port instances which know their latency information
};

struct DAGOrderNodes{
  Array<InstanceNode*> sinks;
  Array<InstanceNode*> sources;
  Array<InstanceNode*> computeUnits;
  Array<InstanceNode*> instances;
  Array<int>           order;
  int size;
  int maxOrder;
};

// TODO: Remove memory from old versat if tests pass.

struct FUInstance{
  String name;

  // This should be versat side only, but it's easier to have them here for now
  String parameters; // TODO: Actual parameter structure
  Accelerator* accel;
  FUDeclaration* declaration;
  int id;

  // PC only
  int baseDelay; // TODO: Delays should be pulled out from FUInstances and simply calculated and stored inside corresponding FUDeclarations if needed to be stored or just calculated at code generation time.

  union{
    int literal;
    int bufferAmount;
    int portIndex;
  };
  int sharedIndex;
  bool isStatic;
  bool sharedEnable;
};

struct DebugState{
  uint dotFormat;
  bool outputGraphs;
  bool outputAccelerator;
  bool outputVersat;
  bool outputVCD;
  bool outputAcceleratorInfo;
  bool useFixedBuffers;
};

struct Options{
  Array<String> verilogFiles;
  Array<String> extraSources;
  Array<String> includePaths;
  Array<String> unitPaths;
  
  String hardwareOutputFilepath;
  String softwareOutputFilepath;
  String verilatorRoot;

  const char* specificationFilepath;
  const char* topName;
  int addrSize; // AXI_ADDR_W - used to be bitSize
  int dataSize; // AXI_DATA_W

  bool addInputAndOutputsToTop;
  bool debug;
  bool shadowRegister;
  bool architectureHasDatabus;
  bool useFixedBuffers;
  bool generateFSTFormat;
  bool noDelayPropagation;
  bool useDMA;
  bool exportInternalMemories;
};

struct Versat{
  Pool<FUDeclaration> declarations;
  Pool<Accelerator> accelerators;

  Arena permanent;
  Arena temp;

  DebugState debug;

  Options* opts;
};

struct UnitValues{
  int inputs;
  int outputs;

  int configs;
  int states;
  int delays;
  int ios;
  int extraData;
  int statics;
  int sharedUnits;
  int externalMemoryInterfaces;
  int externalMemoryByteSize;
  int numberUnits;

  int memoryMappedBits;
  bool isMemoryMapped;
  bool signalLoop;
};

struct TypeAndInstance{
  String typeName;
  String instanceName;
};

struct CompiledTemplate;
namespace BasicTemplates{
  extern CompiledTemplate* acceleratorTemplate;
  extern CompiledTemplate* iterativeTemplate;
  extern CompiledTemplate* topAcceleratorTemplate;
  extern CompiledTemplate* acceleratorHeaderTemplate;
  extern CompiledTemplate* externalInternalPortmapTemplate;
  extern CompiledTemplate* externalPortTemplate;
  extern CompiledTemplate* externalInstTemplate;
  extern CompiledTemplate* internalWiresTemplate;
}

struct GraphMapping;

struct CalculateDelayResult{
  Hashmap<EdgeNode,int>* edgesDelay;
  Hashmap<InstanceNode*,int>* nodeDelay;
};

// Temp
bool EqualPortMapping(PortInstance p1,PortInstance p2);

// General info
UnitValues CalculateAcceleratorValues(Versat* versat,Accelerator* accel);
bool IsUnitCombinatorial(FUInstance* inst);

// Delay
CalculateDelayResult CalculateDelay(Versat* versat,Accelerator* accel,Arena* out);

// Graph fixes
void FixDelays(Versat* versat,Accelerator* accel,Hashmap<EdgeNode,int>* edgeDelays);

// Accelerator merging
DAGOrderNodes CalculateDAGOrder(InstanceNode* instances,Arena* arena);

// Debug
void AssertGraphValid(InstanceNode* nodes,Arena* arena);

// Misc
bool CheckValidName(String name); // Check if name can be used as identifier in verilog
bool IsTypeHierarchical(FUDeclaration* decl);
FUInstance* GetInputInstance(InstanceNode* nodes,int inputIndex);
FUInstance* GetOutputInstance(InstanceNode* nodes);

// Temp
struct ModuleInfo;
FUDeclaration* RegisterModuleInfo(Versat* versat,ModuleInfo* info);

// Versat functions
Versat* InitVersat();
void Free(Versat* versat); // Usually not needed, as memory is freed on program termination and Versat is supposed to be "active" from start to finish, but usuful for debugging memory problems
void ParseVersatSpecification(Versat* versat,String content);
void ParseVersatSpecification(Versat* versat,const char* filepath);

// Accelerator functions
Accelerator* CreateAccelerator(Versat* versat,String name);
FUInstance* CreateFUInstance(Accelerator* accel,FUDeclaration* type,String entityName);
Accelerator* Flatten(Versat* versat,Accelerator* accel,int times); // This should work on the passed accelerator. We should not create a new accelerator

// Static and configuration sharing
void ShareInstanceConfig(FUInstance* inst, int shareBlockIndex);
void SetStatic(Accelerator* accel,FUInstance* inst);

// Declaration functions
FUDeclaration* RegisterFU(Versat* versat,FUDeclaration declaration);
FUDeclaration* GetTypeByName(Versat* versat,String str);
FUDeclaration* RegisterIterativeUnit(Versat* versat,Accelerator* accel,FUInstance* inst,int latency,String name);
FUDeclaration* RegisterSubUnit(Versat* versat,Accelerator* accel);

void PrintDeclaration(FILE* out,FUDeclaration* decl,Arena* temp,Arena* temp2);

// Helper functions, useful to implement custom units
FUInstance* CreateOrGetInput(Accelerator* accel,String name,int portNumber);
FUInstance* CreateOrGetOutput(Accelerator* accel);

// Helper functions to create sub accelerators
int GetInputPortNumber(FUInstance* inputInstance);

// General hook function for debugging purposes
int CalculateMemoryUsage(Versat* versat); // Not accurate, but returns the biggest amount of memory usage.

void TestVersatSide(Versat* versat); // Calls tests from versat side

#include "typeSpecifics.inl"

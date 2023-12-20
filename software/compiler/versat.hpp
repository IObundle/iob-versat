#ifndef INCLUDED_VERSAT_HPP
#define INCLUDED_VERSAT_HPP

#include <vector>
#include <unordered_map>

#include "../common/utils.hpp"
#include "../common/memory.hpp"
#include "../common/logger.hpp"

#include "accelerator.hpp"
#include "configurations.hpp"
#include "graph.hpp"

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
  OUTPUT_VCD,
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

struct DAGOrder{
  FUInstance** sinks;
  int numberSinks;
  FUInstance** sources;
  int numberSources;
  FUInstance** computeUnits;
  int numberComputeUnits;
  FUInstance** instances;
};

struct VersatComputedData{
  int memoryMaskSize;
  int memoryAddressOffset;
  char memoryMaskBuffer[33];
  char* memoryMask;
};

struct ComputedData{
  Array<ExternalMemoryInterface> external;
  Array<VersatComputedData> data;
};

struct Parameter{
  String name;
  String value;
  Parameter* next;
};

struct DAGOrderNodes{
  InstanceNode** sinks;
  int numberSinks;
  InstanceNode** sources;
  int numberSources;
  InstanceNode** computeUnits;
  int numberComputeUnits;
  InstanceNode** instances;
  int* order;
  int size;
  int maxOrder;
};

struct FUInstance{
  String name;

  // Embedded memory
  int* memMapped;
  iptr* config;
  int* state;
  iptr* delay;
  Byte* externalMemory;

  // This should be part of another hierarchy.
  // Required for accel subunits but not required by embedded or should user code have access to them
  int* outputs;
  int* storedOutputs;
  unsigned char* extraData;
  FUInstance* declarationInstance; // Used because of AccessMemory and the fact that GetInput depends on instance pointer

  // This should be versat side only, but it's easier to have them here for now
  // User code should not access them directly. Future versions will not have them
  String parameters;
  Accelerator* accel;
  FUDeclaration* declaration;
  int id;

  // PC only
  int baseDelay;

  // Configuration + State variables that versat needs access to
  int done; // Units that are sink or sources of data must implement done to ensure circuit does not end prematurely
  bool isStatic;

  bool namedAccess;

  // Various uses
  Accelerator* iterative;

  UnitDebugData* debugData;

  union{
    int literal;
    int bufferAmount;
    int portIndex;
  };
  //GraphComputedData* graphData;
  //VersatComputedData* versatData;
  int sharedIndex;
  char tag;
  bool savedConfiguration; // For subunits registered, indicate when we save configuration before hand
  bool savedMemory; // Same for memory
  bool sharedEnable;
  bool initialized;
};

struct DebugState{
  uint dotFormat;
  bool outputGraphs;
  bool outputAccelerator;
  bool outputVersat;
  bool outputVCD;
  bool useFixedBuffers;
};

struct Options{
  // Repr : test
  int addrSize;
  int dataSize;
  bool shadowRegister;
  bool architectureHasDatabus;
  bool generateFSTFormat;
  bool noDelayPropagation;
};

struct Versat{
  Pool<FUDeclaration> declarations;
  Pool<Accelerator> accelerators;

  Arena permanent;
  Arena temp;

  int base;
  int numberConfigurations;

  DebugState debug;

  String outputLocation;
  Options opts;
};

struct UnitValues{
  int inputs;
  int outputs;

  int configs;
  int states;
  int delays;
  int ios;
  int totalOutputs;
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

struct VersatComputedValues{
  int nConfigs;
  int configBits;

  int versatConfigs;
  int versatStates;

  int nStatics;
  int staticBits;
  int staticBitsStart;

  int nDelays;
  int delayBits;
  int delayBitsStart;

  // Configurations = config + static + delays
  int nConfigurations;
  int configurationBits;
  int configurationAddressBits;

  int nStates;
  int stateBits;
  int stateAddressBits;

  int unitsMapped;
  int memoryMappedBytes;
  //   int maxMemoryMapDWords;

  int nUnitsIO;

  int numberConnections;

  int externalMemoryInterfaces;

  int stateConfigurationAddressBits;
  int memoryAddressBits;
  int memoryMappingAddressBits;
  int memoryConfigDecisionBit;
  //int lowerAddressSize;

  bool signalLoop;
};

struct HierarchicalName{
  String name;
  HierarchicalName* next;
};

struct TypeAndInstance{
  String typeName;
  String instanceName;
};

struct SharingInfo{
  int* ptr;
  bool init;
};

// Simple operations should also be stored here. They are versat agnostic as well
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

struct CompiledTemplate;
namespace BasicTemplates{
  extern CompiledTemplate* acceleratorTemplate;
  extern CompiledTemplate* topAcceleratorTemplate;
  extern CompiledTemplate* wrapperTemplate;
  extern CompiledTemplate* acceleratorHeaderTemplate;
  extern CompiledTemplate* externalPortmapTemplate;
  extern CompiledTemplate* externalInternalPortmapTemplate;
  extern CompiledTemplate* externalPortTemplate;
  extern CompiledTemplate* externalInstTemplate;
  extern CompiledTemplate* iterativeTemplate;
}

struct GraphMapping;

struct TypeStructInfoElement{
  String type;
  String name;
};

struct TypeStructInfo{
  String name;
  Array<TypeStructInfoElement> entries;
};

// Temp
bool EqualPortMapping(PortInstance p1,PortInstance p2);

// General info
UnitValues CalculateIndividualUnitValues(FUInstance* inst); // Values for individual unit, not taking into account sub units. For a composite, this pretty much returns empty except for total outputs, as the unit itself must allocate output memory
UnitValues CalculateAcceleratorUnitValues(Versat* versat,FUInstance* inst); // Values taking into account sub units
UnitValues CalculateAcceleratorValues(Versat* versat,Accelerator* accel);
VersatComputedValues ComputeVersatValues(Versat* versat,Accelerator* accel);
int CalculateTotalOutputs(Accelerator* accel);
int CalculateTotalOutputs(FUInstance* inst);
bool IsConfigStatic(Accelerator* topLevel,FUInstance* inst);
bool IsUnitCombinatorial(FUInstance* inst);

int CalculateNumberOfUnits(InstanceNode* node);

// Delay
int CalculateLatency(FUInstance* inst);
Hashmap<EdgeNode,int>* CalculateDelay(Versat* versat,Accelerator* accel,Arena* out);
void SetDelayRecursive(Accelerator* accel,Arena* arena);

// Graph fixes
void FixMultipleInputs(Versat* versat,Accelerator* accel);
void FixMultipleInputs(Versat* versat,Accelerator* accel,Hashmap<FUInstance*,int>* instanceToInput);
void FixDelays(Versat* versat,Accelerator* accel,Hashmap<EdgeNode,int>* edgeDelays);

// Accelerator merging
DAGOrderNodes CalculateDAGOrder(InstanceNode* instances,Arena* arena);

// Debug
void AssertGraphValid(InstanceNode* nodes,Arena* arena);
void OutputGraphDotFile(Versat* versat,Accelerator* accel,bool collapseSameEdges,const char* filenameFormat,...) __attribute__ ((format (printf, 4, 5)));
void OutputGraphDotFile(Versat* versat,Accelerator* accel,bool collapseSameEdges,FUInstance* highlighInstance,const char* filenameFormat,...) __attribute__ ((format (printf, 5, 6)));
void OutputGraphDotFile(Versat* versat,Accelerator* accel,bool collapseSameEdges,Set<FUInstance*>* highlight,const char* filenameFormat,...) __attribute__ ((format (printf, 5, 6)));
Array<String> GetFullNames(Accelerator* accel,Arena* out);

void TemplateSetDefaults(Versat*);

// Misc
bool CheckValidName(String name); // Check if name can be used as identifier in verilog
bool IsTypeHierarchical(FUDeclaration* decl);
bool IsTypeSimulatable(FUDeclaration* decl);
int CountNonSpecialChilds(InstanceNode* nodes);
void OutputCircuitSource(Versat* versat,FUDeclaration* decl,Accelerator* accel,FILE* file);
InstanceNode* GetInputNode(InstanceNode* nodes,int inputIndex);
int GetInputValue(InstanceNode* nodes,int index);
FUInstance* GetInputInstance(InstanceNode* nodes,int inputIndex);
InstanceNode* GetOutputNode(InstanceNode* nodes);
FUInstance* GetOutputInstance(InstanceNode* nodes);
PortNode GetInputValueInstance(FUInstance* inst,int index);

ComputedData CalculateVersatComputedData(InstanceNode* instances,VersatComputedValues val,Arena* out);

// Temp
struct ModuleInfo;
FUDeclaration* RegisterModuleInfo(Versat* versat,ModuleInfo* info);

Accelerator* RecursiveFlatten(Versat* versat,Accelerator* topLevel);

// Versat functions
Versat* InitVersat(int base,int numberConfigurations,bool initUnits = false);
void Free(Versat* versat); // Usually not needed, as memory is freed on program termination and Versat is supposed to be "active" from start to finish, but usuful for debugging memory problems
void ParseCommandLineOptions(Versat* versat,int argc,const char** argv);
uint SetDebug(Versat* versat,VersatDebugFlags flags,uint flag);
void ParseVersatSpecification(Versat* versat,String content);
void ParseVersatSpecification(Versat* versat,const char* filepath);

// Accelerator functions
Accelerator* CreateAccelerator(Versat* versat);
FUInstance* CreateFUInstance(Accelerator* accel,FUDeclaration* type,String entityName);
void AcceleratorRun(Accelerator* accel,int times = 1);
void AcceleratorRunDebug(Accelerator* accel);
void RemoveFUInstance(Accelerator* accel,FUInstance* inst);
void OutputVersatSource(Versat* versat,Accelerator* accel,const char* hardwarePath,const char* softwarePath,String accelName = STRING("TOP"),bool isSimple = false); // Not actually the name but the type.
Accelerator* Flatten(Versat* versat,Accelerator* accel,int times); // This should work on the passed accelerator. We should not create a new accelerator

// Access units and sub units inside an accelerator. Can use printf style arguments, but only chars and integers are currently supported. Format is <format1>,<args1 if any>,<format2>,...
#define GetInstanceByName(ACCEL,...) GetInstanceByName_(ACCEL,NUMBER_ARGS(__VA_ARGS__),__VA_ARGS__)
FUInstance* GetInstanceByName_(Accelerator* accel,int argc, ...);

#define GetSubInstanceByName(TOP_LEVEL,INST,...) GetSubInstanceByName_(TOP_LEVEL,INST,NUMBER_ARGS(__VA_ARGS__),__VA_ARGS__)
FUInstance* GetSubInstanceByName_(Accelerator* topLevel,FUInstance* inst,int argc, ...);

// Unit connection
void ConnectUnits(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay = 0);
void ConnectUnitsIfNotConnected(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay = 0);

// Read/Write to unit
void VersatUnitWrite(FUInstance* instance,int address, int value);
int VersatUnitRead(FUInstance* instance,int address);
void VersatMemoryCopy(FUInstance* instance,int* dest,int* data,int size);
void VersatMemoryCopy(FUInstance* instance,int* dest,Array<int> data);

// Unit default configuration and configuration sharing
void SetDefaultConfiguration(FUInstance* inst);
void ShareInstanceConfig(FUInstance* inst, int shareBlockIndex);
void SetStatic(Accelerator* accel,FUInstance* inst);

// Declaration functions
FUDeclaration* RegisterFU(Versat* versat,FUDeclaration declaration);
FUDeclaration* GetTypeByName(Versat* versat,String str);
FUDeclaration* RegisterIterativeUnit(Versat* versat,Accelerator* accel,FUInstance* inst,int latency,String name);
FUDeclaration* RegisterSubUnit(Versat* versat,String name,Accelerator* accel);

// Configuration loading and storing
void ClearConfigurations(Accelerator* accel);
void SaveConfiguration(Accelerator* accel,int configuration);
void LoadConfiguration(Accelerator* accel,int configuration);
void ActivateMergedAccelerator(Versat* versat,Accelerator* accel,FUDeclaration* type); // Set the accelerator configuration to execute the merged graph represented by type

// Debug + output general accelerator info
void OutputMemoryMap(Versat* versat,Accelerator* accel);
void OutputUnitInfo(FUInstance* instance);
void DisplayAcceleratorMemory(Accelerator* topLevel);
void DisplayUnitConfiguration(Accelerator* topLevel);

void DebugAccelerator(Accelerator* accel);
void DebugVersat(Versat* versat);

// Debug units, only works for pc-emul (no declaration info in embedded)
bool CheckInputAndOutputNumber(FUDeclaration* type,int inputs,int outputs);
void PrintAcceleratorInstances(Accelerator* accel);

// Helper functions, useful to implement custom units
int GetInputValue(FUInstance* instance,int port);
int GetNumberOfInputs(FUInstance* inst);
int GetNumberOfOutputs(FUInstance* inst);
FUInstance* CreateOrGetInput(Accelerator* accel,String name,int portNumber);
FUInstance* CreateOrGetOutput(Accelerator* accel);

// Helper functions to create sub accelerators
int GetNumberOfInputs(Accelerator* accel);
int GetNumberOfOutputs(Accelerator* accel);
void SetInputValue(Accelerator* accel,int portNumber,int number);
int GetOutputValue(Accelerator* accel,int portNumber);
int GetInputPortNumber(FUInstance* inputInstance);

// General hook function for debugging purposes
int CalculateMemoryUsage(Versat* versat); // Not accurate, but returns the biggest amount of memory usage.

void TestVersatSide(Versat* versat); // Calls tests from versat side

#include "typeSpecifics.inl"

#endif // INCLUDED_VERSAT_HPP

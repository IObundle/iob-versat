#ifndef INCLUDED_VERSAT_H
#define INCLUDED_VERSAT_H

#include "stdio.h"
#include "stdint.h"
#include "stdbool.h"

#include "utils.h"

// Ugly but useful for now
#define MAKE_VERSAT(NAME) \
   char tempBuffer_##NAME[1024]; \
   memset(tempBuffer_##NAME,0,1024); \
   Versat* NAME = (Versat*) tempBuffer_##NAME

#ifdef __cplusplus
#define EXPORT extern "C"
#else
#define EXPORT
#endif

typedef struct Versat Versat;
typedef struct FUInstance FUInstance;
typedef int32_t* (*FUFunction)(FUInstance*);
typedef int32_t (*MemoryAccessFunction)(FUInstance* instance, int address, int value,int write);

typedef struct PortInstance{
   FUInstance* inst;
   int port;
} PortInstance;

enum DelayType {
   DELAY_TYPE_SINK               = 0x1, // Sink of data in the circuit.
   DELAY_TYPE_SOURCE             = 0x2, // If the unit is a source of data
   DELAY_TYPE_IMPLEMENTS_DELAY   = 0x4, // Wether unit implements delay or not
   DELAY_TYPE_SOURCE_DELAY       = 0x8, // Wether the unit has delay to produce data, or to process data (1 - delay on source,0 - delay on compute/sink)
   DELAY_TYPE_IMPLEMENTS_DONE    = 0x10 // Done is used to indicate to outside world that circuit is still processing. Only when every unit that implements done sets it to 1 should the circuit terminate
};

typedef struct Wire{
   const char* name;
   int bitsize;
} Wire;

typedef struct Accelerator Accelerator;

typedef struct FUDeclaration{
   HierarchyName name;

   int nInputs;
   int nOutputs;

	union {
      // Composite declaration
      Accelerator* circuit;

      // Simple FU declaration
      struct{
         // Config and state interface
         int nConfigs;
         const Wire* configWires;

         int nStates;
         const Wire* stateWires;

         int latency; // Assume, for now, every port has the same latency

         int memoryMapBytes;
         int extraDataSize;
         bool doesIO;
         FUFunction initializeFunction;
         FUFunction startFunction;
         FUFunction updateFunction;
         MemoryAccessFunction memAccessFunction;
      };
   };

   enum {SINGLE,COMPOSITE,SPECIAL} type;
   enum DelayType delayType;
} FUDeclaration;

typedef struct GraphComputedData GraphComputedData;

typedef struct FUInstance{
   Accelerator* accel;
	HierarchyName name;
	FUDeclaration* declaration;
	int id;
   Accelerator* compositeAccel;

	int32_t* outputs;
	int32_t* storedOutputs;
   void* extraData;

   // Embedded memory
   volatile int* memMapped;
   volatile int* config;
   volatile int* state;

   // Configuration + State variables that versat needs access to
   volatile int delay; // How many cycles unit must wait before seeing valid data, one delay for each output
   int done; // Units that are sink or sources of data must implement done to ensure circuit does not end prematurely

   // Various uses
   GraphComputedData* tempData;
   char tag;
} FUInstance;

#if 0
struct ConnectionInfo{
   PortInstance inst;
   int port;
   int delay;
};

struct GraphComputedData{
   int numberInputs;
   int numberOutputs;
   ConnectionInfo* inputs; // Delay not used
   ConnectionInfo* outputs;
   enum {TAG_UNCONNECTED,TAG_COMPUTE,TAG_SOURCE,TAG_SINK,TAG_SOURCE_AND_SINK} nodeType;
   int inputDelay;
};

struct Versat{
	FUDeclaration* declarations;
	int nDeclarations;
	Pool<Accelerator> accelerators;

	int numberConfigurations;

   // Options
   int byteAddressable;
   int useShadowRegisters;
};

// MERGING

struct Accelerator{
   Versat* versat;

   Pool<FUInstance> instances;
	Pool<Edge> edges;

   Pool<FUInstance*> inputInstancePointers;
   FUInstance* outputInstance;

	void* configuration;
	int configurationSize;

	bool init;
};

struct UnitInfo{
   int nConfigsWithDelay;
   int configBitSize;
   int nStates;
   int stateBitSize;
   int memoryMappedBytes;
   int implementsDelay;
   int numberDelays;
   int implementsDone;
   int doesIO;
};

#endif

typedef struct Edge{ // A edge in a graph
   PortInstance units[2];
} Edge;

typedef struct MappingNode{ // Mapping (edge to edge or node to node)
   Edge edges[2];
} MappingNode;

typedef struct MappingEdge{ // Edge between mapping from edge to edge
   MappingNode nodes[2];
} MappingEdge;

typedef struct ConsolidationGraph{
   MappingNode* nodes;
   int numberNodes;
   MappingEdge* edges;
   int numberEdges;

   // Used in get clique;
   int* validNodes;
} ConsolidationGraph;

typedef struct DAGOrder{
   FUInstance** sinks;
   int numberSinks;
   FUInstance** sources;
   int numberSources;
   FUInstance** computeUnits;
   int numberComputeUnits;
   FUInstance** instancePtrs; // Source, then compute, then sink (source_and_sink treated as sink)
} DAGOrder;

EXPORT Accelerator* Flatten(Versat* versat,Accelerator* accel,int times);

EXPORT void OutputGraphDotFile(Accelerator* accel,FILE* outputFile,int collapseSameEdges);

// Versat functions
EXPORT void InitVersat(Versat* versat,int base,int numberConfigurations);
EXPORT FUDeclaration* RegisterFU(Versat* versat,FUDeclaration declaration);
EXPORT void OutputVersatSource(Versat* versat,const char* definitionFilepath,const char* sourceFilepath,const char* configurationFilepath);
EXPORT void OutputMemoryMap(Versat* versat);

EXPORT FUDeclaration* GetTypeByName(Versat* versat,const char* name);

// Accelerator functions
EXPORT Accelerator* CreateAccelerator(Versat* versat);
EXPORT FUInstance* CreateFUInstance(Accelerator* accel,FUDeclaration* type);
EXPORT FUInstance* CreateNamedFUInstance(Accelerator* accel,FUDeclaration* type,const char* entityName,HierarchyName* hierarchyParent);
EXPORT void RemoveFUInstance(Accelerator* accel,FUInstance* inst);

#define GetInstanceByName(ACCEL,...) GetInstanceByName_(ACCEL,NUMBER_ARGS(__VA_ARGS__),__VA_ARGS__)
EXPORT FUInstance* GetInstanceByName_(Accelerator* accel,int argc, ...);

EXPORT void SaveConfiguration(Accelerator* accel,int configuration);
EXPORT void LoadConfiguration(Accelerator* accel,int configuration);

EXPORT void AcceleratorRun(Accelerator* accel);
EXPORT void IterativeAcceleratorRun(Accelerator* accel);

EXPORT void AddInput(Accelerator* accel);

EXPORT // Helper functions
EXPORT int32_t GetInputValue(FUInstance* instance,int port);
EXPORT int UnitDelays(FUInstance* inst);

EXPORT void ConnectUnits(FUInstance* out,int outIndex,FUInstance* in,int inIndex);

EXPORT void VersatUnitWrite(FUInstance* instance,int address, int value);
EXPORT int32_t VersatUnitRead(FUInstance* instance,int address);

EXPORT int CalculateLatency(FUInstance* inst,int sourceAndSinkAsSource);
EXPORT void CalculateDelay(Versat* versat,Accelerator* accel);
EXPORT void* CalculateGraphData(Accelerator* accel);
EXPORT DAGOrder CalculateDAGOrdering(Accelerator* accel);

EXPORT ConsolidationGraph GenerateConsolidationGraph(Accelerator* accel1,Accelerator* accel2);
EXPORT ConsolidationGraph MaxClique(ConsolidationGraph graph);
EXPORT Accelerator* MergeGraphs(Versat* versat,Accelerator* accel1,Accelerator* accel2,ConsolidationGraph graph);

#endif // INCLUDED_VERSAT_H

#ifndef INCLUDED_VERSAT_HPP
#define INCLUDED_VERSAT_HPP

#include <stdio.h>
#include <unordered_map>
#include <initializer_list>
#include <vector>
#include <set>

#include "utils.hpp"
#include "memory.hpp"

// Forward declarations
struct Versat;
struct Accelerator;
struct FUInstance;
struct FUDeclaration;

typedef int* (*FUFunction)(FUInstance*);
typedef int (*MemoryAccessFunction)(FUInstance* instance, int address, int value,int write);

enum DelayType {
   DELAY_TYPE_BASE               = 0x0,
   DELAY_TYPE_SINK_DELAY         = 0x1,
   DELAY_TYPE_SOURCE_DELAY       = 0x2,
   DELAY_TYPE_COMPUTE_DELAY      = 0x4,
   DELAY_TYPE_IMPLEMENTS_DONE    = 0x8
};

inline DelayType operator|(DelayType a, DelayType b)
{
    return static_cast<DelayType>(static_cast<int>(a) | static_cast<int>(b));
}

#define CHECK_DELAY(inst,T) ((inst->declaration->delayType & T) == T)

struct Wire{
   SizedString name;
   int bitsize;
};

// Maybe add wire name here?
// Need wire name
struct StaticInfo{
   FUDeclaration* module;
   SizedString name;
   Wire* wires;
   int nConfigs;
   int* ptr;
};

bool operator<(const StaticInfo& lhs,const StaticInfo& rhs);

struct FUDeclaration{
   HierarchyName name;

   int nInputs;
   int nOutputs;

   int* inputDelays;
   int* latencies;

   Accelerator* circuit; // Composite declaration

   // Config and state interface
   int nConfigs;
   Wire* configWires;

   int nStates;
   Wire* stateWires;

   int nStaticConfigs;

   int nDelays; // Code only handles 1 single instace, for now, hardware needs this value for correct generation
   int nIOs;
   int memoryMapBits;
   bool isMemoryMapped;
   int extraDataSize;
   FUFunction initializeFunction;
   FUFunction startFunction;
   FUFunction updateFunction;
   MemoryAccessFunction memAccessFunction;
   const char* operation;
   bool isOperation;

   std::vector<StaticInfo> staticUnits;

   enum {SINGLE = 0x0,COMPOSITE = 0x1,SPECIAL = 0x2} type;

   DelayType delayType;
};

struct PortInstance{
   FUInstance* inst;
   int port;
};

struct ConnectionInfo{
   PortInstance inst;
   int port;
   int delay;
};

struct GraphComputedData{
   int numberInputs;
   int numberOutputs;
   int inputPortsUsed;
   int outputPortsUsed;
   ConnectionInfo* inputs;
   ConnectionInfo* outputs;
   enum {TAG_UNCONNECTED,TAG_COMPUTE,TAG_SOURCE,TAG_SINK,TAG_SOURCE_AND_SINK} nodeType;
   int inputDelay;
};

struct VersatComputedData{
   int memoryMaskSize;
   char memoryMask[33];
   int memoryAddressOffset;
};

struct FUInstance{
	HierarchyName name;

   // Embedded memory
   int* memMapped;
   int* config;
   int* state;
   int* delay;

   int baseDelay;

   // PC only
   Accelerator* accel;
	FUDeclaration* declaration;
	int id;
   Accelerator* compositeAccel;

	int* outputs;
	int* storedOutputs;
   void* extraData;

   // Configuration + State variables that versat needs access to
   int done; // Units that are sink or sources of data must implement done to ensure circuit does not end prematurely
   bool isStatic;

   bool namedAccess;

   // Various uses
   GraphComputedData* tempData;
   VersatComputedData* versatData;
   char tag;
};

struct DebugState{
   bool outputAccelerator;
};

struct Versat{
	Pool<FUDeclaration> declarations;
	Pool<Accelerator> accelerators;

	Arena permanent;
	Arena temp;

	int base;
	int numberConfigurations;

	// Declaration for units that versat needs to instantiate itself
	FUDeclaration* delay;
   FUDeclaration* input;
   FUDeclaration* output;
   FUDeclaration* pipelineRegister;

   DebugState debug;

   // Command line options
   std::vector<const char*> includeDirs;
};

struct Edge{ // A edge in a graph
   PortInstance units[2];
   int delay;
};

struct DAGOrder{
   FUInstance** sinks;
   int numberSinks;
   FUInstance** sources;
   int numberSources;
   FUInstance** computeUnits;
   int numberComputeUnits;
   Allocation<FUInstance*> instances;
};

struct Accelerator{
   Versat* versat;

   Pool<FUInstance> instances;
	Pool<Edge> edges;

   Pool<FUInstance*> inputInstancePointers;
   FUInstance* outputInstance;

   Allocation<int> configAlloc;
   Allocation<int> stateAlloc;
   Allocation<int> delayAlloc;
   Allocation<int> staticAlloc;

   std::vector<StaticInfo> staticInfo;

	void* configuration;
	int configurationSize;

	enum Locked {FREE,GRAPH,ORDERED,FIXED} locked;
   DAGOrder order;
   Allocation<Byte> graphData;
	Allocation<VersatComputedData> versatData;
	int cyclesPerRun;

	int entityId;
	bool init;

	enum {ACCELERATOR,SUBACCELERATOR,CIRCUIT} type;
};

struct MappingNode{ // Mapping (edge to edge or node to node)
   Edge edges[2];
};

struct MappingEdge{ // Edge between mapping from edge to edge
   MappingNode nodes[2];
};

struct ConsolidationGraph{
   MappingNode* nodes;
   int numberNodes;
   MappingEdge* edges;
   int numberEdges;

   // Used in get clique;
   int* validNodes;
};

// Private but needed to be on header for type introspection
struct Mapping{
   FUInstance* source;
   FUInstance* sink;
};

struct VersatComputedValues{
   int nConfigs;
   int configBits;

   int nStatics;
   int staticBits;

   int nDelays;
   int delayBits;

   // The sum of config + static + delays (all types of configuration)
   int nConfigurations;
   int configurationBits;
   int configurationAddressBits;

   int nStates;
   int stateBits;
   int stateAddressBits;

   int unitsMapped;
   int memoryMapped;
   int maxMemoryMapDWords;

   int nUnitsIO;

   int numberConnections;

   int stateConfigurationAddressBits;
   int memoryAddressBits;
   int memoryMappingAddressBits;
   int memoryConfigDecisionBit;
   int lowerAddressSize;
};

struct SubgraphData{
   Accelerator* accel;
   FUInstance* instanceMapped;
};

struct HashKey{
   SizedString key;
   int data;
};

Accelerator* Flatten(Versat* versat,Accelerator* accel,int times);

void OutputGraphDotFile(Accelerator* accel,bool collapseSameEdges,const char* filenameFormat,...) __attribute__ ((format (printf, 3, 4)));

// Versat functions
void InitVersat(Versat* versat,int base,int numberConfigurations);
void ParseCommandLineOptions(Versat* versat,int argc,const char** argv);
void ParseVersatSpecification(Versat* versat,const char* filepath);

FUDeclaration* RegisterFU(Versat* versat,FUDeclaration declaration);
void OutputVersatSource(Versat* versat,Accelerator* accel,const char* sourceFilepath,const char* constantsFilepath,const char* dataFilepath);
void OutputCircuitSource(Versat* versat,FUDeclaration accelDecl,Accelerator* accel,FILE* file);
void OutputMemoryMap(Versat* versat,Accelerator* accel);
void OutputUnitInfo(FUInstance* instance);

FUDeclaration* GetTypeByName(Versat* versat,SizedString str);

// Accelerator functions
Accelerator* CreateAccelerator(Versat* versat);
FUInstance* CreateFUInstance(Accelerator* accel,FUDeclaration* type);
FUInstance* CreateNamedFUInstance(Accelerator* accel,FUDeclaration* type,SizedString entityName);
void RemoveFUInstance(Accelerator* accel,FUInstance* inst);

void LockAccelerator(Accelerator* accel,Accelerator::Locked);

// Can use printf style arguments, but only chars and integers.
// Put arguments right after format string
#define GetInstanceByName(ACCEL,...) GetInstanceByName_(ACCEL,NUMBER_ARGS(__VA_ARGS__),__VA_ARGS__)
FUInstance* GetInstanceByName_(Accelerator* accel,int argc, ...);

void SaveConfiguration(Accelerator* accel,int configuration);
void LoadConfiguration(Accelerator* accel,int configuration);

void AcceleratorRun(Accelerator* accel);

void AddInput(Accelerator* accel);

// Helper functions
int GetInputValue(FUInstance* instance,int port);

Edge* ConnectUnits(FUInstance* out,int outIndex,FUInstance* in,int inIndex);

void VersatUnitWrite(FUInstance* instance,int address, int value);
int VersatUnitRead(FUInstance* instance,int address);

int CalculateLatency(FUInstance* inst);
void CalculateDelay(Versat* versat,Accelerator* accel);
void SetDelayRecursive(Accelerator* accel);
void CalculateGraphData(Accelerator* accel);
void CalculateVersatData(Accelerator* accel);

ConsolidationGraph GenerateConsolidationGraph(Accelerator* accel1,Accelerator* accel2);
ConsolidationGraph MaxClique(ConsolidationGraph graph);
Accelerator* MergeGraphs(Versat* versat,Accelerator* accel1,Accelerator* accel2,ConsolidationGraph graph);

SubgraphData SubGraphAroundInstance(Versat* versat,Accelerator* accel,FUInstance* instance,int layers);

FUInstance* GetInstance(Accelerator* circuit,std::initializer_list<char*> names);

void Hook(Versat* versat,Accelerator* accel,FUInstance* inst);

#endif // INCLUDED_VERSAT_HPP

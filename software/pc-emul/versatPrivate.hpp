#ifndef INCLUDED_VERSAT_PRIVATE_HPP
#define INCLUDED_VERSAT_PRIVATE_HPP

#include <vector>
#include <unordered_map>

#include "memory.hpp"
#include "logger.hpp"

#include "versat.hpp"

struct ComplexFUInstance;

typedef int* (*FUFunction)(ComplexFUInstance*);
typedef int (*MemoryAccessFunction)(ComplexFUInstance* instance, int address, int value,int write);

enum DelayType {
   DELAY_TYPE_BASE               = 0x0,
   DELAY_TYPE_SINK_DELAY         = 0x1,
   DELAY_TYPE_SOURCE_DELAY       = 0x2,
   DELAY_TYPE_COMPUTE_DELAY      = 0x4
};
#define CHECK_DELAY(inst,T) ((inst->declaration->delayType & T) == T)

inline DelayType operator|(DelayType a, DelayType b)
{return static_cast<DelayType>(static_cast<int>(a) | static_cast<int>(b));}

// So far, this type of graph is only useful for consolidation graphs.
template<typename Node,typename Edge>
class Graph{
public:
   Array<Node> nodes;
   Array<Edge> edges;

   BitArray validNodes;
};

struct Wire{
   SizedString name;
   int bitsize;
};

struct StaticId{
   FUDeclaration* parent;
   SizedString name;
};

struct StaticData{
   Array<Wire> configs;
   int offset;
};

struct StaticInfo{
   StaticId id;
   StaticData data;
};

struct PortInstance{
   ComplexFUInstance* inst;
   int port;
};

struct ConnectionInfo{
   PortInstance instConnectedTo;
   int port;
   int edgeDelay; // Stores the edge delay information
   int* delay; // Used to calculate delay. Does not store edge delay information
};

struct Edge{ // A edge in a graph
   PortInstance units[2];
   int delay;
};

struct DAGOrder{
   ComplexFUInstance** sinks;
   int numberSinks;
   ComplexFUInstance** sources;
   int numberSources;
   ComplexFUInstance** computeUnits;
   int numberComputeUnits;
   ComplexFUInstance** instances;
};

// A declaration is constant after being registered
struct FUDeclaration{
   SizedString name;

   Array<int> inputDelays;
   Array<int> outputLatencies;

   Array<Wire> configs;
   Array<Wire> states;

   // Care, these only make sense for composite units. Don't forget that Single units do not use these.
   // Do not use their size to make decisions unless you know you are dealing with composite units
   Array<int> configOffsets;
   Array<int> stateOffsets;
   Array<int> delayOffsets;
   Array<int> outputOffsets;
   Array<int> extraDataOffsets;

   Array<int> defaultConfig;
   Array<int> defaultStatic;

   int totalOutputs; // Temp

   int nDelays; // Code only handles 1 single instance, for now, hardware needs this value for correct generation
   int nIOs;
   int memoryMapBits;
   int nStaticConfigs;
   int extraDataSize;

   // Stores different accelerators depending on properties we want
   Accelerator* baseCircuit;
   Accelerator* fixedMultiEdgeCircuit;
   Accelerator* fixedDelayCircuit;

   DAGOrder temporaryOrder;

   // Merged accelerator
   FUDeclaration* mergedType[2]; // TODO: probably change it from static to a dynamic allocating with more space, in order to accommodate multiple mergings (merge A with B and then the result with C)

   // Iterative
   SizedString unitName;
   FUDeclaration* baseDeclaration;
   Accelerator* initial;
   Accelerator* forLoop;
   int dataSize;

   FUFunction initializeFunction;
   FUFunction startFunction;
   FUFunction updateFunction;
   FUFunction destroyFunction;
   MemoryAccessFunction memAccessFunction;

   const char* operation;
   Hashmap<StaticId,StaticData> staticUnits;

   enum {SINGLE = 0x0,COMPOSITE = 0x1,SPECIAL = 0x2,MERGED = 0x3,ITERATIVE = 0x4} type;
   DelayType delayType;

   bool isOperation;
   bool implementsDone;
   bool isMemoryMapped;
};

struct GraphComputedData{
   Array<ConnectionInfo> allInputs; // All connections, even repeated ones
   Array<ConnectionInfo> allOutputs;
   Array<PortInstance> singleInputs; // Assume no repetition. If repetion exists, multipleSamePortInputs is true. If not connected, port instance inst is nullptr
   int outputs; // No single outputs because not much reasons to.
   enum {TAG_UNCONNECTED,TAG_COMPUTE,TAG_SOURCE,TAG_SINK,TAG_SOURCE_AND_SINK} nodeType;
   int inputDelay;
   int order;
   bool multipleSamePortInputs;
};

struct VersatComputedData{
   int memoryMaskSize;
   int memoryAddressOffset;
   char memoryMask[33];
};

struct Parameter{
   SizedString name;
   SizedString value;
   Parameter* next;
};

struct ComplexFUInstance : public FUInstance{
   // Various uses
   FUInstance* declarationInstance;

   Accelerator* iterative;

   int bufferAmount;
   GraphComputedData* graphData;
   VersatComputedData* versatData;
   char tag;
   bool savedConfiguration; // For subunits registered, indicate when we save configuration before hand
   bool savedMemory; // Same for memory
   int sharedIndex;
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

struct Versat{
	Pool<FUDeclaration> declarations;
	Pool<Accelerator> accelerators;

	Arena permanent;
	Arena temp;

	int base;
	int numberConfigurations;

   DebugState debug;
};

struct Test{
   FUInstance* inst;
   ComplexFUInstance storedValues;
   bool usingStoredValues;
};

struct Accelerator{ // Graph + data storage
   Versat* versat;
   FUDeclaration* subtype; // Set if subaccelerator (accelerator associated to a FUDeclaration). A "free" accelerator has this set to nullptr

   Pool<ComplexFUInstance> instances;
	Pool<Edge> edges;

   Pool<ComplexFUInstance> subInstances;

   Allocation<int> configAlloc;
   Allocation<int> stateAlloc;
   Allocation<int> delayAlloc;
   Allocation<int> staticAlloc;
   Allocation<Byte> extraDataAlloc;

   Allocation<int> outputAlloc;
   Allocation<int> storedOutputAlloc;

   Pool<StaticInfo> staticInfo;

	void* configuration;
	int configurationSize;

	int cyclesPerRun;

	int entityId;
	bool init;
};

struct EdgeView{
   Edge* edge; // Points to edge inside accelerator
   ComplexFUInstance** nodes[2]; // Points to node inside AcceleratorView
   int delay;
};

// A view into an accelerator.
class AcceleratorView{ // More like a "graph view"
public:
   // We use dynamic memory for nodes and edges, as we need to add/remove nodes to the graph and to update the associated accelerator view as well
   Pool<ComplexFUInstance*> nodes;
   Pool<EdgeView> edges;

   Versat* versat;
   Accelerator* accel;

   // All the other memory is meant to be stored in an arena. A temp arena for temporary use or permanent if to store in a FUDeclaration
   Array<Byte> graphData;
   Array<VersatComputedData> versatData;
   DAGOrder order;
   bool dagOrder;

public:

   void CalculateGraphData(Arena* arena);
   void SetGraphData();

   void CalculateDelay(Arena* arena,bool outputDebugGraph = false);
   DAGOrder CalculateDAGOrdering(Arena* arena);
   void CalculateVersatData(Arena* arena);
};

struct HashKey{
   SizedString key;
   int data;
};

class AcceleratorIterator{
public:
   Array<PoolIterator<ComplexFUInstance>> stack;
   Hashmap<StaticId,StaticData>* staticUnits;
   Accelerator* topLevel;
   int level;
   bool calledStart;
   bool populate;

   // Must call first
   ComplexFUInstance* Start(Accelerator* topLevel,ComplexFUInstance* compositeInst,Arena* temp,bool populate = false);
   ComplexFUInstance* Start(Accelerator* topLevel,Arena* temp,bool populate = false);

   ComplexFUInstance* Descend(); // Current() must be a composite instance, otherwise this will fail

   ComplexFUInstance* Next(); // Iterates over subunits
   ComplexFUInstance* Skip(); // Next unit on the same level

   ComplexFUInstance* Current(); // Returns nullptr to indicate end of iteration
   ComplexFUInstance* CurrentAcceleratorInstance(); // Returns the accelerator instance for the Current() instance or nullptr if currently at top level

   AcceleratorIterator LevelBelowIterator(Arena* temp); // Current() must be a composite instance, Returns an iterator that will iterate starting from the level below, but will end without going to upper levels.
   AcceleratorIterator LevelBelowIterator(); // Not taking an arena means that the returned iterator uses current iterator memory. Returned iterator must be iterated fully before the current iterator can be used, otherwise memory conflicts will arise as both iterators are sharing the same stack
};

struct IterativeUnitDeclaration{
   SizedString name;
   SizedString unitName;
   FUDeclaration* baseDeclaration;
   Accelerator* initial;
   Accelerator* forLoop;

   int dataSize;
   int latency;
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

   int memoryMappedBits;
   bool isMemoryMapped;
};

struct VersatComputedValues{
   int nConfigs;
   int configBits;

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
   int maxMemoryMapDWords;

   int nUnitsIO;

   int numberConnections;

   int stateConfigurationAddressBits;
   int memoryAddressBits;
   int memoryMappingAddressBits;
   int memoryConfigDecisionBit;
   int lowerAddressSize;
};

struct HierarchicalName{
   SizedString name;
   HierarchicalName* next;
};

struct FUInstanceInterfaces{
   PushPtr<int> config;
   PushPtr<int> state;
   PushPtr<int> delay;
   PushPtr<int> outputs;
   PushPtr<int> storedOutputs;
   PushPtr<int> statics;
   PushPtr<Byte> extraData;
};

struct SharingInfo{
   int* ptr;
   bool init;
};

struct SpecificMergeNodes{
   ComplexFUInstance* instA;
   ComplexFUInstance* instB;
};

struct MergeEdge{
   ComplexFUInstance* instances[2];
};

struct PortEdge{
   PortInstance units[2];
};

struct MappingNode{ // Mapping (edge to edge or node to node)
   union{
      MergeEdge nodes;
      PortEdge edges[2];
   };

   enum {NODE,EDGE} type;
};

struct MappingEdge{ // Edge between mapping from edge to edge
   MappingNode* nodes[2];
};

struct Mapping{
   FUInstance* source;
   FUInstance* sink;
};

struct ConsolidationGraphOptions{
   SpecificMergeNodes* specifics;
   int nSpecifics;
   int order;
   int difference;
   bool mapNodes;
   enum {NOTHING,SAME_ORDER,EXACT_ORDER} type;
};

namespace BasicDeclaration{
	extern FUDeclaration* buffer;
   extern FUDeclaration* fixedBuffer;
   extern FUDeclaration* input;
   extern FUDeclaration* output;
   extern FUDeclaration* multiplexer;
   extern FUDeclaration* combMultiplexer;
   extern FUDeclaration* pipelineRegister;
   extern FUDeclaration* data;
}

typedef std::unordered_map<ComplexFUInstance*,ComplexFUInstance*> InstanceMap;
typedef Graph<MappingNode,MappingEdge> ConsolidationGraph;

// Temp
void PopulateAccelerator(Accelerator* topLevel,Accelerator* accel,FUDeclaration* topDeclaration,FUInstanceInterfaces& inter,Hashmap<StaticId,StaticData>& staticMap);
void PopulateAccelerator2(Accelerator* accel,FUDeclaration* topDeclaration,FUInstanceInterfaces& inter,Hashmap<StaticId,StaticData>& staticMap);

// General info
UnitValues CalculateIndividualUnitValues(ComplexFUInstance* inst); // Values for individual unit, not taking into account sub units. For a composite, this pretty much returns empty except for total outputs, as the unit itself must allocate output memory
UnitValues CalculateAcceleratorUnitValues(Versat* versat,ComplexFUInstance* inst); // Values taking into account sub units
UnitValues CalculateAcceleratorValues(Versat* versat,Accelerator* accel);
VersatComputedValues ComputeVersatValues(Versat* versat,Accelerator* accel);
int CalculateTotalOutputs(Accelerator* accel);
int CalculateTotalOutputs(FUInstance* inst);
bool IsConfigStatic(Accelerator* topLevel,ComplexFUInstance* inst);
bool IsUnitCombinatorial(FUInstance* inst);

// Accelerator
Accelerator* CopyAccelerator(Versat* versat,Accelerator* accel,InstanceMap* map,bool flat);
ComplexFUInstance* CopyInstance(Accelerator* newAccel,ComplexFUInstance* oldInstance,SizedString newName,bool flat);
void InitializeFUInstances(Accelerator* accel,bool force);
void CompressAcceleratorMemory(Accelerator* accel);

// Unit connection (returns Edge)
Edge* FindEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay = 0);
Edge* ConnectUnitsGetEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay = 0);
Edge* ConnectUnitsIfNotConnectedGetEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay = 0);
Edge* ConnectUnits(PortInstance out,PortInstance in,int delay = 0);

// Delay
int CalculateLatency(ComplexFUInstance* inst);
void CalculateDelay(Versat* versat,Accelerator* accel);
void SetDelayRecursive(Accelerator* accel);

// Graph fixes
void FixMultipleInputs(Versat* versat,Accelerator* accel);
void FixDelays(Versat* versat,Accelerator* accel,AcceleratorView& view);

// AcceleratorView related functions
AcceleratorView CreateAcceleratorView(Accelerator* accel,Arena* arena);
AcceleratorView CreateAcceleratorView(Accelerator* accel,std::vector<Edge*>& edgeMappings,Arena* arena);
AcceleratorView SubGraphAroundInstance(Versat* versat,Accelerator* accel,ComplexFUInstance* instance,int layers,Arena* arena);

// Accelerator merging
bool MappingConflict(MappingNode map1,MappingNode map2);
ConsolidationGraph MaxClique(ConsolidationGraph graph,Arena* arena);
ConsolidationGraph GenerateConsolidationGraph(Versat* versat,Arena* arena,Accelerator* accel1,Accelerator* accel2,ConsolidationGraphOptions options,MergingStrategy strategy);
Accelerator* MergeAccelerator(Versat* versat,Accelerator* accel1,Accelerator* accel2,SpecificMergeNodes* specificNodes,int nSpecifics,MergingStrategy strategy);

// Debug
bool IsGraphValid(AcceleratorView view);
void OutputGraphDotFile(Versat* versat,AcceleratorView& view,bool collapseSameEdges,ComplexFUInstance* highlighInstance,const char* filenameFormat,...) __attribute__ ((format (printf, 5, 6)));
void CheckMemory(AcceleratorIterator iter);

// Misc
bool CheckValidName(SizedString name); // Check if name can be used as identifier in verilog
SizedString MappingNodeIdentifier(MappingNode* node,Arena* memory);
void OutputCircuitSource(Versat* versat,FUDeclaration* decl,Accelerator* accel,FILE* file);
ComplexFUInstance* GetInputInstance(Accelerator* accel,int inputIndex);
ComplexFUInstance* GetOutputInstance(Accelerator* accel);

#include "typeSpecifics.inl"

#endif // INCLUDED_VERSAT_PRIVATE_HPP

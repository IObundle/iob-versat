#ifndef INCLUDED_VERSAT_PRIVATE_HPP
#define INCLUDED_VERSAT_PRIVATE_HPP

#include <vector>
#include <unordered_map>

#include "memory.hpp"
#include "logger.hpp"

#include "versat.hpp"

struct ComplexFUInstance;
struct Versat;

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

struct Wire{
   String name;
   int bitsize;
};

struct StaticId{
   FUDeclaration* parent;
   String name;
};

struct StaticData{
   FUDeclaration* decl;
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

struct PortEdge{
   PortInstance units[2];
};

struct Edge{ // A edge in a graph
   union{
      struct{
         PortInstance out;
         PortInstance in;
      };
      PortEdge edge;
      PortInstance units[2];
   };

   int delay;
   Edge* next;
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

struct SimpleNode{
   FUDeclaration* decl;
};

struct SimpleEdge{
   int out;
   int outPort;
   int in;
   int inPort;
};

struct SimpleGraph{
   Array<SimpleNode> nodes;
   Array<SimpleEdge> edges;
};

struct CalculatedOffsets{
   Array<int> offsets;
   int max;
};

// A declaration is constant after being registered
struct FUDeclaration{
   String name;

   Array<int> inputDelays;
   Array<int> outputLatencies;

   Array<Wire> configs;
   Array<Wire> states;

   // Care, these only make sense for composite units. Don't forget that Single units do not use these.
   // Do not use their size to make decisions unless you know you are dealing with composite units
   CalculatedOffsets configOffsets;
   CalculatedOffsets stateOffsets;
   CalculatedOffsets delayOffsets;
   CalculatedOffsets outputOffsets;
   CalculatedOffsets extraDataOffsets;

   Array<int> defaultConfig;
   Array<int> defaultStatic;

   int totalOutputs; // Temp

   int nDelays; // Code only handles 1 single instance, for now, hardware needs this value for correct generation
   int nIOs;
   int memoryMapBits;
   int nStaticConfigs;
   int extraDataSize;
   int externalMemoryBitsize;
   int externalMemoryDatasize;

   // Stores different accelerators depending on properties we want
   Accelerator* baseCircuit;
   Accelerator* fixedDelayCircuit;
   SimpleGraph fixedDelayCircuitSimple;

   DAGOrder temporaryOrder;

   // Merged accelerator
   Array<FUDeclaration*> mergedType; // TODO: probably change it from static to a dynamic allocating with more space, in order to accommodate multiple mergings (merge A with B and then the result with C)

   // Iterative
   String unitName;
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
   Hashmap<StaticId,StaticData>* staticUnits;

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
   String name;
   String value;
   Parameter* next;
};

struct ComplexFUInstance;
struct InstanceNode;

struct PortNode{
   InstanceNode* node;
   int port;
};

struct EdgeNode{
   PortNode node0;
   PortNode node1;
};

struct ConnectionNode{
   PortNode instConnectedTo;
   int port;
   int edgeDelay;
   int* delay; // Maybe not the best approach to calculate delay. TODO: check later
   ConnectionNode* next;
};

struct InstanceNode{
   ComplexFUInstance* inst;
   InstanceNode* next;

   // Calculated and updated every time a connection is added or removed
   ConnectionNode* allInputs;
   ConnectionNode* allOutputs;
   Array<PortNode> inputs;
   int outputs; // No single outputs because not much reasons to.
   bool multipleSamePortInputs;
   enum {TAG_UNCONNECTED,TAG_COMPUTE,TAG_SOURCE,TAG_SINK,TAG_SOURCE_AND_SINK} type;

   // The following is only calculated by specific functions. Otherwise assume data is old if any change to the graph occurs
   // In fact, maybe this should be removed and should just be a hashmap + array to save computed data.
   // But need to care because top level accelerator still needs to be able to calculate this values and persist them somehow
   int inputDelay;
   //int order;
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
   int maxOrder;
};

struct ComplexFUInstance : public FUInstance{
   // Various uses
   FUInstance* declarationInstance;

   Accelerator* iterative;

   //ComplexFUInstance* next;

   //InstanceNode* inputs;
   //InstanceNode* outputs;

   union{
   int literal;
   int bufferAmount;
   int portIndex;
   };
   //GraphComputedData* graphData;
   VersatComputedData* versatData;
   int sharedIndex;
   char tag;
   bool savedConfiguration; // For subunits registered, indicate when we save configuration before hand
   bool savedMemory; // Same for memory
   bool sharedEnable;
   bool initialized;
   bool debugBreak;
};

struct OrderedInstance{ // This is a more powerful and generic approach to subgraphing. Should not be named ordered, could be used for anything really, not just order iteration
   InstanceNode* node;
   OrderedInstance* next;
};

struct Accelerator{ // Graph + data storage
   Versat* versat;
   FUDeclaration* subtype; // Set if subaccelerator (accelerator associated to a FUDeclaration). A "free" accelerator has this set to nullptr

   //ComplexFUInstance* instances;
   //ComplexFUInstance* lastAdded;

   //int numberInstances;
   //int numberEdges;
   Edge* edges;

   OrderedInstance* ordered;
   //InstanceNode* ordered;
   InstanceNode* allocated;
   InstanceNode* lastAllocated;
   Pool<ComplexFUInstance> instances;
   Pool<ComplexFUInstance> subInstances;

   Allocation<int> configAlloc;
   Allocation<int> stateAlloc;
   Allocation<int> delayAlloc;
   Allocation<int> staticAlloc;
   Allocation<Byte> extraDataAlloc;
   Allocation<int> externalMemoryAlloc;

   Allocation<int> outputAlloc;
   Allocation<int> storedOutputAlloc;

   Arena instancesMemory;
   Arena edgesMemory;
   Arena temp;

   Pool<StaticInfo> staticInfo;

	void* configuration;
	int configurationSize;

	int cyclesPerRun;

	int created;
	int entityId;
	bool init;
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

struct EdgeView{
   Edge* edge; // Points to edge inside accelerator
   int delay;
};

// A view into an accelerator.
#if 0
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
#endif

struct HashKey{
   String key;
   int data;
};

struct IterativeUnitDeclaration{
   String name;
   String unitName;
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
   int externalMemory;

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
   String name;
   HierarchicalName* next;
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
   Array<SpecificMergeNodes> specifics;
   int order;
   int difference;
   bool mapNodes;
   enum {NOTHING,SAME_ORDER,EXACT_ORDER} type;
};

class FUInstanceInterfaces{
public:
   PushPtr<int> config;
   PushPtr<int> state;
   PushPtr<int> delay;
   PushPtr<int> outputs;
   PushPtr<int> storedOutputs;
   PushPtr<int> statics;
   PushPtr<Byte> extraData;
   PushPtr<int> externalMemory;

   void Init(Accelerator* accel);
   void Init(Accelerator* topLevel,ComplexFUInstance* inst);

   void AssertEmpty(bool checkStatic = true);
};

class AcceleratorIterator{
public:
   union Type{
      InstanceNode* node;
      OrderedInstance* ordered;
   };

   Array<Type> stack;
   Hashmap<StaticId,StaticData>* staticUnits;
   Accelerator* topLevel;
   int level;
   int upperLevels;
   bool calledStart;
   bool populate;
   bool ordered;
   bool levelBelow;

   InstanceNode* GetInstance(int level);

   // Must call first
   InstanceNode* Start(Accelerator* topLevel,ComplexFUInstance* compositeInst,Arena* temp,bool populate = false);
   InstanceNode* Start(Accelerator* topLevel,Arena* temp,bool populate = false);

   InstanceNode* StartOrdered(Accelerator* topLevel,Arena* temp,bool populate = false);

   InstanceNode* ParentInstance();
   String GetParentInstanceFullName(Arena* out);

   InstanceNode* Descend(); // Current() must be a composite instance, otherwise this will fail

   InstanceNode* Next(); // Iterates over subunits
   InstanceNode* Skip(); // Next unit on the same level

   InstanceNode* Current(); // Returns nullptr to indicate end of iteration
   InstanceNode* CurrentAcceleratorInstance(); // Returns the accelerator instance for the Current() instance or nullptr if currently at top level

   String GetFullName(Arena* out);

   AcceleratorIterator LevelBelowIterator(Arena* temp); // Current() must be a composite instance, Returns an iterator that will iterate starting from the level below, but will end without going to upper levels.
   AcceleratorIterator LevelBelowIterator(); // Not taking an arena means that the returned iterator uses current iterator memory. Returned iterator must be iterated fully before the current iterator can be used, otherwise memory conflicts will arise as both iterators are sharing the same stack
};

struct ConsolidationGraph{
   Array<MappingNode> nodes;
   Array<BitArray> edges;

   BitArray validNodes;
};

struct ConsolidationResult{
   ConsolidationGraph graph;
   Pool<MappingNode> specificsAdded;
   int upperBound;
};

struct CliqueState{
   int max;
   int upperBound;
   int startI;
   int iterations;
   Array<int> table;
   ConsolidationGraph clique;
   clock_t start;
   bool found;
};

typedef std::unordered_map<ComplexFUInstance*,ComplexFUInstance*> InstanceMap;
typedef std::unordered_map<PortEdge,PortEdge> PortEdgeMap;
typedef std::unordered_map<Edge*,Edge*> EdgeMap;
typedef std::unordered_map<ComplexFUInstance*,ComplexFUInstance*> InstanceMap;
typedef Hashmap<InstanceNode*,InstanceNode*> InstanceNodeMap;

struct MergeGraphResult{
   Accelerator* accel1; // Should pull out the graph stuff instead of using an Accelerator for this
   Accelerator* accel2;

   InstanceNodeMap* map1; // Maps node from accel1 to newGraph
   InstanceNodeMap* map2; // Maps node from accel2 to newGraph
   //std::vector<Edge*> accel1EdgeMap;
   //std::vector<Edge*> accel2EdgeMap;
   Accelerator* newGraph;
};

struct MergeGraphResultExisting{
   Accelerator* result;
   Accelerator* accel2;
   InstanceNodeMap* map2;
};

// Simple operations should also be stored here. They are versat agnostic as well
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

struct CompiledTemplate;
namespace BasicTemplates{
   extern CompiledTemplate* acceleratorTemplate;
   extern CompiledTemplate* topAcceleratorTemplate;
   extern CompiledTemplate* dataTemplate;
}

struct GraphMapping;

// Temp
bool EqualPortMapping(PortInstance p1,PortInstance p2);
void PopulateAccelerator(Accelerator* topLevel,Accelerator* accel,FUDeclaration* topDeclaration,FUInstanceInterfaces& inter,Hashmap<StaticId,StaticData>* staticMap);
void PopulateAccelerator2(Accelerator* accel,FUDeclaration* topDeclaration,FUInstanceInterfaces& inter,Hashmap<StaticId,StaticData>* staticMap);
ConsolidationResult GenerateConsolidationGraph(Versat* versat,Arena* arena,Accelerator* accel1,Accelerator* accel2,ConsolidationGraphOptions options);
MergeGraphResult MergeGraph(Versat* versat,Accelerator* flatten1,Accelerator* flatten2,GraphMapping& graphMapping,String name);
void AddCliqueToMapping(GraphMapping& res,ConsolidationGraph clique);

// General info
UnitValues CalculateIndividualUnitValues(ComplexFUInstance* inst); // Values for individual unit, not taking into account sub units. For a composite, this pretty much returns empty except for total outputs, as the unit itself must allocate output memory
UnitValues CalculateAcceleratorUnitValues(Versat* versat,ComplexFUInstance* inst); // Values taking into account sub units
UnitValues CalculateAcceleratorValues(Versat* versat,Accelerator* accel);
VersatComputedValues ComputeVersatValues(Versat* versat,Accelerator* accel);
int CalculateTotalOutputs(Accelerator* accel);
int CalculateTotalOutputs(FUInstance* inst);
bool IsConfigStatic(Accelerator* topLevel,ComplexFUInstance* inst);
bool IsUnitCombinatorial(FUInstance* inst);

void ReorganizeAccelerator(Accelerator* graph,Arena* temp);

// Accelerator
Accelerator* CopyAccelerator(Versat* versat,Accelerator* accel,InstanceMap* map);
Accelerator* CopyFlatAccelerator(Versat* versat,Accelerator* accel,InstanceMap* map);
ComplexFUInstance* CopyInstance(Accelerator* newAccel,ComplexFUInstance* oldInstance,String newName,InstanceNode* previous);
ComplexFUInstance* CopyInstance(Accelerator* newAccel,ComplexFUInstance* oldInstance,String newName);
InstanceNode* CopyInstance(Accelerator* newAccel,InstanceNode* oldInstance,String newName);
InstanceNode* CreateFlatFUInstance(Accelerator* accel,FUDeclaration* type,String name);
void InitializeFUInstances(Accelerator* accel,bool force);
int CountNonOperationChilds(Accelerator* accel);

// Unit connection (returns Edge)
Edge* FindEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay = 0);
Edge* ConnectUnitsGetEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay = 0);
Edge* ConnectUnitsGetEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay,Edge* previous);
Edge* ConnectUnitsIfNotConnectedGetEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay = 0);
Edge* ConnectUnits(PortInstance out,PortInstance in,int delay = 0);
Edge* ConnectUnits(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay,Edge* previousEdge);

// Delay
int CalculateLatency(ComplexFUInstance* inst);
Hashmap<EdgeNode,int>* CalculateDelay(Versat* versat,Accelerator* accel,Arena* out);
void SetDelayRecursive(Accelerator* accel);

// Graph fixes
void FixMultipleInputs(Versat* versat,Accelerator* accel);
void FixMultipleInputs(Versat* versat,Accelerator* accel,Hashmap<ComplexFUInstance*,int>* instanceToInput);
//void FixDelays(Versat* versat,Accelerator* accel,AcceleratorView& view);
void FixDelays(Versat* versat,Accelerator* accel,Hashmap<EdgeNode,int>* edgeDelays);

// AcceleratorView related functions
//AcceleratorView CreateAcceleratorView(Accelerator* accel,Arena* arena);
//AcceleratorView CreateAcceleratorView(Accelerator* accel,std::vector<Edge*>& edgeMappings,Arena* arena);
//AcceleratorView SubGraphAroundInstance(Versat* versat,Accelerator* accel,ComplexFUInstance* instance,int layers,Arena* arena);

// Accelerator merging
bool MappingConflict(MappingNode map1,MappingNode map2);
CliqueState MaxClique(ConsolidationGraph graph,int upperBound,Arena* arena,float MAX_CLIQUE_TIME);
ConsolidationGraph GenerateConsolidationGraph(Versat* versat,Arena* arena,Accelerator* accel1,Accelerator* accel2,ConsolidationGraphOptions options,MergingStrategy strategy);
//GraphMapping MergeAccelerator(Versat* versat,Accelerator* accel1,Accelerator* accel2,SpecificMergeNodes* specificNodes,int nSpecifics,MergingStrategy strategy,String name);

DAGOrderNodes CalculateDAGOrder(InstanceNode* instances,Arena* arena);

// Debug
void AssertGraphValid(InstanceNode* nodes,Arena* arena);
void OutputGraphDotFile(Versat* versat,Accelerator* accel,bool collapseSameEdges,const char* filenameFormat,...) __attribute__ ((format (printf, 4, 5)));
void OutputGraphDotFile(Versat* versat,Accelerator* accel,bool collapseSameEdges,ComplexFUInstance* highlighInstance,const char* filenameFormat,...) __attribute__ ((format (printf, 5, 6)));
void OutputGraphDotFile(Versat* versat,Accelerator* accel,bool collapseSameEdges,Set<ComplexFUInstance*>* highlight,const char* filenameFormat,...) __attribute__ ((format (printf, 5, 6)));
void CheckMemory(AcceleratorIterator iter);
void CheckMemory(AcceleratorIterator iter,MemType type);
void CheckMemory(AcceleratorIterator iter,MemType type,Arena* arena);
int NumberUnits(Accelerator* accel,Arena* arena); // TODO: Check configurations.cpp
Array<String> GetFullNames(Accelerator* accel,Arena* out);

void OutputConsolidationGraph(ConsolidationGraph graph,Arena* memory,bool onlyOutputValid,const char* format,...);

// Misc
bool CheckValidName(String name); // Check if name can be used as identifier in verilog
String MappingNodeIdentifier(MappingNode* node,Arena* memory);
int CountNonSpecialChilds(InstanceNode* nodes);
void OutputCircuitSource(Versat* versat,FUDeclaration* decl,Accelerator* accel,FILE* file);
InstanceNode* GetInputNode(InstanceNode* nodes,int inputIndex);
int GetInputValue(InstanceNode* nodes,int index);
ComplexFUInstance* GetInputInstance(InstanceNode* nodes,int inputIndex);
InstanceNode* GetOutputNode(InstanceNode* nodes);
ComplexFUInstance* GetOutputInstance(InstanceNode* nodes);
PortNode GetInputValueInstance(FUInstance* inst,int index);

MergeGraphResult HierarchicalMergeAccelerators(Versat* versat,Accelerator* accel1,Accelerator* accel2,String name);
MergeGraphResult HierarchicalMergeAcceleratorsFullClique(Versat* versat,Accelerator* accel1,Accelerator* accel2,String name);

#include "typeSpecifics.inl"

struct GraphMapping{
   InstanceMap instanceMap;
   InstanceMap reverseInstanceMap;
   PortEdgeMap edgeMap;
};


#endif // INCLUDED_VERSAT_PRIVATE_HPP

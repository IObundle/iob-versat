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

template<typename Node,typename Edge> //
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

struct StaticInfo{
   FUDeclaration* module;
   SizedString name;
   Wire* wires;
   int nConfigs;
   int* ptr; // Pointer to config of existing unit
};

struct PortInstance{
   ComplexFUInstance* inst;
   int port;
};

struct ConnectionInfo{
   PortInstance instConnectedTo;
   int port;
   int delay;
};

struct Edge{ // A edge in a graph
   PortInstance units[2];
   int delay;
};

// A declaration is constant after being registered
struct FUDeclaration{
   SizedString name;

   int nInputs;
   int* inputDelays;
   int nOutputs;
   int* latencies;

   // Interfaces
   int nConfigs;
   Wire* configWires;
   int nStates;
   Wire* stateWires;
   int nDelays; // Code only handles 1 single instace, for now, hardware needs this value for correct generation
   int nIOs;
   int memoryMapBits;
   int nStaticConfigs;
   int extraDataSize;

   // Stores different accelerators depending on properties we want
   Accelerator* baseCircuit;
   Accelerator* fixedMultiEdgeCircuit;
   Accelerator* fixedDelayCircuit;

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
   Pool<StaticInfo> staticUnits;

   enum {SINGLE = 0x0,COMPOSITE = 0x1,SPECIAL = 0x2,MERGED = 0x3,ITERATIVE = 0x4} type;
   DelayType delayType;

   bool isOperation;
   bool implementsDone;
   bool isMemoryMapped;
};

struct GraphComputedData{
   int numberInputs;
   int numberOutputs;
   int inputPortsUsed;
   int outputPortsUsed;
   ConnectionInfo* inputs;
   ConnectionInfo* outputs;
   Array<PortInstance> inputToPortInstance;
   enum {TAG_UNCONNECTED,TAG_COMPUTE,TAG_SOURCE,TAG_SINK,TAG_SOURCE_AND_SINK} nodeType;
   int inputDelay;
   int order;
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

	// Declaration for units that versat needs to instantiate itself
	FUDeclaration* buffer;
   FUDeclaration* fixedBuffer;
   FUDeclaration* input;
   FUDeclaration* output;
   FUDeclaration* multiplexer;
   FUDeclaration* combMultiplexer;
   FUDeclaration* pipelineRegister;
   FUDeclaration* data;

   DebugState debug;

   // Command line options
   std::vector<const char*> includeDirs;
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

   Pool<ComplexFUInstance*> inputInstancePointers;
   ComplexFUInstance* outputInstance;

   Allocation<int> configAlloc;
   Allocation<int> stateAlloc;
   Allocation<int> delayAlloc;
   Allocation<int> staticAlloc;
   Allocation<int> outputAlloc;
   Allocation<int> storedOutputAlloc;
   Allocation<Byte> extraDataAlloc;

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

// Graph associated to an accelerator
class AcceleratorView : public Graph<ComplexFUInstance*,EdgeView>{
public:
   DAGOrder order;
   bool dagOrder;
   bool graphData;
   bool versatData;
   Accelerator* accel;

   void CalculateGraphData(Arena* arena);
   void CalculateDelay(Arena* arena);
   DAGOrder CalculateDAGOrdering(Arena* arena);
   void CalculateVersatData(Arena* arena);
};

struct HashKey{
   SizedString key;
   int data;
};

class AcceleratorIterator{
public:
   PoolIterator<ComplexFUInstance> stack[16];
   int index;
   bool calledStart;

   ComplexFUInstance* Start(Accelerator* accel); // Must call first
   ComplexFUInstance* Next(); // Returns nullptr in the end

   ComplexFUInstance* CurrentAcceleratorInstance(); // Returns the top accelerator for the last FUInstance returned by Start or Next
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

struct StaticId{
   FUDeclaration* parent;
   SizedString name;
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

typedef std::unordered_map<ComplexFUInstance*,ComplexFUInstance*> InstanceMap;
typedef Graph<MappingNode,MappingEdge> ConsolidationGraph;

// General info
UnitValues CalculateIndividualUnitValues(ComplexFUInstance* inst); // Values for individual unit, not taking into account sub units
UnitValues CalculateAcceleratorUnitValues(Versat* versat,ComplexFUInstance* inst); // Values taking into account sub units
UnitValues CalculateAcceleratorValues(Versat* versat,Accelerator* accel);
VersatComputedValues ComputeVersatValues(Versat* versat,Accelerator* accel);
int CalculateTotalOutputs(Accelerator* accel);
int CalculateTotalOutputs(FUInstance* inst);
bool IsConfigStatic(Accelerator* topLevel,ComplexFUInstance* inst);
bool IsUnitCombinatorial(FUInstance* inst);

// Accelerator
void PopulateAccelerator(Versat* versat,Accelerator* accel);
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
void FixDelays(Versat* versat,Accelerator* accel,AcceleratorView view);

// AcceleratorView related functions
AcceleratorView CreateAcceleratorView(Accelerator* accel,Arena* arena);
AcceleratorView CreateAcceleratorView(Accelerator* accel,std::vector<Edge*>& edgeMappings,Arena* arena);
AcceleratorView SubGraphAroundInstance(Versat* versat,Accelerator* accel,ComplexFUInstance* instance,int layers,Arena* arena);
void ClearFUInstanceTempData(Accelerator* accel);

// Accelerator merging
bool MappingConflict(MappingNode map1,MappingNode map2);
ConsolidationGraph MaxClique(ConsolidationGraph graph,Arena* arena);
ConsolidationGraph GenerateConsolidationGraph(Versat* versat,Arena* arena,Accelerator* accel1,Accelerator* accel2,ConsolidationGraphOptions options,MergingStrategy strategy);
Accelerator* MergeAccelerator(Versat* versat,Accelerator* accel1,Accelerator* accel2,SpecificMergeNodes* specificNodes,int nSpecifics,MergingStrategy strategy);

// Debug
bool IsGraphValid(AcceleratorView view);
void OutputGraphDotFile(Versat* versat,AcceleratorView view,bool collapseSameEdges,const char* filenameFormat,...) __attribute__ ((format (printf, 4, 5)));

// Misc
bool CheckValidName(SizedString name); // Check if name can be used as identifier in verilog
SizedString MappingNodeIdentifier(MappingNode* node,Arena* memory);
void OutputCircuitSource(Versat* versat,FUDeclaration* decl,Accelerator* accel,FILE* file);

inline bool operator==(const PortInstance& p1,const PortInstance& p2){return p1.inst == p2.inst && p1.port == p2.port;};
inline bool operator==(const StaticId& id1,const StaticId& id2){
   bool res = CompareString(id1.name,id2.name) && id1.parent == id2.parent;
   return res;
}

template<> class std::hash<PortInstance>{
   public:
   std::size_t operator()(PortInstance const& s) const noexcept{
      int res = SimpleHash(MakeSizedString((const char*) &s,sizeof(PortInstance)));

      return (std::size_t) res;
   }
};

template<> class std::hash<StaticId>{
   public:
   std::size_t operator()(StaticId const& s) const noexcept{
      int res = SimpleHash(s.name);
      res += (int) s.parent;

      return (std::size_t) res;
   }
};

#endif // INCLUDED_VERSAT_PRIVATE_HPP

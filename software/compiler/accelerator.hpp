#pragma once

#include "utils.hpp"
#include "memory.hpp"
#include "verilogParsing.hpp"

struct FUInstance;
struct FUDeclaration;
struct Accelerator;
struct Edge;
struct AccelInfo;

typedef Hashmap<FUInstance*,FUInstance*> InstanceMap;
typedef Hashmap<Edge,Edge> EdgeMap;

#define PushInstanceMap(...) PushHashmap<FUInstance*,FUInstance*>(__VA_ARGS__)
#define PushEdgeMap(...) PushHashmap<Edge,Edge>(__VA_ARGS__)

struct PortInstance{
  FUInstance* inst;
  int port;
};

inline bool operator==(const PortInstance& p1,const PortInstance& p2){
  bool res = (p1.inst == p2.inst && p1.port == p2.port);
  return res;
}

inline bool operator!=(const PortInstance& p1,const PortInstance& p2){
  bool res = !(p1 == p2);
  return res;
}

template<> class std::hash<FUInstance*>{
public:
   std::size_t operator()(FUInstance* t) const noexcept{
     int offset = sizeof(void*) == 8 ? 3 : 2;
     std::size_t res = ((std::size_t) t) >> offset;
     return res;
   }
};

template<> class std::hash<PortInstance>{
public:
  std::size_t operator()(PortInstance const& s) const noexcept{
    std::size_t res = std::hash<FUInstance*>()(s.inst);
    res += s.port;

    return res;
  }
};

struct Edge{ // A edge in a graph
  union{
    struct{
      PortInstance out;
      PortInstance in;
    };
    PortInstance units[2];
  };

  int delay;
  Edge* next;
};

bool EdgeEqualNoDelay(const Edge& e0,const Edge& e1);

inline bool operator==(const Edge& e0,const Edge& e1){
  return (EdgeEqualNoDelay(e0,e1) && e0.delay == e1.delay);
}

struct EdgeDelayInfo{
  int* value;
  bool isAny;
};

// Need to formalize this. 
struct DelayInfo{
  int value;
  bool isAny; // True if the edge can have any delay at runtime (because attached to a variable buffer). If true, the value represents the minimum amount of delay needed by the edge (we can't have negative delays, even in an isAny edge)
};

// Three possibilities. A node contains zero isAny edges. A node contains all isAny edges. A node contains some isAny edges.

// First is easy. Third, the value is still the minimum of values and the node is still fixed. Second case the value is still the minimum but now the node can be isAny as well.

// All this stuff just makes me think of adding a proper constraint based approach to delay calculation. Basically model this as an CSP (or we just calculate expressions that calculate everything based on the value of other expressions) (we can then calculate the value of the expressions by giving an initial input value and the inputs are propragated by the fact that every expressions is dependent on every value).
// I think that problem is whether we actually gain anything from this. We would have greater power, but if we produce the same results, no point.

// TODO: Would like to see if using the ConnectionNode approach makes sense. We might be able to represent the edges in a list and simplify other parts of the code. Probably best to analyze how the graph structures are being used through the code base and make a decision then. Only merge needs to perform multiple graph mutations. Everything else kinda works fine if we use a more constant approach for the graphs.
struct ConnectionNode{
  PortInstance instConnectedTo;
  int port;
  int edgeDelay;
  // TODO: This is bad, we are storing something that is used only by the delay calculations functions and it is not a good approach because of merge. We already have the proper solution by using the AccelInfo approach, so now the only thing left is to remove the usage of this delay. 
  EdgeDelayInfo delay; // Maybe not the best approach to calculate delay. TODO: check later
  ConnectionNode* next;
};

enum NodeType{
  NodeType_UNCONNECTED,
  NodeType_SOURCE,
  NodeType_COMPUTE,
  NodeType_SINK,
  NodeType_SOURCE_AND_SINK
};

struct ParameterValue{
  String val; // Mostly a placeholder for now. Eventually want a better way of handling parameters and expression of parameters
};

struct FUInstance{
  String name;

  Array<ParameterValue> parameterValues; // Associates a value to the corresponding parameter on the associated FUDeclaration

  Accelerator* accel;

  FUDeclaration* declaration;
  int id;

  union{
    int literal;
    int bufferAmount;
    int portIndex;
    int muxGroup; // Merge multiplexers that belong to the same group must also have the same config (similar to shared units, but we want to separate the share mechanism from the mechanism used to represent multiplexers groups)
  };
  int sharedIndex;
  Array<bool> isSpecificConfigShared;
  bool isStatic;
  bool sharedEnable;
  bool isMergeMultiplexer; // TODO: Kinda of an hack for now
  
  // Calculated and updated every time a connection is added or removed
  ConnectionNode* allInputs;
  ConnectionNode* allOutputs;
  Array<PortInstance> inputs;
  Array<bool> outputs;

  Array<String> addressGenUsed;
  
  bool multipleSamePortInputs;
  NodeType type;
};

enum AcceleratorPurpose{
  AcceleratorPurpose_TEMP,
  AcceleratorPurpose_BASE,
  AcceleratorPurpose_FIXED_DELAY,
  AcceleratorPurpose_FLATTEN,
  AcceleratorPurpose_MODULE,
  AcceleratorPurpose_RECON,
  AcceleratorPurpose_MERGE
};

struct Accelerator{ // Graph + data storage
  Pool<FUInstance> allocated;
  
  DynamicArena* accelMemory; // TODO: Should just add linked list to normal arena and replace this with a simple arena.

   // Mainly for debugging
  String name; // For debugging purposes it's useful to give accelerators a name
  int id;
  AcceleratorPurpose purpose;
};

// NOTE: These values are specific to the top accelerator only. They differ because of stuff like DMA, the config interface for the accelerator and so on.
//       Any change that is specific to the top unit should change these values. More config space and things like that can be 'reserved' by changing the config values here and the same holds true for all the other interfaces.
//       Code that does not care about the top level unit should just use the values found in AccelInfo.
struct VersatComputedValues{
  int nConfigs;

  int versatConfigs;
  int versatStates;

  int nStatics;

  int nDelays;
  int delayBitsStart;

  int nUnits;
  int nDones;
  
  // Configurations = config + static + delays
  int configurationBits;
  int configurationAddressBits;

  int nStates;
  int stateBits;
  int stateAddressBits;

  int unitsMapped;
  int memoryMappedBytes;

  int nUnitsIO;
  int numberConnections;

  int memoryAddressBits;
  int memoryConfigDecisionBit;

  // External memories, not memory mapped.
  int externalMemoryInterfaces;
  int totalExternalMemory;  

  //bool signalLoop;
};

struct DAGOrderNodes{
  Array<FUInstance*> sinks;
  Array<FUInstance*> sources;
  Array<FUInstance*> computeUnits;
  Array<FUInstance*> instances;
  Array<int>           order;
  int size;
  int maxOrder;
};

struct AcceleratorMapping{
  int firstId;
  int secondId;

  TrieMap<PortInstance,PortInstance>* inputMap;
  TrieMap<PortInstance,PortInstance>* outputMap;
};

struct EdgeIterator{
  PoolIterator<FUInstance> currentNode;
  PoolIterator<FUInstance> end;
  ConnectionNode* currentPort;
  
  bool HasNext();
  Edge Next();
};

// Strange forward declare
template<typename T> struct WireTemplate;
typedef WireTemplate<int> Wire;

// TODO: Kinda not good, need to look at the wrapper again to simplif this part.
//       Too many stuff is dependent on explicit data and complications are starting to pile up.
struct WireExtra{
  String source;
  String source2;
  String name;
  int bitSize;
  VersatStage stage;
  bool isStatic;

  WireExtra& operator=(Wire& w){
    this->stage = w.stage;
    this->bitSize = w.bitSize;
    this->isStatic = w.isStatic;
    this->name = w.name;
    return *this;
  }
};

struct StaticId{
   FUDeclaration* parent;
   String name;
};

template<> class std::hash<StaticId>{
   public:
   std::size_t operator()(StaticId const& s) const noexcept{
      std::size_t res = std::hash<String>()(s.name) + (std::size_t) s.parent;
      return (std::size_t) res;
   }
};

struct StaticData{
   Array<Wire> configs;
};

struct StaticInfo{
   StaticId id;
   StaticData data;
};

struct CalculatedOffsets{
   Array<int> offsets;
   int max;
};

enum MemType{
   CONFIG,
   STATE,
   DELAY,
   STATIC
};

Accelerator* CreateAccelerator(String name,AcceleratorPurpose purpose);

Pair<Accelerator*,AcceleratorMapping*> CopyAcceleratorWithMapping(Accelerator* accel,AcceleratorPurpose purpose,bool preserveIds,Arena* out);
Accelerator* CopyAccelerator(Accelerator* accel,AcceleratorPurpose purpose,bool preserveIds,InstanceMap* map); // Maps instances from accel to the copy
FUInstance*  CopyInstance(Accelerator* newAccel,FUInstance* oldInstance,bool preserveIds,String newName);

bool NameExists(Accelerator* accel,String name);

int GetFreeShareIndex(Accelerator* accel);
void ShareInstanceConfig(FUInstance* instance, int shareBlockIndex);
void SetStatic(Accelerator* accel,FUInstance* instance);

Array<FUDeclaration*> MemSubTypes(AccelInfo* info,Arena* out);

// TODO: This could work on top of AccelInfo.
Hashmap<StaticId,StaticData>* CollectStaticUnits(AccelInfo* info,Arena* out);

int ExternalMemoryByteSize(ExternalMemoryInterface* inter);
int ExternalMemoryByteSize(Array<ExternalMemoryInterface> interfaces); // Size of a simple memory mapping.

// TODO: Instead of versatComputedValues, could return something like a FUDeclaration
//       (or something that FUDeclaration would be composed off)
//

// This computes the values for the top accelerator only.
// Different of a regular accelerator because it can add more configs for DMA and other top level things
VersatComputedValues ComputeVersatValues(AccelInfo* accel,bool useDMA);

// Returns false if parameter does not exist 
bool SetParameter(FUInstance* inst,String parameterName,String parameterValue);

Array<Edge> GetAllEdges(Accelerator* accel,Arena* out);
EdgeIterator IterateEdges(Accelerator* accel);

// Unit connection
Opt<Edge> FindEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay);
void ConnectUnitsGetEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay = 0);
void ConnectUnitsGetEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay,Edge* previous);
void ConnectUnitsIfNotConnected(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay = 0);
void ConnectUnits(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay = 0);
void ConnectUnits(PortInstance out,PortInstance in,int delay = 0);

void CalculateNodeType(FUInstance* node);
void FixInputs(FUInstance* node);

Array<int> GetNumberOfInputConnections(FUInstance* node,Arena* out);
Array<Array<PortInstance>> GetAllInputs(FUInstance* node,Arena* out);

void RemoveConnection(Accelerator* accel,FUInstance* out,int outPort,FUInstance* in,int inPort);

// Fixes edges such that unit before connected to after, are reconnected to new unit
void InsertUnit(Accelerator* accel,PortInstance before, PortInstance after, PortInstance newUnit);

// If we have A:X -> B:Y and we give this function B:Y, it returns A:X
PortInstance GetAssociatedOutputPortInstance(FUInstance* unit,int portIndex);

bool IsCombinatorial(Accelerator* accel);

void ConnectUnits(PortInstance out,PortInstance in,int delay);

String GlobalStaticWireName(StaticId id,Wire w,Arena* out);

// Accelerator mappings. A simple way of mapping nodes and port edges from one accelerator to another.
void MappingCheck(AcceleratorMapping* map);
void MappingCheck(AcceleratorMapping* map,Accelerator* first,Accelerator* second);
AcceleratorMapping* MappingSimple(Accelerator* first,Accelerator* second,Arena* out);
AcceleratorMapping* MappingSimple(Accelerator* first,Accelerator* second,int size,Arena* out);
AcceleratorMapping* MappingInvert(AcceleratorMapping* toReverse,Arena* out);
AcceleratorMapping* MappingCombine(AcceleratorMapping* first,AcceleratorMapping* second,Arena* out);
PortInstance MappingMapInput(AcceleratorMapping* mapping,PortInstance toMap);
PortInstance MappingMapOutput(AcceleratorMapping* mapping,PortInstance toMap);
void MappingInsertEqualNode(AcceleratorMapping* mapping,FUInstance* first,FUInstance* second);
void MappingInsertInput(AcceleratorMapping* mapping,PortInstance first,PortInstance second);
void MappingInsertOutput(AcceleratorMapping* mapping,PortInstance first,PortInstance second);
void MappingPrintInfo(AcceleratorMapping* map);
void MappingPrintAll(AcceleratorMapping* map);

FUInstance* MappingMapNode(AcceleratorMapping* mapping,FUInstance* inst);

Set<PortInstance>* MappingMapInput(AcceleratorMapping* map,Set<PortInstance>* set,Arena* out);

struct SubMappingInfo{
  FUDeclaration* subDeclaration;
  String higherName;
  bool isInput;
  int subPort;
};

typedef TrieMap<SubMappingInfo,PortInstance> SubMap;

inline bool operator==(const SubMappingInfo& p1,const SubMappingInfo& p2){
  bool res = (p1.subDeclaration == p2.subDeclaration &&
              CompareString(p1.higherName,p2.higherName) &&
              p1.subPort == p2.subPort &&
              p1.isInput == p2.isInput);
  return res;
}

template<> class std::hash<SubMappingInfo>{
public:
   std::size_t operator()(const SubMappingInfo& t) const noexcept{
     std::size_t res = std::hash<FUDeclaration*>()(t.subDeclaration) +
                       std::hash<String>()(t.higherName) +
                       std::hash<bool>()(t.isInput) +
                       std::hash<int>()(t.subPort);

     return res;
   }
};

void PrintSubMappingInfo(SubMap* info);

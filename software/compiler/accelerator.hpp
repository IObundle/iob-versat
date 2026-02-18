#pragma once

#include "utils.hpp"
#include "verilogParsing.hpp"

struct FUInstance;
struct FUDeclaration;
struct Accelerator;
struct Edge;
struct AccelInfo;
struct ConfigFunction;
struct InstanceInfo;
struct SymbolicExpression;

typedef Hashmap<FUInstance*,FUInstance*> InstanceMap;
typedef Hashmap<Edge,Edge> EdgeMap;

struct PortInstance{
  FUInstance* inst;
  int port;
  Direction dir; // NOTE: At the moment a Direction_NONE is most likely an error. TODO: Does it make sense to remove this and just have two different types, an InputPortInstance and an OutputPortInstance?
};

static inline PortInstance MakePortOut(FUInstance* inst,int port){
  PortInstance res = {};
  res.inst = inst;
  res.port = port;
  res.dir = Direction_OUTPUT;
  return res;
}
static inline PortInstance MakePortIn(FUInstance* inst,int port){
  PortInstance res = {};
  res.inst = inst;
  res.port = port;
  res.dir = Direction_INPUT;
  return res;
}

inline bool operator==(const PortInstance& p1,const PortInstance& p2){
  bool res = (p1.inst == p2.inst && p1.port == p2.port && p1.dir == p2.dir);
  return res;
}

inline bool operator!=(const PortInstance& p1,const PortInstance& p2){
  bool res = !(p1 == p2);
  return res;
}

inline u64 Hash(PortInstance s){
  // NOTE: Hashing the pointer, not actually hashing the contents of the FU
  u64 res = Hash(s.inst);
  res *= ((int) s.dir) + 1;
  res += s.port;
    
  return res;
}

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

inline u64 Hash(Edge s){
  u64 res = Hash(s.units[0]);
  res += Hash(s.units[1]);
  res += s.delay;
  return  res;
}

static inline Edge MakeEdge(FUInstance* out,int outPort,FUInstance* in,int inPort,int delay = 0){
  Edge edge = {};
  edge.out.inst = out;
  edge.out.port = outPort;
  edge.out.dir = Direction_OUTPUT;
  edge.in.inst = in;
  edge.in.port = inPort;
  edge.in.dir = Direction_INPUT;
  edge.delay = delay;
  
  return edge;
}

static inline Edge MakeEdge(PortInstance out,PortInstance in,int delay = 0){
  Assert(out.dir == Direction_OUTPUT);
  Assert(in.dir == Direction_INPUT);

  Edge edge = {};
  edge.out = out;
  edge.in = in;
  edge.delay = delay;

  return edge;
}

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
  SymbolicExpression* val;
  //String val; // Mostly a placeholder for now. Eventually want a better way of handling parameters and expression of parameters
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
  bool debug;
  
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

struct StaticId{
   FUDeclaration* parent;
   String name;
};

inline u64 Hash(StaticId id){
  u64 res = Hash(id.name) + Hash(id.parent);
  return res;
}
inline bool operator==(const StaticId& id1,const StaticId& id2){
   bool res = CompareString(id1.name,id2.name) && id1.parent == id2.parent;
   return res;
}

struct StaticData{
  FUDeclaration* decl; // Declaration of unit that contains the origin of the given configs
  Array<Wire> configs; // The actual configs, which might differ from decl->configs because parameters might be instantiated 
};

struct StaticInfo{
   StaticId id;
   StaticData data;
};


struct WireInformation{
  Wire wire;
  int addr;
  bool isStatic;
  SymbolicExpression* startBitExpr;
};

// NOTE: These values are specific to the top accelerator only. They differ because of stuff like DMA, the config interface for the accelerator and so on.
//       Any change that is specific to the top unit should change these values. More config space and things like that can be 'reserved' by changing the config values here and the same holds true for all the other interfaces.
//       Code that does not care about the top level unit should just use the values found in AccelInfo.
struct VersatComputedValues{
  AccelInfo* info;
  
  Array<VersatRegister> registers; // The index of the register is the same index on the accelerator. registers[0] is associated to pos 0 on the accelerator, registers[1] to pos 1 and so on.

  Array<ExternalMemoryInterface> externalMemoryInterfaces;
  Hashmap<StaticId,StaticData>* staticUnits;

  // All configuration (including static and delays) wires
  Array<WireInformation> allWiresInfo;

  // How many configs and state positions are reversed for Versat registers
  // TODO: We probably can remove this and use the size of registers to get this value.
  int versatConfigs;
  int versatStates;

  int nConfigs;
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
  //int externalMemoryInterfaces;
  int totalExternalMemory;  

  SymbolicExpression* configSizeExpr;
  SymbolicExpression* delayStart;
  SymbolicExpression* configurationBitsExpr;
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

  // After port instance change, do not need two maps for input/output differences. Should be able to only use 1.
  TrieMap<FUInstance*,FUInstance*>* instanceMap;
  TrieMap<PortInstance,PortInstance>* inputMap;
  TrieMap<PortInstance,PortInstance>* outputMap;
};

struct EdgeIterator{
  PoolIterator<FUInstance> currentNode;
  PoolIterator<FUInstance> end;
  ConnectionNode* currentPort;
  
  bool IsValid();
  void Next();
  Edge Value();
};

struct CalculatedOffsets{
   Array<int> offsets;
   int max;
};

struct DelayToAdd{
  Edge edge;
  String bufferName;
  String bufferParameters;
  int bufferAmount;
};

struct SubMappingInfo{
  FUDeclaration* subDeclaration;
  String higherName;
  bool isInput;
  int subPort;
};

inline u64 Hash(SubMappingInfo t){
     u64 res = Hash(t.subDeclaration) +
               Hash(t.higherName) +
               Hash(t.isInput) +
               Hash(t.subPort);

     return res;
}
inline bool operator==(const SubMappingInfo& p1,const SubMappingInfo& p2){
  bool res = (p1.subDeclaration == p2.subDeclaration &&
              CompareString(p1.higherName,p2.higherName) &&
              p1.subPort == p2.subPort &&
              p1.isInput == p2.isInput);
  return res;
}

typedef TrieMap<SubMappingInfo,PortInstance> SubMap;

// ======================================
// Accelerator creation

Accelerator* CreateAccelerator(String name,AcceleratorPurpose purpose);
Accelerator* CopyAccelerator(Accelerator* accel,AcceleratorPurpose purpose,bool preserveIds);
Pair<Accelerator*,AcceleratorMapping*> CopyAcceleratorWithMapping(Accelerator* accel,AcceleratorPurpose purpose,bool preserveIds,Arena* out);

// ======================================
// Unit creation and operations

FUInstance* CreateFUInstance(Accelerator* accel,FUDeclaration* type,String entityName);
FUInstance* CopyInstance(Accelerator* newAccel,FUInstance* oldInstance,bool preserveIds,String newName);
FUInstance* GetUnit(Accelerator* accel,String name);

bool IsUnitCombinatorial(FUInstance* inst);

Array<int> GetNumberOfInputConnections(FUInstance* node,Arena* out);
Array<Array<PortInstance>> GetAllInputs(FUInstance* node,Arena* out);

// If we have A:X -> B:Y and we give this function B:Y, it returns A:X
PortInstance GetAssociatedOutputPortInstance(FUInstance* unit,int portIndex);

// Fixes edges such that unit before connected to after, are reconnected to new unit
void InsertUnit(Accelerator* accel,PortInstance before, PortInstance after, PortInstance newUnitOutput,PortInstance newUnitInput,int delay = 0);
void RemoveFUInstance(Accelerator* accel,FUInstance* toRemove);

// ======================================
// Edges

void ConnectUnitsGetEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay = 0);
void ConnectUnitsGetEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay,Edge* previous);
void ConnectUnitsIfNotConnected(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay = 0);
void ConnectUnits(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay = 0);
void ConnectUnits(PortInstance out,PortInstance in,int delay = 0);
void ConnectUnits(PortInstance out,PortInstance in,int delay);

void RemoveConnection(Accelerator* accel,FUInstance* out,int outPort,FUInstance* in,int inPort);

Opt<Edge> FindEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay);
Opt<Edge> FindEdge(PortInstance out,PortInstance in,int delay);

Array<Edge> GetAllEdges(Accelerator* accel,Arena* out);
EdgeIterator IterateEdges(Accelerator* accel);

// ======================================
// Config sharing

int GetFreeShareIndex(Accelerator* accel);
void ShareInstanceConfig(FUInstance* instance, int shareBlockIndex);
void SetStatic(FUInstance* instance);

// ======================================
// Graph inputs and outputs
FUInstance* GetInputInstance(Pool<FUInstance>* nodes,int inputIndex);
FUInstance* GetOutputInstance(Pool<FUInstance>* nodes);

// ======================================
// Params

bool SetParameter(FUInstance* inst,String parameterName,SymbolicExpression* val); // False if param does not exist
SymbolicExpression* GetParameterValue(FUInstance* inst,String name);

//
// Graph fixes and operations
void FixDelays(Accelerator* accel,Hashmap<Edge,DelayInfo>* edgeDelays);
Pair<Accelerator*,SubMap*> Flatten(Accelerator* accel,int times);
DAGOrderNodes CalculateDAGOrder(Accelerator* accel,Arena* out);

bool IsCombinatorial(Accelerator* accel);
bool NameExists(Accelerator* accel,String name);
String GenerateNewValidName(Accelerator* accel,String base,Arena* out);

Array<FUDeclaration*> MemSubTypes(AccelInfo* info,Arena* out);

Hashmap<StaticId,StaticData>* CollectStaticUnits(AccelInfo* info,Arena* out);

// TODO: We kinda want to "remove" this since memories should be able to depend on parameters, but we currently calculate and instantiate memories because we cannot export memory info.
int ExternalMemoryByteSize(ExternalMemoryInterface* inter);
int ExternalMemoryByteSize(Array<ExternalMemoryInterface> interfaces); // Size of a simple memory mapping.

// This computes the values for the top accelerator only.
// Different of a regular accelerator because it can add more configs for DMA and other top level things
VersatComputedValues ComputeVersatValues(Accelerator* graph,AccelInfo* accel,Arena* out);

//
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

void PrintSubMappingInfo(SubMap* info);

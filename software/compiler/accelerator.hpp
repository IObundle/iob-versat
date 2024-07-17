#pragma once

#include <unordered_map>

#include "utils.hpp"
#include "memory.hpp"
//#include "configurations.hpp"
#include "verilogParsing.hpp"

struct FUInstance;
struct FUDeclaration;
struct Accelerator;
struct Edge;

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

typedef Array<Edge> Path;
typedef Hashmap<Edge,Path> PathMap;

struct GenericGraphMapping{
  InstanceMap* nodeMap;
  PathMap* edgeMap;
};

struct EdgeDelayInfo{
  int* value;
  bool isAny;
};

struct ConnectionNode{
  PortInstance instConnectedTo;
  int port;
  int edgeDelay;
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

struct FUInstance{
  String name;

  // This should be versat side only, but it's easier to have them here for now
  String parameters; // TODO: Actual parameter structure
  Accelerator* accel;
  FUDeclaration* declaration;
  int id;

  union{
    int literal;
    int bufferAmount;
    int portIndex;
  };
  int sharedIndex;
  bool isStatic;
  bool sharedEnable;
  bool isMergeMultiplexer; // TODO: Kinda of an hack for now
  int mergeMultiplexerId;
  
  FUInstance* next;

  // Calculated and updated every time a connection is added or removed
  ConnectionNode* allInputs;
  ConnectionNode* allOutputs;
  Array<PortInstance> inputs;
  Array<bool> outputs;

  //int outputs;
  bool multipleSamePortInputs;
  NodeType type;
};

enum AcceleratorPurpose{
  AcceleratorPurpose_TEMP,
  AcceleratorPurpose_BASE,
  AcceleratorPurpose_FLATTEN,
  AcceleratorPurpose_MODULE,
  AcceleratorPurpose_RECON,
  AcceleratorPurpose_MERGE
};

// TODO: Memory leaks can be fixed by having a global InstanceNode and FUInstance Pool and a global dynamic arena for everything else.
struct Accelerator{ // Graph + data storage
  FUInstance* allocated;
  FUInstance* lastAllocated;
  
  DynamicArena* accelMemory; // TODO: We could remove all this because we can now build all the accelerators in place. (Add an API that functions like a Accelerator builder and at the end we lock everything into an immutable graph).

   // Mainly for debugging
  String name; // For debugging purposes it's useful to give accelerators a name
  int id;
  AcceleratorPurpose purpose;
};

struct MemoryAddressMask{
  // Repr: memoryMask ( memoryMaskSize )

  int memoryMaskSize;
  char memoryMaskBuffer[33];
  char* memoryMask;
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

  int nUnitsIO;
  int numberConnections; // TODO: Same as accel
  int externalMemoryInterfaces;

  int stateConfigurationAddressBits;
  int memoryAddressBits;
  int memoryMappingAddressBits;
  int memoryConfigDecisionBit;

  bool signalLoop;
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

  TrieMap<FUInstance*,FUInstance*>* map;
};

struct EdgeIterator{
  FUInstance* currentNode;
  ConnectionNode* currentPort;
  
  bool HasNext();
  Edge Next();
};

// Strange forward declare
template<typename T> struct WireTemplate;
typedef WireTemplate<int> Wire;

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

Accelerator* GetAcceleratorById(int id);

Accelerator* CreateAccelerator(String name,AcceleratorPurpose purpose);

Pair<Accelerator*,AcceleratorMapping*> CopyAcceleratorWithMapping(Accelerator* accel,AcceleratorPurpose purpose,bool preserveIds,Arena* out);
Accelerator* CopyAccelerator(Accelerator* accel,AcceleratorPurpose purpose,bool preserveIds,InstanceMap* map); // Maps instances from accel to the copy
FUInstance*  CopyInstance(Accelerator* newAccel,FUInstance* oldInstance,bool preserveIds,String newName);

bool NameExists(Accelerator* accel,String name);

int GetFreeShareIndex(Accelerator* accel); // TODO: Slow 

Array<FUDeclaration*> AllNonSpecialSubTypes(Accelerator* accel,Arena* out,Arena* temp);
Array<FUDeclaration*> ConfigSubTypes(Accelerator* accel,Arena* out,Arena* temp);
Array<FUDeclaration*> MemSubTypes(Accelerator* accel,Arena* out,Arena* temp);

Hashmap<StaticId,StaticData>* CollectStaticUnits(Accelerator* accel,FUDeclaration* topDecl,Arena* out);

int ExternalMemoryByteSize(ExternalMemoryInterface* inter);
int ExternalMemoryByteSize(Array<ExternalMemoryInterface> interfaces); // Size of a simple memory mapping.

// TODO: Instead of versatComputedValues, could return something like a FUDeclaration
//       (or something that FUDeclaration would be composed off)
//       
VersatComputedValues ComputeVersatValues(Accelerator* accel,bool useDMA);

Array<Edge> GetAllEdges(Accelerator* accel,Arena* out);
EdgeIterator IterateEdges(Accelerator* accel);

// Unit connection
Opt<Edge> FindEdge(PortInstance out,PortInstance in);
Opt<Edge> FindEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay);
void ConnectUnitsGetEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay = 0);
void ConnectUnitsGetEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay,Edge* previous);
void ConnectUnitsIfNotConnected(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay = 0);
void  ConnectUnits(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay = 0);
void ConnectUnits(PortInstance out,PortInstance in,int delay = 0);

//FUInstance* GetInstanceNode(Accelerator* accel,FUInstance* inst); TODO: Remove this
void CalculateNodeType(FUInstance* node);
void FixInputs(FUInstance* node);

Array<int> GetNumberOfInputConnections(FUInstance* node,Arena* out);
Array<Array<PortInstance>> GetAllInputs(FUInstance* node,Arena* out);

void RemoveConnection(Accelerator* accel,FUInstance* out,int outPort,FUInstance* in,int inPort);
FUInstance* RemoveUnit(FUInstance* nodes,FUInstance* unit);

// Fixes edges such that unit before connected to after, are reconnected to new unit
void InsertUnit(Accelerator* accel,PortInstance before, PortInstance after, PortInstance newUnit);

bool IsCombinatorial(Accelerator* accel);

void ConnectUnits(PortInstance out,PortInstance in,int delay);

void MappingCheck(AcceleratorMapping* map);
void MappingCheck(AcceleratorMapping* map,Accelerator* first,Accelerator* second);
AcceleratorMapping* MappingSimple(Accelerator* first,Accelerator* second,Arena* out);
AcceleratorMapping* MappingSimple(Accelerator* first,Accelerator* second,int size,Arena* out);
AcceleratorMapping* MappingInvert(AcceleratorMapping* toReverse,Arena* out);
AcceleratorMapping* MappingCombine(AcceleratorMapping* first,AcceleratorMapping* second,Arena* out);
FUInstance* MappingMap(AcceleratorMapping* mapping,FUInstance* toMap);
void MappingInsert(AcceleratorMapping* mapping,FUInstance* first,FUInstance* second);
void MappingPrintInfo(AcceleratorMapping* map);
void MappingPrintAll(AcceleratorMapping* map);

Set<PortInstance>* MappingMap(AcceleratorMapping* map,Set<PortInstance>* set,Arena* out);

#pragma once

#include <unordered_map>

#include "utils.hpp"
#include "memory.hpp"
#include "configurations.hpp"
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

struct ConnectionNode{
  PortInstance instConnectedTo;
  int port;
  int edgeDelay;
  int* delay; // Maybe not the best approach to calculate delay. TODO: check later
  ConnectionNode* next;
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

// TODO: Memory leaks can be fixed by having a global InstanceNode and FUInstance Pool and a global dynamic arena for everything else.
struct Accelerator{ // Graph + data storage
  FUInstance* allocated;
  FUInstance* lastAllocated;
  Pool<FUInstance> instances; // TODO: Does anyone care about this or can we just use the allocated list?
  
  DynamicArena* accelMemory; // TODO: We could remove all this because we can now build all the accelerators in place. (Add an API that functions like a Accelerator builder and at the end we lock everything into an immutable graph).

  String name; // For debugging purposes it's useful to give accelerators a name
  int id;
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

struct EdgeIterator{
  FUInstance* currentNode;
  ConnectionNode* currentPort;
  
  bool HasNext();
  Edge Next();
};

Accelerator* CopyAccelerator(Accelerator* accel,InstanceMap* map); // Maps instances from accel to the copy
FUInstance*  CopyInstance(Accelerator* newAccel,FUInstance* oldInstance,String newName);

bool NameExists(Accelerator* accel,String name);

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
//Opt<Edge> FindEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex);
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


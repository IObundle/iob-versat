#pragma once

#include <unordered_map>

#include "utils.hpp"
#include "memory.hpp"
#include "declaration.hpp"

struct FUInstance;
struct FUDeclaration;
struct Edge;
struct PortEdge;

typedef Hashmap<FUInstance*,FUInstance*> InstanceMap;
typedef Hashmap<PortEdge,PortEdge> PortEdgeMap;
typedef Hashmap<Edge*,Edge*> EdgeMap;
typedef Hashmap<InstanceNode*,InstanceNode*> InstanceNodeMap;

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

struct PortEdge{
   PortInstance units[2];
};

inline bool operator==(const PortEdge& e1,const PortEdge& e2){
   bool res = (e1.units[0] == e2.units[0] && e1.units[1] == e2.units[1]);
   return res;
}

inline bool operator!=(const PortEdge& e1,const PortEdge& e2){
   bool res = !(e1 == e2);
   return res;
}

template<> class std::hash<PortEdge>{
   public:
   std::size_t operator()(PortEdge const& s) const noexcept{
      std::size_t res = std::hash<PortInstance>()(s.units[0]);
      res += std::hash<PortInstance>()(s.units[1]);
      return res;
   }
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

struct FUInstance;
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
   FUInstance* inst;
   InstanceNode* next;

   // Calculated and updated every time a connection is added or removed
   ConnectionNode* allInputs;
   ConnectionNode* allOutputs;
   Array<PortNode> inputs;
   Array<bool> outputs;
   //int outputs;
   bool multipleSamePortInputs;
   NodeType type;
};

struct Accelerator{ // Graph + data storage
  Edge* edges; // TODO: Should be removed, edge info is all contained inside the instance nodes and desync leads to bugs since some code still uses this.

  InstanceNode* allocated;
  InstanceNode* lastAllocated;
  Pool<FUInstance> instances;

  DynamicArena* accelMemory; // TODO: We could remove all this because we can now build all the accelerators in place. (Add an API that functions like a Accelerator builder and at the end we lock everything into an immutable graph).

  std::unordered_map<StaticId,StaticData> staticUnits;

  String name; // For debugging purposes it's useful to give accelerators a name
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

enum OrderType{
  OrderType_SINK,
};

struct DAGOrderNodes{
  Array<InstanceNode*> sinks;
  Array<InstanceNode*> sources;
  Array<InstanceNode*> computeUnits;
  Array<InstanceNode*> instances;
  Array<int>           order;
  int size;
  int maxOrder;
};

struct CalculateDelayResult{
  Hashmap<EdgeNode,int>* edgesDelay;
  Hashmap<InstanceNode*,int>* nodeDelay;
};

// TODO: The concept of flat instance no longer exists. Remove them and check if any code dependend on the fact that copy flat did not copy static or shared 
Accelerator* CopyAccelerator(Accelerator* accel,InstanceMap* map,Arena* out);
Accelerator* CopyFlatAccelerator(Accelerator* accel,InstanceMap* map);
FUInstance* CopyInstance(Accelerator* newAccel,FUInstance* oldInstance,String newName,InstanceNode* previous);
FUInstance* CopyInstance(Accelerator* newAccel,FUInstance* oldInstance,String newName);
InstanceNode* CopyInstance(Accelerator* newAccel,InstanceNode* oldInstance,String newName);
InstanceNode* CreateFlatFUInstance(Accelerator* accel,FUDeclaration* type,String name);

Array<FUDeclaration*> AllNonSpecialSubTypes(Accelerator* accel,Arena* out,Arena* temp);
Array<FUDeclaration*> ConfigSubTypes(Accelerator* accel,Arena* out,Arena* temp);
Array<FUDeclaration*> MemSubTypes(Accelerator* accel,Arena* out,Arena* temp);

int ExternalMemoryByteSize(ExternalMemoryInterface* inter);
int ExternalMemoryByteSize(Array<ExternalMemoryInterface> interfaces); // Size of a simple memory mapping.

// TODO: Instead of versatComputedValues, could return something like a FUDeclaration
//       (or something that FUDeclaration would be composed off)
//       
VersatComputedValues ComputeVersatValues(Accelerator* accel,bool useDMA);

CalculateDelayResult CalculateDelay(Accelerator* accel,DAGOrderNodes order,Arena* out);

// Unit connection
Edge* FindEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay = 0);
Edge* ConnectUnitsGetEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay = 0);
Edge* ConnectUnitsGetEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay,Edge* previous);
Edge* ConnectUnitsIfNotConnectedGetEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay = 0);
void  ConnectUnits(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay = 0);
Edge* ConnectUnits(PortInstance out,PortInstance in,int delay = 0);

InstanceNode* GetInstanceNode(Accelerator* accel,FUInstance* inst);
void CalculateNodeType(InstanceNode* node);
void FixInputs(InstanceNode* node);

Array<int> GetNumberOfInputConnections(InstanceNode* node,Arena* out);
Array<Array<PortNode>> GetAllInputs(InstanceNode* node,Arena* out);

void RemoveConnection(Accelerator* accel,InstanceNode* out,int outPort,InstanceNode* in,int inPort);
InstanceNode* RemoveUnit(InstanceNode* nodes,InstanceNode* unit);

// Fixes edges such that unit before connected to after, are reconnected to new unit
void InsertUnit(Accelerator* accel,PortNode output,PortNode input,PortNode newUnit);
void InsertUnit(Accelerator* accel, PortInstance before, PortInstance after, PortInstance newUnit);

bool IsCombinatorial(Accelerator* accel);

Edge* ConnectUnitsGetEdge(PortNode out,PortNode in,int delay);

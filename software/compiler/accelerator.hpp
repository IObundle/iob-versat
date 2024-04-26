#pragma once

#include <unordered_map>

#include "utils.hpp"
#include "memory.hpp"
#include "declaration.hpp"

struct FUInstance;
struct FUDeclaration;
struct Edge;
struct PortEdge;
struct Versat;

typedef std::unordered_map<FUInstance*,FUInstance*> InstanceMap;
typedef std::unordered_map<PortEdge,PortEdge> PortEdgeMap;
typedef std::unordered_map<Edge*,Edge*> EdgeMap;
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
   enum {TAG_UNCONNECTED,TAG_COMPUTE,TAG_SOURCE,TAG_SINK,TAG_SOURCE_AND_SINK} type;

   // The following is only calculated by specific functions. Otherwise assume data is old if any change to the graph occurs
   // In fact, maybe this should be removed and should just be a hashmap + array to save computed data.
   // But need to care because top level accelerator still needs to be able to calculate this values and persist them somehow
   int inputDelay;
   //int order;
};

struct OrderedInstance{ // This is a more powerful and generic approach to subgraphing. Should not be named ordered, could be used for anything really, not just order iteration
   InstanceNode* node;
   OrderedInstance* next;
};

struct Accelerator{ // Graph + data storage
  Versat* versat;

  Edge* edges; // TODO: Should be removed, edge info is all contained inside the instance nodes and desync leads to bugs since some code still uses this.

  OrderedInstance* ordered;
  InstanceNode* allocated;
  InstanceNode* lastAllocated;
  Pool<FUInstance> instances;

  DynamicArena* accelMemory; // TODO: We could remove all this because we can now build all the accelerators in place. (Add an API that functions like a Accelerator builder and at the end we lock everything into an immutable graph).

  std::unordered_map<StaticId,StaticData> staticUnits;

  String name; // For debugging purposes it's useful to give accelerators a name
};

struct VersatComputedData{
  int memoryMaskSize;
  char memoryMaskBuffer[33];
  char* memoryMask;
};

struct ComputedData{
  Array<ExternalMemoryInterface> external; // Just a grouping of all external interfaces.
  Array<VersatComputedData> data;          
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
  int numberConnections;
  int externalMemoryInterfaces;

  int stateConfigurationAddressBits;
  int memoryAddressBits;
  int memoryMappingAddressBits;
  int memoryConfigDecisionBit;

  bool signalLoop;
};

// Accelerator

// TODO: The concept of flat instance no longer exists. Remove them and check if any code dependend on the fact that copy flat did not copy static or shared 
Accelerator* CopyAccelerator(Versat* versat,Accelerator* accel,InstanceMap* map);
Accelerator* CopyFlatAccelerator(Versat* versat,Accelerator* accel,InstanceMap* map);
FUInstance* CopyInstance(Accelerator* newAccel,FUInstance* oldInstance,String newName,InstanceNode* previous);
FUInstance* CopyInstance(Accelerator* newAccel,FUInstance* oldInstance,String newName);
InstanceNode* CopyInstance(Accelerator* newAccel,InstanceNode* oldInstance,String newName);
InstanceNode* CreateFlatFUInstance(Accelerator* accel,FUDeclaration* type,String name);

ComputedData CalculateVersatComputedData(Array<InstanceInfo> info,VersatComputedValues val,Arena* out);

Array<FUDeclaration*> AllNonSpecialSubTypes(Accelerator* accel,Arena* out,Arena* temp);
Array<FUDeclaration*> ConfigSubTypes(Accelerator* accel,Arena* out,Arena* temp);
Array<FUDeclaration*> MemSubTypes(Accelerator* accel,Arena* out,Arena* temp);

void ReorganizeAccelerator(Accelerator* graph,Arena* temp);
void ReorganizeIterative(Accelerator* accel,Arena* temp);

int ExternalMemoryByteSize(ExternalMemoryInterface* inter);
int ExternalMemoryByteSize(Array<ExternalMemoryInterface> interfaces); // Size of a simple memory mapping.

// TODO: Instead of versatComputedValues, could return something like a FUDeclaration
//       (or something that FUDeclaration would be composed off)
//       
VersatComputedValues ComputeVersatValues(Versat* versat,Accelerator* accel,bool useDMA);

// Unit connection
Edge* FindEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay = 0);
Edge* ConnectUnitsGetEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay = 0);
Edge* ConnectUnitsGetEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay,Edge* previous);
Edge* ConnectUnitsIfNotConnectedGetEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay = 0);
void  ConnectUnits(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay = 0);
Edge* ConnectUnits(PortInstance out,PortInstance in,int delay = 0);
Edge* ConnectUnits(InstanceNode* out,int outPort,InstanceNode* in,int inPort,int delay = 0);

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

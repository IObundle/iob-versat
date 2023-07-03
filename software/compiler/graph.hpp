#ifndef INCLUDED_GRAPH
#define INCLUDED_GRAPH

//#include "versat.hpp"
#include "utils.hpp"

struct Accelerator;
struct Arena;
struct FUInstance;
struct FUInstance;

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

struct ConnectionInfo{
   PortInstance instConnectedTo;
   int port;
   int edgeDelay; // Stores the edge delay information
   int* delay; // Used to calculate delay. Does not store edge delay information
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

// Unit connection
Edge* FindEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay = 0);
Edge* ConnectUnitsGetEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay = 0);
Edge* ConnectUnitsGetEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay,Edge* previous);
Edge* ConnectUnitsIfNotConnectedGetEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay = 0);
Edge* ConnectUnits(PortInstance out,PortInstance in,int delay = 0);
Edge* ConnectUnits(InstanceNode* out,int outPort,InstanceNode* in,int inPort,int delay = 0);
Edge* ConnectUnits(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay,Edge* previousEdge);

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

Edge* ConnectUnitsGetEdge(PortNode out,PortNode in,int delay);

#endif // INCLUDED_GRAPH

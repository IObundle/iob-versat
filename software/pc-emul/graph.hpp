#ifndef INCLUDED_GRAPH
#define INCLUDED_GRAPH

#include "versatPrivate.hpp"

struct Arena;

#if 0
struct Node{
   Node* next;
   String name;
   ComplexFUInstance* inst;
};

struct PortNode{
   Node* node;
   int port;
};

struct EdgeNode{
   EdgeNode* next;

   union {
      struct {
      PortNode out;
      PortNode in;
      };
      PortNode units[2];
   };
};

struct Graph{
   Node* nodes;
   EdgeNode* edges;
   Accelerator* accel;
};
#endif

typedef ComplexFUInstance Node;
typedef PortInstance PortNode;
typedef Edge EdgeNode;
typedef Accelerator Graph;

struct FlattenResult{
   Node* flatUnitStart;
   Node* flatUnitEnd; // One node after the flattening
   EdgeNode* flatEdgeStart;
   EdgeNode* flatEdgeEnd;
   int flattenedUnits;
   int flattenedEdges;
   int circuitEdgesStart;
   int circuitEdgesEnd;
};

struct FlatteningTemp{
   int index;
   FUDeclaration* parent;
   FUDeclaration* decl;
   String name;
   int flattenedUnits;
   int subunitsCount;
   int edgeCount;
   int edgeStart;
   int level;
   int flag;
   FlatteningTemp* next;
   FlatteningTemp* child;
};

template<typename T>
struct Sublist{
   T* start;
   T* end;
};

template<typename T>
int Size(Sublist<T> sub){
   int count = 0;
   FOREACH_LIST_BOUNDED(ptr,sub.start,sub.end){
      count += 1;
   }
   return count;
}

template<typename T>
Sublist<T> SimpleSublist(T* start,int size){
   Sublist<T> res = {};
   res.start = start;
   res.end = ListGet(start,size);
   return res;
}

struct Subgraph{
   Sublist<Node> nodes;
   Sublist<EdgeNode> edges;
};

template<typename T>
void AssertNoLoop(T* head,Arena* arena){
   BLOCK_REGION(arena);

   Hashmap<T*,int> mapping = {};
   mapping.Init(arena,10000);

   FOREACH_LIST(ptr,head){
      GetOrAllocateResult<int> res = mapping.GetOrAllocate(ptr);

      Assert(!res.result);
      *res.data = 0;
   }
}

Node* GetOutputInstance(Subgraph sub);

Graph* PushGraph(Arena* arena);

// Adds at the beginning.
Node*     AddNode(Graph* graph,ComplexFUInstance* inst,Node* previous = nullptr);
EdgeNode* AddEdge(Graph* graph,Node* out,int outPort,Node* in,int inPort,EdgeNode* previous = nullptr);

void RemoveNodeAndEdges(Graph* graph,Node* node);

FlattenResult FlattenNode(Graph* graph,Node* node,Arena* arena);

String OutputDotGraph(Graph* graph,Arena* arena);
String OutputDotGraph(Subgraph graph,Arena* output);

ConsolidationResult GenerateConsolidationGraph(Versat* versat,Arena* arena,Subgraph accel1,Subgraph accel2,ConsolidationGraphOptions options);

#endif // INCLUDED_GRAPH

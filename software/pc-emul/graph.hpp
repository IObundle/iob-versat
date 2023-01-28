#ifndef INCLUDED_GRAPH
#define INCLUDED_GRAPH

#include "versatPrivate.hpp"

struct Arena;

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
   PortNode out;
   PortNode in;
};

struct Graph{
   Node* nodes;
   EdgeNode* edges;
   Accelerator* accel;
};

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
   FUDeclaration* decl;
   String name;
   int flattenedUnits;
   int subunitsCount;
   int edgeCount;
};

#define FOREACH_LIST(ITER,START) for(auto* ITER = START; ITER; ITER = ITER->next)

Graph* PushGraph(Arena* arena);

// Adds at the beginning.
Node*     AddNode(Graph* graph,ComplexFUInstance* inst,Node* previous = nullptr);
EdgeNode* AddEdge(Graph* graph,Node* out,int outPort,Node* in,int inPort,EdgeNode* previous = nullptr);

void RemoveNodeAndEdges(Graph* graph,Node* node);

void ConvertGraph(Graph* graph,Accelerator* accel,Arena* arena);

FlattenResult FlattenNode(Graph* graph,Node* node,Arena* arena);

String OutputDotGraph(Graph* graph,Arena* arena);

#endif // INCLUDED_GRAPH

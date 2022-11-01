#ifndef INCLUDED_MERGE_GRAPH
#define INCLUDED_MERGE_GRAPH

#include "versatPrivate.hpp"

struct MergeEdge{
   FUInstance* instances[2];
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
   MappingNode nodes[2];
};

struct ConsolidationGraph{
   MappingNode* nodes;
   int numberNodes;
   MappingEdge* edges;
   int numberEdges;

   // Used in get clique;
   int* validNodes;
};

struct Mapping{
   FUInstance* source;
   FUInstance* sink;
};

SizedString MappingNodeIdentifier(MappingNode* node,Arena* memory);

ConsolidationGraph GenerateConsolidationGraph(Accelerator* accel1,Accelerator* accel2);
ConsolidationGraph MaxClique(ConsolidationGraph graph);

#endif // INCLUDED_MERGE_GRAPH

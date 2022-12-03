#ifndef INCLUDED_MERGE_GRAPH
#define INCLUDED_MERGE_GRAPH

#include "versatPrivate.hpp"

struct MergeEdge{
   ComplexFUInstance* instances[2];
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
   MappingNode* nodes[2];
};

typedef Graph<MappingNode,MappingEdge> ConsolidationGraph;

struct Mapping{
   FUInstance* source;
   FUInstance* sink;
};

struct ConsolidationGraphOptions{
   int order;
   int difference;
   bool mapNodes;
   enum {NOTHING,SAME_ORDER,EXACT_ORDER} type;
};

bool MappingConflict(MappingNode map1,MappingNode map2);
ConsolidationGraph MaxClique(ConsolidationGraph graph,Arena* arena);

ConsolidationGraph GenerateConsolidationGraph(Versat* versat,Arena* arena,Accelerator* accel1,Accelerator* accel2,ConsolidationGraphOptions options,MergingStrategy strategy);

Accelerator* MergeAccelerator(Versat* versat,Accelerator* accel1,Accelerator* accel2,MergingStrategy strategy);

SizedString MappingNodeIdentifier(MappingNode* node,Arena* memory);

#endif // INCLUDED_MERGE_GRAPH

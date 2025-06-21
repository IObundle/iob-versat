#pragma once

#include "versat.hpp"

struct FUInstance;
struct Accelerator;
struct AcceleratorMapping;
struct FUDeclaration;

struct SpecificMerge{
  String instA;
  String instB;
};

struct IndexRecord{
  int index;
  IndexRecord* next;
};

struct SpecificMergeNodes{
  FUInstance* instA;
  FUInstance* instB;
};

struct SpecificMergeNode{
  int firstIndex;
  String firstName;
  int secondIndex;
  String secondName;
};

struct MergeEdge{
  FUInstance* instances[2];
};

struct MappingNode{ // Mapping (edge to edge or node to node)
  union{
    MergeEdge nodes;
    Edge edges[2];
  };

  enum {NODE,EDGE} type;
};


struct GraphAndMapping{
  FUDeclaration* decl;
  AcceleratorMapping* map;
  Set<PortInstance>* mergeMultiplexers;
  bool fromStruct;
};

template<> class std::hash<GraphAndMapping>{
public:
   std::size_t operator()(GraphAndMapping const& s) const noexcept{
     std::size_t res = (std::size_t) s.decl;
     return res;
   }
};

static bool operator==(const GraphAndMapping& g0,const GraphAndMapping& g1){
  return (g0.decl == g1.decl);
}

inline bool operator==(const MappingNode& node0,const MappingNode& node1){
  if(node0.type != node1.type){
    return false;
  }

  switch(node0.type){
  case MappingNode::NODE:{
    bool res = node0.nodes.instances[0] == node1.nodes.instances[0] &&
               node0.nodes.instances[1] == node1.nodes.instances[1];
    return res;
  } break;
  case MappingNode::EDGE:{
    bool res = (node0.edges[0] == node1.edges[0] &&
                node0.edges[1] == node1.edges[1]);

    return res;
  } break;
  }
}

struct ConsolidationGraphOptions{
  Array<SpecificMergeNodes> specifics;
  int order;
  int difference;
  bool mapNodes;
  enum {NOTHING,SAME_ORDER,EXACT_ORDER} type;
};

struct ConsolidationGraph{
  Array<MappingNode> nodes;
  Array<BitArray> edges;

  BitArray validNodes;
};

struct ConsolidationResult{
  ConsolidationGraph graph;
  Pool<MappingNode> specificsAdded;
  int upperBound;
};

struct CliqueState{
  int max;
  int upperBound;
  int startI;
  int iterations;
  Array<int> table;
  ConsolidationGraph clique;
  Time start;
  bool found;
};

struct OverheadCount{
  int muxes;
  int buffers;
};

struct ReconstituteResult{
  Accelerator* accel;
  AcceleratorMapping* accelToRecon;
};

struct IsCliqueResult{
  bool result;
  int failedIndex;
};

// TODO: For now everything is hashmap to help in debugging to be able to reuse previous code
typedef Hashmap<FUInstance*,FUInstance*> InstanceMap;
typedef Hashmap<Edge,Edge> EdgeMap;

struct MergeGraphResult{
  Accelerator* accel1; // Should pull out the graph stuff instead of using an Accelerator for this
  Accelerator* accel2;

  InstanceMap* map1; // Maps node from accel1 to newGraph
  InstanceMap* map2; // Maps node from accel2 to newGraph
  Accelerator* newGraph;
};

struct MergeGraphResultExisting{
  Accelerator* result;
  Accelerator* accel2;

  AcceleratorMapping* map2;
};

struct GraphMapping{
  InstanceMap* instanceMap;
  InstanceMap* reverseInstanceMap;
  EdgeMap* edgeMap;
};

enum MergingStrategy{
  SIMPLE_COMBINATION,
  CONSOLIDATION_GRAPH,
  FIRST_FIT
};

void OutputConsolidationGraph(ConsolidationGraph graph,bool onlyOutputValid,String moduleName,String fileName);

ConsolidationResult GenerateConsolidationGraph(Accelerator* accel1,Accelerator* accel2,ConsolidationGraphOptions options,Arena* out);
MergeGraphResult MergeGraph(Accelerator* flatten1,Accelerator* flatten2,GraphMapping& graphMapping,String name);
void AddCliqueToMapping(GraphMapping& res,ConsolidationGraph clique);

void InsertMapping(GraphMapping& map,Edge& edge0,Edge& edge1);

bool NodeMappingConflict(Edge edge1,Edge edge2);
bool MappingConflict(MappingNode map1,MappingNode map2);
ConsolidationGraph Copy(ConsolidationGraph graph,Arena* out);

bool MappingConflict(MappingNode map1,MappingNode map2);
CliqueState MaxClique(ConsolidationGraph graph,int upperBound,Arena* out,float MAX_CLIQUE_TIME);
ConsolidationGraph GenerateConsolidationGraph(Arena* out,Accelerator* accel1,Accelerator* accel2,ConsolidationGraphOptions options,MergingStrategy strategy);

MergeGraphResult HierarchicalHeuristic(FUDeclaration* decl1,FUDeclaration* decl2,String name);

BitArray* CalculateNeighborsTable(ConsolidationGraph graph,Arena* out);

IsCliqueResult IsClique(ConsolidationGraph graph);

ConsolidationGraph ParallelMaxClique(ConsolidationGraph graph,int upperBound,Arena* out,float MAX_CLIQUE_TIME);

String MappingNodeIdentifier(MappingNode* node,Arena* memory);
MergeGraphResult HierarchicalMergeAccelerators(Accelerator* accel1,Accelerator* accel2,String name);
MergeGraphResult HierarchicalMergeAcceleratorsFullClique(Accelerator* accel1,Accelerator* accel2,String name);

FUDeclaration* MergeAccelerators(FUDeclaration* accel1,FUDeclaration* accel2,String name,int flatteningOrder = 99,MergingStrategy strategy = MergingStrategy::CONSOLIDATION_GRAPH,SpecificMerge* specifics = nullptr,int nSpecifics = 0);

FUDeclaration* Merge(Array<FUDeclaration*> types,
                     String name,Array<SpecificMergeNode> specifics,
                     MergeModifier = MergeModifier_NO_UNIT_MERGED,MergingStrategy strat = MergingStrategy::CONSOLIDATION_GRAPH);


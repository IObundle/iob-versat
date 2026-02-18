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

inline u64 Hash(GraphAndMapping s){
  return Hash(s.decl);
}

static bool operator==(const GraphAndMapping& g0,const GraphAndMapping& g1){
  return (g0.decl == g1.decl);
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
  AcceleratorMapping* instMap;

  InstanceMap* instanceMap;
  InstanceMap* reverseInstanceMap;
  EdgeMap* edgeMap;
};

enum MergingStrategy{
  SIMPLE_COMBINATION,
  CONSOLIDATION_GRAPH,
  FIRST_FIT
};

struct MergeAndRecons{
  Arena arenaInst;
  Arena* arena;
  
  Accelerator* mergedGraph;
  Array<Accelerator*> recons;
  Array<AcceleratorMapping*> reconToMerged;
  Array<AcceleratorMapping*> inputToRecon;
};

// ============================================================================
// Merge and recons simultaneous handling

MergeAndRecons* StartMerge(int amountOfRecons,Array<Accelerator*> inputs);
void DebugOutputGraphs(MergeAndRecons* recons,String stageName);

void AddEdgeSingle(MergeAndRecons* recons,Edge inputEdge,int reconBase);
void AddEdgeMappingSingle(MergeAndRecons* recons,Edge inputEdge,GraphMapping* map,int reconBase);

Opt<Edge> MapMergedEdgeToRecon(MergeAndRecons* recons,Edge mergedEdge,int reconIndex);
void InsertNewUnit(MergeAndRecons* recons,String name,FUDeclaration* decl,PortInstance before,PortInstance after,int delay,int isStatic);

bool EqualPortMapping(PortInstance p1,PortInstance p2);

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
                     MergeModifier modifier = MergeModifier_NONE,MergingStrategy strat = MergingStrategy::CONSOLIDATION_GRAPH);

FUDeclaration* Merge2(Array<FUDeclaration*> types,
                     String name,Array<SpecificMergeNode> specifics,
                      MergeModifier modifier = MergeModifier_NONE,MergingStrategy strat = MergingStrategy::CONSOLIDATION_GRAPH);

/*

How merge works currently.

First we flatten all the accelerators into the most basic components.

We start with a copy of the first graph as the merged graph.
We iteratively calculate the mapping from the input graphs with the merged graph and then merge them.
We also store the mappings that map the other graph to the units on the merged graph.
We also calculate inverse mappings (from merged to inputs).
We calculate nodes that contain multiple inputs (candidates for multiplexers).
For each node with multiple inputs, we create a CombMux and use the maps to find the input port of the connection.
- Input graph 2 connects to port 2 of the CombMux.

At this point the merged graph is free of multiple inputs. It contains CombMuxes to resolve any input problem.

At this point we start calculating the configuration info.
In order to calculate this info and in order to correctly pass information from the input graphs to the merged configs, we first calculate a recon graph for each input graph that contains the new added CombMuxes (it's a reconstitution but if a mux was added to a path then that recon graph will contain the mux as well).

We also calculate all sorts of mappings. In the end, we have a bunch of recon graphs that map between input graphs to merged graph but also contain the extra units added.

- Because we want to support hiearchical merge, this stuff becomes more complicated although not sure how yet.

At the same time, after calculating all the recons, we try to fix graph delays by calculating delays in the recons and adding a variable buffer on the merged graph if a delay is needed. If so, we recalculate everything, including the recons graphs again to also include the delays added.

At this point, after calculating all the recons and the graphs having the buffers inserted, we can calculate and try to match the data from the recons and store it into the configuration info.

What data is mostly stored that depends on the input graphs?
- The original node name. (Merged nodes will have multiple names, one for each merged unit).
- Wether the node belongs or not to that configuration.
- Misc data like the addressGensUsed by that unit and whether the unit is being debugged or not.
- For muxes we need to store which input is associated to that configuration. (Depends on input graphs because the port is equal to the index of the input graph).

Generally, I do want to be able to store data inside the configuration info that depends on the input configuration info. Having this mapping is good, it means that we associate extra data to the input graphs and the merged graph is capable of processing it.

OK, so what is it that I want?

I want to merge the accelerators and have mappings from the merged accelerator to the input graphs.

The two extra units that can be added are the muxes and the variable buffers.
The problem with the buffers is that by adding them we are potentially changing the delay calculations of other graphs.
- Adding multiplexers is easy. Can be done in a single "round".
- Adding buffers is hard. Because buffers can change the time properties of other graphs we need to make sure that when we add buffers we also add them to the other recons 

As long as I can create the recons and stuff I think that I can do the rest of the code.

The problem is the interaction between hierarchical merge stuff. Need to figure out what is the biggest change between the ReconstituteGraphFromStruct and ReconstituteGraph are.



*/

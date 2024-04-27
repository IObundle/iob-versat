#pragma once

#include "versat.hpp"
#include "thread.hpp"
#include <unordered_map>

//#define MAX_CLIQUE_TIME 10.0f

struct FUInstance;
struct InstanceNode;
struct OrderedInstance;
struct Accelerator;
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


struct MergeEdge{
   FUInstance* instances[2];
};

struct MappingNode{ // Mapping (edge to edge or node to node)
   union{
      MergeEdge nodes;
      PortEdge edges[2];
   };

   enum {NODE,EDGE} type;
};

inline bool operator==(const MappingNode& node1,const MappingNode& node2){
   bool res = (node1.edges[0] == node2.edges[0] &&
               node1.edges[1] == node2.edges[1]);
   return res;
}

struct MappingEdge{ // Edge between mapping from edge to edge
   MappingNode* nodes[2];
};

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

// Represents the full algorithm
struct RefParallelState{
   ConsolidationGraph result;
   ConsolidationGraph graphCopy;
   Byte* arenaMark; // Stores everything expect result, allocated before hand
   Arena* arena;
   TaskFunction taskFunction;
   clock_t start;
   int nThreads;
   int upperBound;
   int i;

   Array<Arena> threadArenas;
   Array<int> table; // Shared by threads
   int max;
   bool timeout;
};

struct ParallelCliqueState{
   volatile int* max;
   volatile int thisMax;
   Array<int> table;
   Array<bool> tableLooked;
   ConsolidationGraph* cliqueOut;
   clock_t start;
   int counter;
   int index;
   int upperbound;
   bool timeout;
   bool found;
};

// Allocated for each task
struct RefParallelTask{
   RefParallelState* state;
   int index;
   float MAX_CLIQUE_TIME;
};

struct IsCliqueResult{
   bool result;
   int failedIndex;
};

typedef std::unordered_map<FUInstance*,FUInstance*> InstanceMap;
typedef std::unordered_map<PortEdge,PortEdge> PortEdgeMap;
typedef std::unordered_map<Edge*,Edge*> EdgeMap;
typedef Hashmap<InstanceNode*,InstanceNode*> InstanceNodeMap;

struct MergeGraphResult{
   Accelerator* accel1; // Should pull out the graph stuff instead of using an Accelerator for this
   Accelerator* accel2;

   InstanceNodeMap* map1; // Maps node from accel1 to newGraph
   InstanceNodeMap* map2; // Maps node from accel2 to newGraph
   //std::vector<Edge*> accel1EdgeMap;
   //std::vector<Edge*> accel2EdgeMap;
   Accelerator* newGraph;
};

struct MergeGraphResultExisting{
   Accelerator* result;
   Accelerator* accel2;
   InstanceNodeMap* map2;
};

struct GraphMapping{
   InstanceMap instanceMap;
   InstanceMap reverseInstanceMap;
   PortEdgeMap edgeMap;
};

enum MergingStrategy{
   SIMPLE_COMBINATION,
   CONSOLIDATION_GRAPH,
   FIRST_FIT
};

void OutputConsolidationGraph(ConsolidationGraph graph,Arena* memory,bool onlyOutputValid,const char* format,...);

ConsolidationResult GenerateConsolidationGraph(Versat* versat,Arena* arena,Accelerator* accel1,Accelerator* accel2,ConsolidationGraphOptions options);
MergeGraphResult MergeGraph(Versat* versat,Accelerator* flatten1,Accelerator* flatten2,GraphMapping& graphMapping,String name);
void AddCliqueToMapping(GraphMapping& res,ConsolidationGraph clique);

void InsertMapping(GraphMapping& map,PortEdge& edge0,PortEdge& edge1);
//void Clique(CliqueState* state,ConsolidationGraph graphArg,int index,IndexRecord* record,int size,Arena* arena);

bool NodeMappingConflict(PortEdge edge1,PortEdge edge2);
bool MappingConflict(MappingNode map1,MappingNode map2);
ConsolidationGraph Copy(ConsolidationGraph graph,Arena* arena);
int NodeIndex(ConsolidationGraph graph,MappingNode* node);

bool MappingConflict(MappingNode map1,MappingNode map2);
CliqueState MaxClique(ConsolidationGraph graph,int upperBound,Arena* arena,float MAX_CLIQUE_TIME);
ConsolidationGraph GenerateConsolidationGraph(Versat* versat,Arena* arena,Accelerator* accel1,Accelerator* accel2,ConsolidationGraphOptions options,MergingStrategy strategy);

MergeGraphResult HierarchicalHeuristic(Versat* versat,FUDeclaration* decl1,FUDeclaration* decl2,String name);

int ValidNodes(ConsolidationGraph graph);

BitArray* CalculateNeighborsTable(ConsolidationGraph graph,Arena* out);

IsCliqueResult IsClique(ConsolidationGraph graph);

ConsolidationGraph ParallelMaxClique(ConsolidationGraph graph,int upperBound,Arena* arena,float MAX_CLIQUE_TIME);

String MappingNodeIdentifier(MappingNode* node,Arena* memory);
MergeGraphResult HierarchicalMergeAccelerators(Versat* versat,Accelerator* accel1,Accelerator* accel2,String name);
MergeGraphResult HierarchicalMergeAcceleratorsFullClique(Versat* versat,Accelerator* accel1,Accelerator* accel2,String name);

FUDeclaration* MergeAccelerators(Versat* versat,FUDeclaration* accel1,FUDeclaration* accel2,String name,int flatteningOrder = 99,MergingStrategy strategy = MergingStrategy::CONSOLIDATION_GRAPH,SpecificMerge* specifics = nullptr,int nSpecifics = 0);
//FUDeclaration* MergeThree(Versat* versat,FUDeclaration* typeA,FUDeclaration* typeB,FUDeclaration* typeC);
FUDeclaration* Merge(Versat* versat,Array<FUDeclaration*> types,String name,MergingStrategy strat = MergingStrategy::CONSOLIDATION_GRAPH);

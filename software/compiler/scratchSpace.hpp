#pragma once

#include "versat.hpp"
#include "merge.hpp"
#include "textualRepresentation.hpp"
#include "debug.hpp"
#include "intrinsics.hpp"

// Bunch of code written when exploring a way of doing hierarchical merging.
// Didn't fully worked (couldn't generate the circuits and sim-run or pc-emul them) but the graph merging part did.
// Probably best to rewrite if needing to implement. 

#if 0

struct IndexMapping{
   int index0;
   int index1;
};

struct SubgraphMapping{
   FUDeclaration* decl1;
   FUDeclaration* decl2;
   Array<IndexMapping> edgeMappings;
   //Array<int> table;
   //Array<IndexMapping> tableMappings;
};

struct GenerateMappingNodesResult{
   Array<MappingNode> nodes;
   Pool<MappingNode> specificsAdded;
};

struct CalculateCliqueAndGetMappingsResult{
   Array<IndexMapping> mappings;
   Array<int> cliqueTable;
   Array<IndexMapping> cliqueTableMappings;
};

struct FlattenMapping{
   FlatteningTemp* t1;
   FlatteningTemp* t2;
   Array<IndexMapping> edgeMappings;
   //int weight_;
};

struct MappingState{
   FUDeclaration* decl1;
   FUDeclaration* decl2;

   FlatteningTemp* t1;
   FlatteningTemp* t2;

   Array<IndexMapping> edgeMappings;

   Array<int> submappingsNeeded;
   bool computing;
   bool computed;
};

struct MinResult{
   int value;
   int index;
};

static const int PROCESS_STATE_WAITING = 0;
static const int PROCESS_STATE_COMPUTING = 1;

struct ColoredOrderingResult{
   Array<int> order;
   int upperBound;
};

struct SimpleCondition{
   pthread_mutex_t mutex;
   pthread_cond_t cond;
   volatile bool signalled;
};

static SimpleCondition InitSimpleCondition(){
   SimpleCondition res = {};
   pthread_mutex_init(&res.mutex,NULL);
   pthread_cond_init(&res.cond,NULL);
   return res;
}

static void Wait(SimpleCondition* cond){
   pthread_mutex_lock(&cond->mutex);

   while(!cond->signalled){
      pthread_cond_wait(&cond->cond,&cond->mutex);
   }

   cond->signalled = false;

   pthread_mutex_unlock(&cond->mutex);
}

static void Signal(SimpleCondition* cond){
   pthread_mutex_lock(&cond->mutex);
   cond->signalled = true;
   pthread_mutex_unlock(&cond->mutex);

   pthread_cond_broadcast(&cond->cond);
}

struct HeuristicState{
   // Input
   Graph* graph1;
   Graph* graph2;
   FlatteningTemp* t1;
   FlatteningTemp* t2;
   int input;

   // Input and output
   Array<SubgraphMapping> mappings;
   volatile int* inserted;

   // Output
   Arena mappingArena;
   Arena* resultArena;
   pthread_mutex_t* mappingMutex; // For output
   SimpleCondition* cond;

   bool finished;
   bool set;
};

Subgraph DefaultSubgraph(Graph* graph);
void AddSpecificsToMapping(GraphMapping& mapping,Pool<MappingNode> specifics);
Subgraph RemoveInputs(Subgraph sub);
Subgraph RemoveInputsAndOutputs(Subgraph sub);

void PrintGraphEdges(ConsolidationGraph* graph,Arena* arena);

void Clique(CliqueState* state,ConsolidationGraph graphArg,int index,IndexRecord* record,int size,Arena* arena);
CliqueState* InitMaxClique(ConsolidationGraph graph,int upperBound,Arena* arena);
void RunMaxClique(CliqueState* state,Arena* arena,float MAX_CLIQUE_TIME);
CliqueState MaxClique(ConsolidationGraph graph,int upperBound,Arena* arena);

void AllMaxCliques(CliqueState* state,Array<ConsolidationGraph>& allCliques,int& maxCliquesFound,ConsolidationGraph graphArg,int index,IndexRecord* record,int size,Arena* arena);
Array<ConsolidationGraph> AllMaxClique(ConsolidationGraph graph,int upperBound,Arena* arena);

ConsolidationGraph BuildConsolidationGraphFromNodes(Array<MappingNode> nodes,Arena* arena);
ConsolidationResult GenerateConsolidationGraphForSubgraphs(Subgraph accel1,Subgraph accel2,Array<MappingNode> implicitNodes,Arena* arena);
ConsolidationResult GenerateConsolidationGraphGivenInitialClique(Subgraph accel1,Subgraph accel2,ConsolidationGraph givenClique,Arena* arena);
CalculateCliqueAndGetMappingsResult CalculateCliqueAndGetMappings(Subgraph sub1,Subgraph sub2,Arena* arena);

void OutputDotGraph(Graph* graph,const char* filepath,Arena* temp);
void OutputDotGraph(Subgraph graph,const char* filepath,Arena* temp);

Array<FlatteningTemp> HierarchicalFlattening(Graph* graph,Arena* arena);

MergeGraphResult ParallelHierarchicalHeuristic(Versat* versat,FUDeclaration* decl1,FUDeclaration* decl2,String name);

Array<IndexMapping> GetIndexMappingFromClique(ConsolidationGraph clique,Subgraph sub1,Subgraph sub2,Arena* out);
IndexMapping GetMappingNodeIndexes(MappingNode node,Edge* sub1,Edge* sub2);
int GetMappingBit(ConsolidationGraph& cg,Graph* graph1,Graph* graph2,IndexMapping map);
Array<IndexMapping> TransformGenericIntoSpecific(Array<IndexMapping> generic,FlatteningTemp* t1,FlatteningTemp* t2,Arena* perm);

Array<int> DegreeReordering(ConsolidationGraph graph,Arena* arena);
Array<int> RadixReordering(ConsolidationGraph graph,Arena* arena);
ColoredOrderingResult ColoredOrdering(ConsolidationGraph graph,Arena* arena);

Array<int> CalculateDegrees(ConsolidationGraph graph,Arena* arena);
Array<int> CalculateCoreNumbers(ConsolidationGraph graph,Arena* arena);

Array<MappingNode> ReorderMappingNodes(Array<MappingNode> nodes, Array<int> order,Arena* temp);

GenerateMappingNodesResult GenerateMappingNodes(Subgraph accel1,Subgraph accel2,Arena* out,Arena* temp,Array<MappingNode> implicitNodes);

void ReorganizeAccelerator(Accelerator* accel,Arena* temp);

MinResult FindSmallest(Array<int> array);
Array<int> CountValues(Array<int> array,int maximumValue,Arena* out);
Array<int> OffsetGivenCounts(Array<int> counts,Arena* out);
void AssertIndexedArray(Array<int> array,Arena* arena);

void SortEdgesByVertices(Graph* graph); // Defined in graph.cpp

static bool operator==(const IndexMapping& m0,const IndexMapping& m1){
   bool res = (m0.index0 == m1.index0 && m0.index1 == m1.index1);
   return res;
}

bool CheckIfMappingIsPossible(FlatteningTemp* head1,FlatteningTemp* head2,Array<SubgraphMapping> mappings);

#endif

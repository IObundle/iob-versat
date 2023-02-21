#ifndef INCLUDED_MERGE
#define INCLUDED_MERGE

#include "versatPrivate.hpp"
#include "thread.hpp"

#define MAX_CLIQUE_TIME 10.0f

struct IndexRecord{
   int index;
   IndexRecord* next;
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
};

struct IsCliqueResult{
   bool result;
   int failedIndex;
};

void InsertMapping(GraphMapping& map,PortEdge& edge0,PortEdge& edge1);
void Clique(CliqueState* state,ConsolidationGraph graphArg,int index,IndexRecord* record,int size,Arena* arena);

bool NodeMappingConflict(PortEdge edge1,PortEdge edge2);
bool MappingConflict(MappingNode map1,MappingNode map2);
ConsolidationGraph Copy(ConsolidationGraph graph,Arena* arena);
int NodeIndex(ConsolidationGraph graph,MappingNode* node);

MergeGraphResult HierarchicalHeuristic(Versat* versat,FUDeclaration* decl1,FUDeclaration* decl2,String name);

int ValidNodes(ConsolidationGraph graph);

BitArray* CalculateNeighborsTable(ConsolidationGraph graph,Arena* arena);

IsCliqueResult IsClique(ConsolidationGraph graph);

ConsolidationGraph ParallelMaxClique(ConsolidationGraph graph,int upperBound,Arena* arena);

#endif // INCLUDED_MERGE

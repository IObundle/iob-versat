#include "scratchSpace.hpp"

#include <unistd.h>

#if 0
#define G(r,i,a,b,c,d)                      \
  do {                                      \
    a = a + b + m[blake2s_sigma[r][2*i+0]]; \
    d = rotr32(d ^ a, 16);                  \
    c = c + d;                              \
    b = rotr32(b ^ c, 12);                  \
    a = a + b + m[blake2s_sigma[r][2*i+1]]; \
    d = rotr32(d ^ a, 8);                   \
    c = c + d;                              \
    b = rotr32(b ^ c, 7);                   \
  } while(0)

#define ROUND(r)                    \
  do {                              \
    G(r,0,v[ 0],v[ 4],v[ 8],v[12]); \
    G(r,1,v[ 1],v[ 5],v[ 9],v[13]); \
    G(r,2,v[ 2],v[ 6],v[10],v[14]); \
    G(r,3,v[ 3],v[ 7],v[11],v[15]); \
    G(r,4,v[ 0],v[ 5],v[10],v[15]); \
    G(r,5,v[ 1],v[ 6],v[11],v[12]); \
    G(r,6,v[ 2],v[ 7],v[ 8],v[13]); \
    G(r,7,v[ 3],v[ 4],v[ 9],v[14]); \
  } while(0)

static void blake2s_compress( blake2s_state *S, const uint8_t in[BLAKE2S_BLOCKBYTES] )
{
  uint32_t m[16];
  uint32_t v[16];
  size_t i;

  for( i = 0; i < 16; ++i ) {
    m[i] = load32( in + i * sizeof( m[i] ) );
  }

  for( i = 0; i < 8; ++i ) {
    v[i] = S->h[i];
  }

  v[ 8] = blake2s_IV[0];
  v[ 9] = blake2s_IV[1];
  v[10] = blake2s_IV[2];
  v[11] = blake2s_IV[3];
  v[12] = S->t[0] ^ blake2s_IV[4];
  v[13] = S->t[1] ^ blake2s_IV[5];
  v[14] = S->f[0] ^ blake2s_IV[6];
  v[15] = S->f[1] ^ blake2s_IV[7];

  ROUND( 0 );
  ROUND( 1 );
  ROUND( 2 );
  ROUND( 3 );
  ROUND( 4 );
  ROUND( 5 );
  ROUND( 6 );
  ROUND( 7 );
  ROUND( 8 );
  ROUND( 9 );

  for( i = 0; i < 8; ++i ) {
    S->h[i] = S->h[i] ^ v[i] ^ v[i + 8];
  }
}
#endif

#if 0
// Old version
MergeGraphResult HierarchicalMergeAccelerators(Versat* versat,Accelerator* accel1,Accelerator* accel2,String name){
   Arena* arena = &versat->temp;
   ArenaMarker marker(arena);

   Graph* graph1 = accel1;
   Array<FlatteningTemp> res1 = HierarchicalFlattening(graph1,arena);

   printf("Graph1: \n");
   PrintGraphInfo(graph1);

   Graph* graph2 = accel2;
   Array<FlatteningTemp> res2 = HierarchicalFlattening(graph2,arena);

   printf("Graph2: \n");
   PrintGraphInfo(graph2);

   Hashmap<FUDeclaration*,FlatteningTemp*> decl1 = {};
   decl1.Init(arena,999);

   Hashmap<FUDeclaration*,FlatteningTemp*> decl2 = {};
   decl2.Init(arena,999);

   FOREACH_LIST(ptr,&res1[0]){
      printf("[%.*s] %.*s %d\n",UNPACK_SS(ptr->decl->name),UNPACK_SS(ptr->name),ptr->level);
      decl1.InsertIfNotExist(ptr->decl,ptr);
   }

   FOREACH_LIST(ptr,&res2[0]){
      printf("[%.*s] %.*s %d\n",UNPACK_SS(ptr->decl->name),UNPACK_SS(ptr->name),ptr->level);
      decl2.InsertIfNotExist(ptr->decl,ptr);
   }

   OutputDotGraph(graph1,"debug/graph_0.dot",arena);
   OutputDotGraph(graph2,"debug/graph_1.dot",arena);

   // Get subgraph mappings (one for each declaration pair)
   Array<SubgraphMapping> mappings = PushArray<SubgraphMapping>(arena,99);
   int index = 0;
   for(auto pair1 : decl1){
      for(auto pair2 : decl2){
         if(pair1.first == pair2.first){
            continue;
         }
         FUDeclaration* t1 = pair1.second->decl;
         FUDeclaration* t2 = pair2.second->decl;

         printf("%.*s %.*s\n",UNPACK_SS(t1->name),UNPACK_SS(t2->name));

         FlushStdout();
   //exit(0);

         Subgraph sub1 = {};
         sub1.nodes = SimpleSublist(ListGet(graph1->instances,pair1.second->index),pair1.second->subunitsCount);
         sub1.edges = SimpleSublist(ListGet(graph1->edges,pair1.second->edgeStart),pair1.second->edgeCount);

         Subgraph sub2 = {};
         sub2.nodes = SimpleSublist(ListGet(graph2->instances,pair2.second->index),pair2.second->subunitsCount);
         sub2.edges = SimpleSublist(ListGet(graph2->edges,pair2.second->edgeStart),pair2.second->edgeCount);

         OutputDotGraph(sub1,StaticFormat("debug/sub_%.*s_sub.dot",UNPACK_SS(t1->name)),arena);
         OutputDotGraph(sub2,StaticFormat("debug/sub_%.*s_sub.dot",UNPACK_SS(t2->name)),arena);

         CalculateCliqueAndGetMappingsResult res = CalculateCliqueAndGetMappings(sub1,sub2,arena); // Should be, in theory,a pretty good aproximation
         Array<IndexMapping> indexMappings = res.mappings;

         #if 0
         for(IndexMapping m : indexMappings){
            printf("Map edge %d to %d\n",m.index0,m.index1);
         }
         printf("\n");
         #endif

         mappings[index].decl1 = t1;
         mappings[index].decl2 = t2;
         mappings[index].edgeMappings = indexMappings;
         mappings[index].table = res.cliqueTable;
         mappings[index].tableMappings = res.cliqueTableMappings;
         index += 1;
      }
   }
   mappings.size = index;

   Subgraph sub1 = DefaultSubgraph(graph1);
   Subgraph sub2 = DefaultSubgraph(graph2);

   ConsolidationResult result = GenerateConsolidationGraphForSubgraphs(sub1,sub2,{},arena);
   ConsolidationGraph graph = result.graph;

   Array<FlattenMapping> specificMappings = GetSpecificMappings(&res1[0],&res2[0],mappings,arena);

   #if 1
   for(FlattenMapping& m : specificMappings){
      printf("%.*s:%d %.*s:%d - %d\n",UNPACK_SS(m.t1->decl->name),m.t1->index,UNPACK_SS(m.t2->decl->name),m.t2->index,m.weight);
   }
   #endif

   for(int i = 0; i < specificMappings.size; i++){
      for(int j = i + 1; j < specificMappings.size; j++){
         if(specificMappings[j].t1->index > specificMappings[i].t1->index){
            FlattenMapping temp = specificMappings[j];
            specificMappings[j] = specificMappings[i];
            specificMappings[i] = temp;
         }
      }
   }

   #if 0
   printf("Possible mappings:\n");
   for(int i = 0; i < graph.nodes.size; i++){
      IndexMapping map = GetMappingNodeIndexes(graph.nodes[i],graph1->edges,graph2->edges);

      printf("%02d : %02d    ",map.index0,map.index1);

      EdgeNode* e1 = ListGet(graph1->edges,map.index0);
      EdgeNode* e2 = ListGet(graph2->edges,map.index1);

      ArenaMarker marker(arena);
      String str1 = Repr(e1->edge,GRAPH_DOT_FORMAT_NAME,arena);
      String str2 = Repr(e2->edge,GRAPH_DOT_FORMAT_NAME,arena);
      printf("%.*s // %.*s\n",UNPACK_SS(str1),UNPACK_SS(str2));
   }
   #endif

   ConsolidationGraph approximateClique = Copy(graph,arena);
   approximateClique.validNodes.Fill(0);

   for(FlattenMapping& m : specificMappings){
      ArenaMarker marker(arena);
      Array<IndexMapping> graphMappings = TransformGenericIntoSpecific(m.mapping->edgeMappings,m.t1,m.t2,arena);

      Array<IndexMapping> graphTableMappings = TransformGenericIntoSpecific(m.mapping->tableMappings,m.t1,m.t2,arena);

      #if 0
      for(IndexMapping& map : graphTableMappings){
         printf("%02d %02d\n",map.index0,map.index1);
      }
      #endif

      for(IndexMapping& specific : graphMappings){
         int bit = GetMappingBit(graph,graph1,graph2,specific);

         approximateClique.validNodes.Set(bit,1);
      }
   }
   Assert(IsClique(approximateClique).result);

   GraphMapping mapping = {};

   // Full clique
   #if 01
   {
   printf("Full clique:\n");
   CliqueState* state = InitMaxClique(graph,999,arena);

   RunMaxClique(state,arena);
   ConsolidationGraph clique = state->clique;
   printf("Size: %d\n",graph.nodes.size);
   printf("Size of clique: %d\n",clique.validNodes.GetNumberBitsSet());

   AddCliqueToMapping(mapping,clique);

   }
   #endif

   Array<int> coreNumbers = CalculateCoreNumbers(graph,arena);

   #if 0
   printf("\n");
   for(int i = 0; i < graph.edges.size; i++){
      BitArray& arr = graph.edges[i];

      region(arena){
         String repr = arr.PrintRepresentation(arena);
         printf("%.*s %d\n",UNPACK_SS(repr),arr.GetNumberBitsSet());
      };
   }
   printf("\n");
   #endif

   #if 0
   {
   printf("Given approximate clique:\n");
   ConsolidationResult resultingGraph = GenerateConsolidationGraphGivenInitialClique(sub1,sub2,approximateClique,arena);
   ConsolidationGraph graph = resultingGraph.graph;
   OutputConsolidationGraph(graph,arena,true,"debug/specificCG.dot");

   CliqueState* state = InitMaxClique(graph,999,arena);
   RunMaxClique(state,arena);
   ConsolidationGraph extraClique = state->clique;
   Assert(IsClique(extraClique).result);

   printf("Size: %d\n",graph.nodes.size);
   printf("Size Initial clique: %d\n",approximateClique.validNodes.GetNumberBitsSet());
   printf("Size of extra clique: %d\n",extraClique.validNodes.GetNumberBitsSet());

   AddCliqueToMapping(mapping,approximateClique);
   AddCliqueToMapping(mapping,extraClique);
   }
   #endif

   #if 0
   printf("\n\n");
   for(int i = 0; i < graph.validNodes.bitSize; i++){
      IndexMapping map = GetMappingNodeIndexes(graph.nodes[i],graph1->edges,graph2->edges);

      printf("%d : %d\n",map.index0,map.index1);
   }
   #endif

   #if 0
   //CalculateCliqueAndGetMappings(versat,sub1,sub2,arena);

   //Array<ConsolidationGraph> cliques = AllMaxClique(result.graph,999,arena);
   CliqueState state = MaxClique(result.graph,999,arena);
   ConsolidationGraph clique = state.clique;

   for(int i = 0; i < clique.nodes.size; i++){
      IndexMapping map = GetMappingNodeIndexes(clique.nodes[i],graph1->edges,graph2->edges);

      printf("%02d : %02d    ",map.index0,map.index1);

      EdgeNode* e1 = ListGet(graph1->edges,map.index0);
      EdgeNode* e2 = ListGet(graph2->edges,map.index1);

      ArenaMarker marker(arena);
      String str1 = Repr(e1->edge,GRAPH_DOT_FORMAT_NAME,arena);
      String str2 = Repr(e2->edge,GRAPH_DOT_FORMAT_NAME,arena);
      printf("%.*s // %.*s\n",UNPACK_SS(str1),UNPACK_SS(str2));
   }
   #endif

   #if 0
   printf("Approximate clique:\n");
   for(int i : approximateClique.validNodes){
      IndexMapping map = GetMappingNodeIndexes(approximateClique.nodes[i],graph1->edges,graph2->edges);

      printf("%d : %d\n",map.index0,map.index1);
   }
   #endif

   #if 0
   printf("True Cliques:\n");
   // The best 99 cliques
   Array<ConsolidationGraph> cliques = AllMaxClique(result.graph,999,arena);

   //CliqueState state = MaxClique(result.graph,999,arena);
   //ConsolidationGraph clique = state.clique;
   for(ConsolidationGraph& clique : cliques){
      for(int i : clique.validNodes){
         IndexMapping map = GetMappingNodeIndexes(clique.nodes[i],graph1->edges,graph2->edges);

         printf("%d : %d\n",map.index0,map.index1);
      }
      printf("\n");
   }
   #endif

   #if 0
   {
   ArenaMarker marker(arena);
   printf("Max Clique:\n");
   CliqueState* state = InitMaxClique(result.graph,999,arena);
   Memset(state->table,0);

   #if 0
   for(int i = 0; i < state->table.size; i++){
      MappingNode node = state->clique.nodes[i];

      IndexMapping map = GetMappingNodeIndexes(node,graph1->edges,graph2->edges);

      printf("%d %d\n",map.index0,map.index1);
   }
   #endif

   #if 0
   printf("Before:\n");
   int extra = 0;
   for(FlattenMapping& m : specificMappings){
      ArenaMarker marker(arena);
      Array<IndexMapping> graphTableMappings = TransformGenericIntoSpecific(m.mapping->tableMappings,m.t1,m.t2,arena);

      int maxAdded = 0;
      int index = 0;
      for(IndexMapping& map : graphTableMappings){
         printf("%02d %02d\n",map.index0,map.index1);

         int i = GetMappingBit(graph,graph1,graph2,map);
         state->table[i] = m.mapping->table[index++] + extra;
         maxAdded = std::max(state->table[i],maxAdded);
      }
      extra += maxAdded;
   }
   #endif

   Array<int> coreNumbers = CalculateCoreNumbers(graph,arena);

   for(int i = graph.nodes.size - 1; i >= 0; i--){
      if(state->table[i] == 0){
         state->startI = i + 1;
         break;
      }
   }
   if(state->startI >= graph.nodes.size){
      state->startI = graph.nodes.size - 1;
   }

   state->max = 0;
   for(int i = state->startI; i < graph.nodes.size; i++){
      state->max = std::max(state->table[i],state->max);
   }
   printf("Max: %d\n",state->max);

   #if 0
   int size = state->clique.nodes.size;

   int lastPos = size - 1;
   for(int i = lastPos; i >= 0; i--){
      if(state->table[i] != 0){
         if(i != lastPos){
            Assert(state->table[lastPos] == 0);

            //PrintGraphEdges(&state->clique,arena);

            printf("SwapCliqueNodes(%d,%d);\n",i,lastPos);
            SwapCliqueNodes(state,i,lastPos);

            //String res = PushIntTableRepresentation(arena,state->table);
            //printf("%.*s\n",UNPACK_SS(res));

            //PrintGraphEdges(&state->clique,arena);

            //exit(0);
         }
         lastPos -= 1;
      }
   }
   state->max = state->table[lastPos + 1];
   state->startI = lastPos;
   #endif

   String res = PushIntTableRepresentation(arena,state->table);
   printf("%.*s\n",UNPACK_SS(res));

   sleep(1);

   //exit(0);

   //clock_t start = clock();
   MemoryBarrier();
   uint64 start = RDTSC();
   RunMaxClique(state,arena);
   uint64 end = RDTSC();
   //clock_t end = clock();

   printf("%ld\n",end - start);

   ConsolidationGraph clique = state->clique;
   for(int i : clique.validNodes){
      ArenaMarker marker(arena);
      IndexMapping map = GetMappingNodeIndexes(clique.nodes[i],graph1->edges,graph2->edges);

      String r = Repr(clique.nodes[i],arena);

      printf("%d : %d - %.*s %.*s\n",map.index0,map.index1,UNPACK_SS(r));
   }
   printf("\n");

   res = PushIntTableRepresentation(arena,state->table);
   printf("%.*s\n",UNPACK_SS(res));
   }
   #endif

   AddSpecificsToMapping(mapping,result.specificsAdded);
   //AddCliqueToMapping(mapping,approximateClique);

   printf("%d\n",mapping.edgeMap.size());

   return MergeGraph(versat,graph1,graph2,mapping,STRING("Test"));
}

#endif

bool Contains(IndexRecord* record,int toCheck){
   FOREACH_LIST(ptr,record){
      if(ptr->index == toCheck){
         return true;
      }
   }
   return false;
}

void OutputDotGraph(Graph* graph,const char* filepath,Arena* temp){
   ArenaMarker marker(temp);
   String str = OutputDotGraph(graph,temp);
   FILE* file = OpenFileAndCreateDirectories(filepath,"w");
   fprintf(file,"%.*s",UNPACK_SS(str));
   fclose(file);
}

void OutputDotGraph(Subgraph graph,const char* filepath,Arena* temp){
   ArenaMarker marker(temp);
   String str = OutputDotGraph(graph,temp);
   FILE* file = OpenFileAndCreateDirectories(filepath,"w");
   fprintf(file,"%.*s",UNPACK_SS(str));
   fclose(file);
}

void ReorganizeAccelerator(Accelerator* accel,Arena* temp){
   AcceleratorView view = CreateAcceleratorView(accel,temp);
   view.CalculateGraphData(temp);
   view.CalculateDAGOrdering(temp);

   // Reorganize nodes based on DAG order
   DAGOrder order = view.order;
   int size = view.nodes.Size();
   accel->instances = order.instances[0];
   for(int i = 0; i < size - 1; i++){
      order.instances[i]->next = order.instances[i+1];
   }
   order.instances[size-1]->next = nullptr;
}

Array<MappingNode> ReorderMappingNodes(Array<MappingNode> nodes, Array<int> order,Arena* temp){
   Array<MappingNode> ordered = PushArray<MappingNode>(temp,order.size);

   for(int i = 0; i < ordered.size; i++){
      ordered[i] = nodes[order[i]];
   }

   return ordered;
}

void Clique(CliqueState* state,ConsolidationGraph graphArg,int index,IndexRecord* record,int size,Arena* arena){
   state->iterations += 1;

   //ConsolidationGraph graph = Copy(graphArg,arena);
   ConsolidationGraph graph = graphArg;

   int num = graph.validNodes.GetNumberBitsSet();
   if(num == 0){
      if(size > state->max){
         state->max = size;

         // Record best clique found so far
         state->clique.validNodes.Fill(0);
         for(IndexRecord* ptr = record; ptr != nullptr; ptr = ptr->next){
            state->clique.validNodes.Set(ptr->index,1);
         }

         state->found = true;
      }
      return;
   }

   auto end = GetTime();
   float elapsed = end - state->start;
   if(elapsed > MAX_CLIQUE_TIME){
      state->found = true;
      return;
   }

   int lastI = index;
   do{
      if(size + num <= state->max){
         return;
      }

      #if 0
      if(size + num <= state->table[index]){ // The problem is that state->max will already be equal to this by the time we arrive here.
         return;
      }
      #endif

      int i = graph.validNodes.FirstBitSetIndex(lastI);

      if(size + state->table[i] <= state->max){
         return;
      }

      graph.validNodes.Set(i,0);

      Byte* mark = MarkArena(arena);
      ConsolidationGraph tempGraph = Copy(graph,arena);

      tempGraph.validNodes &= graph.edges[i];

      IndexRecord newRecord = {};
      newRecord.index = i;
      newRecord.next = record;

      //printf("%d\n",i);
      Clique(state,tempGraph,i,&newRecord,size + 1,arena);

      PopMark(arena,mark);

      if(state->found == true){
         return;
      }

      lastI = i;
   } while((num = graph.validNodes.GetNumberBitsSet()) != 0);
}

CliqueState* InitMaxClique(ConsolidationGraph graph,int upperBound,Arena* arena){
   CliqueState* state = PushStruct<CliqueState>(arena);
   *state = {};

   state->table = PushArray<int>(arena,graph.nodes.size);
   state->clique = Copy(graph,arena); // Preserve nodes and edges, but allocates different valid nodes
   state->upperBound = upperBound;

   return state;
}

void RunMaxClique(CliqueState* state,Arena* arena){
   ConsolidationGraph graph = Copy(state->clique,arena);
   graph.validNodes.Fill(0);

   state->start = GetTime();

   int startI = state->startI ? state->startI : graph.nodes.size - 1;

   for(int i = startI; i >= 0; i--){
      Byte* mark = MarkArena(arena);

      state->found = false;
      for(int j = i; j < graph.nodes.size; j++){
         graph.validNodes.Set(j,1);
      }

      graph.validNodes &= graph.edges[i];

      IndexRecord record = {};
      record.index = i;

      //printf("%d / %d\r",i,graph.nodes.size);
      Clique(state,graph,i,&record,1,arena);
      state->table[i] = state->max;

      PopMark(arena,mark);

      if(state->max == state->upperBound){
         printf("Upperbound finish, index: %d (from %d)\n",i,graph.nodes.size);
         break;
      }

      auto end = GetTime();
      float elapsed = end - state->start;
      if(elapsed > MAX_CLIQUE_TIME){
         printf("Timeout, index: %d (from %d)\n",i,graph.nodes.size);
         break;
      }
   }
   printf("\nFinished in: %d\n",state->iterations);

   Assert(IsClique(state->clique).result);
}

CliqueState MaxClique(ConsolidationGraph graph,int upperBound,Arena* arena){
   CliqueState state = {};
   state.table = PushArray<int>(arena,graph.nodes.size);
   state.clique = Copy(graph,arena); // Preserve nodes and edges, but allocates different valid nodes
   //state.start = std::chrono::steady_clock::now();

   graph.validNodes.Fill(0);

   //printf("Upper:%d\n",upperBound);

   state.start = clock();
   for(int i = graph.nodes.size - 1; i >= 0; i--){
      Byte* mark = MarkArena(arena);

      state.found = false;
      for(int j = i; j < graph.nodes.size; j++){
         graph.validNodes.Set(j,1);
      }

      graph.validNodes &= graph.edges[i];

      IndexRecord record = {};
      record.index = i;

      Clique(&state,graph,i,&record,1,arena);
      state.table[i] = state.max;

      PopMark(arena,mark);

      if(state.max == upperBound){
         printf("Upperbound finish, index: %d (from %d)\n",i,graph.nodes.size);
         break;
      }

      auto end = clock();
      float elapsed = (end - state.start) / CLOCKS_PER_SEC;
      if(elapsed > MAX_CLIQUE_TIME){
         printf("Timeout, index: %d (from %d)\n",i,graph.nodes.size);
         break;
      }
   }

   Assert(IsClique(state.clique).result);

   return state;
}

void AllMaxCliques(CliqueState* state,Array<ConsolidationGraph>& allCliques,int& maxCliquesFound,ConsolidationGraph graphArg,int index,IndexRecord* record,int size,Arena* arena){
   //ConsolidationGraph graph = Copy(graphArg,arena);
   ConsolidationGraph graph = graphArg;

   int num = graph.validNodes.GetNumberBitsSet();
   if(num == 0){
      if(size > state->max){
         state->max = size;
      }
      if(size >= 8){
         ConsolidationGraph& clique = allCliques[maxCliquesFound++ % allCliques.size];

         clique.validNodes.Fill(0);
         for(IndexRecord* ptr = record; ptr != nullptr; ptr = ptr->next){
            clique.validNodes.Set(ptr->index,1);
         }
      }
      return;
   }

   auto end = clock();
   float elapsed = (end - state->start) / CLOCKS_PER_SEC;
   if(elapsed > MAX_CLIQUE_TIME){
      state->found = true;
      return;
   }

   int lastI = index;
   do{
      #if 0
      if(size + num <= state->max){
         return;
      }
      #endif

      int i = graph.validNodes.FirstBitSetIndex(lastI);

      #if 0
      if(size + state->table[i] <= state->max){
         return;
      }
      #endif

      graph.validNodes.Set(i,0);

      Byte* mark = MarkArena(arena);
      ConsolidationGraph tempGraph = Copy(graph,arena);

      tempGraph.validNodes &= graph.edges[i];

      IndexRecord newRecord = {};
      newRecord.index = i;
      newRecord.next = record;

      AllMaxCliques(state,allCliques,maxCliquesFound,tempGraph,i,&newRecord,size + 1,arena);

      PopMark(arena,mark);

      if(state->found == true){
         return;
      }

      lastI = i;
   } while((num = graph.validNodes.GetNumberBitsSet()) != 0);
}

Array<ConsolidationGraph> AllMaxClique(ConsolidationGraph graph,int upperBound,Arena* arena){
   Array<ConsolidationGraph> result = PushArray<ConsolidationGraph>(arena,99);
   for(ConsolidationGraph& cg : result){
      cg = Copy(graph,arena);
   }

   CliqueState state = {};
   state.table = PushArray<int>(arena,graph.nodes.size);
   state.clique = Copy(graph,arena); // Preserve nodes and edges, but allocates different valid nodes

   graph.validNodes.Fill(0);

   state.start = clock();
   int maxCliquesFound = 0;
   for(int i = graph.nodes.size - 1; i >= 0; i--){
      Byte* mark = MarkArena(arena);

      state.found = false;
      for(int j = i; j < graph.nodes.size; j++){
         graph.validNodes.Set(j,1);
      }

      graph.validNodes &= graph.edges[i];

      IndexRecord record = {};
      record.index = i;

      //printf("C: %d\n",i);
      AllMaxCliques(&state,result,maxCliquesFound,graph,i,&record,1,arena);
      state.table[i] = state.max;

      PopMark(arena,mark);

      //printf("i:%d max:%d\n",i,state.max);
      if(state.max == upperBound){
         printf("Upperbound finish, index: %d (from %d)\n",i,graph.nodes.size);
         break;
      }

      auto end = clock();
      float elapsed = (end - state.start) / CLOCKS_PER_SEC;
      if(elapsed > MAX_CLIQUE_TIME){
         printf("Timeout, index: %d (from %d)\n",i,graph.nodes.size);
         break;
      }
   }

   #if 0
   for(int i = 0; i < graph.nodes.size; i++){
      printf("%d ",state.table[i]);
   }
   #endif

   printf("%d\n",maxCliquesFound);
   result.size = result.size < maxCliquesFound ? result.size : maxCliquesFound;

   #if 1
   for(ConsolidationGraph& graph : result){
      Assert(IsClique(graph).result);
   }
   #endif

   return result;
}

void FixFlatteningTempHierarchyRecursive(Array<FlatteningTemp> array,int level){
   for(int i = 0; i < array.size;){
      FlatteningTemp* cur = &array[i];
      if(i + 1 >= array.size){
         break;
      }

      int nextIndex = i + 1;
      for(; nextIndex < array.size; nextIndex++){
         if(array[nextIndex].level == level){
            break;
         }
      }

      if(nextIndex < array.size){
         cur->next = &array[nextIndex];
      } else {
         cur->next = nullptr;
      }

      if(nextIndex - i > 1){ // has childs
         cur->child = &array[i + 1];

         Array<FlatteningTemp> subArray = {};
         subArray.data = cur->child;
         subArray.size = nextIndex - i - 1;
         FixFlatteningTempHierarchyRecursive(subArray,level + 1);
      } else {
         cur->child = nullptr;
      }

      i = nextIndex;
   }
}

void FixFlatteningTempHierarchy(Array<FlatteningTemp> array){
   FixFlatteningTempHierarchyRecursive(array,0);
}

Array<FlatteningTemp> HierarchicalFlattening(Graph* graph,Arena* arena){
   Array<FlatteningTemp> result = PushArray<FlatteningTemp>(arena,999);
   Memset(result,{});

   //ArenaMarker marker(arena);
   String str = OutputDotGraph(graph,arena);

   #if 0
   FILE* file = OpenFileAndCreateDirectories("debug/test.dot","w");
   fprintf(file,"%.*s",UNPACK_SS(str));
   fclose(file);
   #endif

   // Flattening
   timeRegion("Flattening part"){
   int index = 0;
   while(1){
      Node* ptr = graph->instances;
      bool noFlatten = true;
      int count = 0;
      while(ptr){
         if(ptr->declaration->type != FUDeclaration::COMPOSITE){
            ptr = ptr->next;
            count += 1;
            continue;
         }

         int nonSpecialSubunits = CountNonSpecialChilds(ptr->declaration->baseCircuit);

         result[index].index = count;
         result[index].decl = ptr->declaration;
         result[index].name = ptr->name;
         result[index].subunitsCount = nonSpecialSubunits;

         Assert(result[index].decl);

         FlattenResult res = FlattenNode(graph,ptr,arena);
         //ptr = res.flatUnitStart;
         ptr = res.flatUnitEnd;

         result[index].flattenedUnits = res.flattenedUnits;

         count += nonSpecialSubunits;
         noFlatten = false;

         #if 0
         String str = OutputDotGraph(graph,arena);
         String dir = PushString(arena,"debug/test_%d.dot",index);
         PushNullByte(arena);
         FILE* file = OpenFileAndCreateDirectories(dir.data,"w");
         fprintf(file,"%.*s",UNPACK_SS(str));
         fclose(file);
         #endif

         index += 1;
      }

      if(noFlatten){
         break;
      }
   }
   result.size = index;
   };

   timeRegion("Assert no loop"){
      AssertNoLoop(graph->edges,arena);
   };

   timeRegion("Sort by edges"){
      SortEdgesByVertices(graph);
   };

   #if 0
   static int nOutputted = 0;
   region(arena){
      String str = OutputDotGraph(graph,arena);
      String dir = PushString(arena,"debug/test_%d.dot",nOutputted++);
      PushNullByte(arena);
      FILE* file = OpenFileAndCreateDirectories(dir.data,"w");
      fprintf(file,"%.*s",UNPACK_SS(str));
      fclose(file);
   };
   exit(0);
   #endif

   // Reorders based on name so that childs are below parent values
   for(int iters = 1; iters < result.size; iters++){
      for(int i = 0; i < result.size - iters; i++){
         FlatteningTemp& t1 = result[i];
         FlatteningTemp& t2 = result[i + 1];

         if(CompareStringOrdered(t1.name,t2.name) < 0){
            SWAP(result[i],result[i + 1]);
         }
      }
   }

   // Calculate number of edges and edge offset
   for(int i = 0; i < result.size; i++){
      String vertName = result[i].name;

      int level = 0;
      for(int i = 0; i < vertName.size; i++){
         if(vertName.data[i] == '.'){
            level += 1;
         }
      }
      result[i].level = level;

      int count = 0;
      int offset = 0;
      int offsetStart = -1;
      FOREACH_LIST(edge,graph->edges){
         offset += 1;

         String toCheck = edge->out.inst->name;
         if(toCheck.size < vertName.size){
            continue;
         }
         toCheck.size = vertName.size;

         if(CompareString(toCheck,vertName)){
            if(offsetStart == -1){
               offsetStart = offset;
            }

            count += 1;
         }
      }
      result[i].edgeCount = count;
      result[i].edgeStart = offsetStart - 1;
   }

   // Fix child and next pointers
   FixFlatteningTempHierarchy(result);

   #if 0
   FOREACH_LIST(ptr,&result[0]){
      printf("%.*s %d\n",UNPACK_SS(ptr->name),ptr->level);
   }
   #endif

   #if 0
   for(FlatteningTemp& t : result){
      printf("%.*s %d\n",UNPACK_SS(t.name),t.level);
   }
   #endif

   #if 0
   for(int i = 0; i < result.size; i++){
      auto d = result[i];
      printf("%.*s Decl:%.*s N:%d +: %d Edge:%d +: %d\n",UNPACK_SS(d.name),UNPACK_SS(d.decl->name),d.index,d.subunitsCount,d.edgeStart,d.edgeCount);
   }
   exit(0);
   #endif

   return result;
}

IndexMapping GetMappingNodeIndexes(MappingNode node,Edge* sub1,Edge* sub2){
   IndexMapping res = {};
   switch(node.type){
   case MappingNode::NODE:{
      printf("Shouldn't be possible\n");
   }break;
   case MappingNode::EDGE:{
      PortEdge e0 = node.edges[0];
      PortEdge e1 = node.edges[1];

      PortInstance i00 = e0.units[0];
      PortInstance i01 = e0.units[1];

      Edge* edge0 = FindEdge(i00.inst,i00.port,i01.inst,i01.port,0);
      Assert(edge0);

      PortInstance i10 = e1.units[0];
      PortInstance i11 = e1.units[1];

      Edge* edge1 = FindEdge(i10.inst,i10.port,i11.inst,i11.port,0);
      Assert(edge1);

      res.index0 = ListIndex(sub1,edge0);
      res.index1 = ListIndex(sub2,edge1);
   }break;
   }

   return res;
}

ConsolidationGraph ParallelBuildConsolidationGraphFromNodes(Array<MappingNode> nodes,Arena* arena){
   struct ThisTask{
      int i;
      Array<BitArray> neighbors;
      Array<MappingNode> nodes;
   };

   auto taskFunction = [](int id,void* arg){
      ThisTask* task = (ThisTask*) arg;

      int i = task->i;
      Array<BitArray> neighbors = task->neighbors;
      Array<MappingNode> nodes = task->nodes;

      MappingNode node1 = nodes[i];

      for(int ii = 0; ii < i; ii++){
         MappingNode node2 = nodes[ii];

         if(MappingConflict(node1,node2)){
            continue;
         }

         neighbors.data[i].Set(ii,1);
         //neighbors.data[ii].Set(i,1);
      }
   };

   Array<BitArray> neighbors = PushArray<BitArray>(arena,nodes.size);

   for(int i = 0; i < nodes.size; i++){
      neighbors[i].Init(arena,nodes.size);
      neighbors[i].Fill(0);
   }

   timeRegion("Parallel edge construction"){
   region(arena){
      Array<ThisTask> data = PushArray<ThisTask>(arena,NumberThreads() * 8);

      Task task = {};
      task.function = taskFunction;
      for(int i = 0; i < nodes.size; i++){
         ThisTask& current = data[i % data.size];

         current.i = i;
         current.neighbors = neighbors;
         current.nodes = nodes;

         task.args = &current;

         while(FullTasks());
         AddTask(task);
      }
      WaitCompletion();
   };
   };

   for(int i = 0; i < nodes.size; i++){
      BitArray array = neighbors.data[i];

      for(int ii = 0; ii < i; ii++){
         int bit = neighbors.data[i].Get(ii);

         if(bit){
            neighbors.data[ii].Set(i,1);
         }
      }
   }

   ConsolidationGraph graph = {};

   graph.nodes = nodes;
   graph.edges = neighbors;
   graph.validNodes.Init(arena,nodes.size);
   graph.validNodes.Fill(1);

   return graph;
}

int CalculateAmountOfMappings(Array<MappingNode> nodes,Arena* arena){
   int count = 0;
   for(int i = 0; i < nodes.size; i++){
      MappingNode node1 = nodes[i];

      for(int ii = 0; ii < i; ii++){
         MappingNode node2 = nodes[ii];

         if(MappingConflict(node1,node2)){
            continue;
         }

         count += 1;
      }
   }
   return count;
}

ConsolidationGraph BuildConsolidationGraphFromNodes(Array<MappingNode> nodes,Arena* arena){
   //return ParallelBuildConsolidationGraphFromNodes(nodes,arena);

   Array<BitArray> neighbors = PushArray<BitArray>(arena,nodes.size);

   for(int i = 0; i < nodes.size; i++){
      neighbors[i].Init(arena,nodes.size);
      neighbors[i].Fill(0);
   }
   for(int i = 0; i < nodes.size; i++){
      MappingNode node1 = nodes[i];

      for(int ii = 0; ii < i; ii++){
         MappingNode node2 = nodes[ii];

         if(MappingConflict(node1,node2)){
            continue;
         }

         neighbors.data[i].Set(ii,1);
      }
   }

   for(int i = 0; i < nodes.size; i++){
      BitArray array = neighbors.data[i];

      for(int ii = 0; ii < i; ii++){
         int bit = neighbors.data[i].Get(ii);

         if(bit){
            neighbors.data[ii].Set(i,1);
         }
      }
   }

   ConsolidationGraph graph = {};

   graph.nodes = nodes;
   graph.edges = neighbors;
   graph.validNodes.Init(arena,nodes.size);
   graph.validNodes.Fill(1);

   return graph;
}

Array<IndexMapping> GetIndexMappingFromClique(ConsolidationGraph clique,Subgraph sub1,Subgraph sub2,Arena* out){
   Array<IndexMapping> res = PushArray<IndexMapping>(out,clique.validNodes.GetNumberBitsSet());

   int index = 0;
   for(int i : clique.validNodes){
      MappingNode node = clique.nodes[i];

      Assert(clique.validNodes.Get(i));

      res[index] = GetMappingNodeIndexes(node,sub1.edges.start,sub2.edges.start);
      index += 1;
   }

   return res;
}

CalculateCliqueAndGetMappingsResult CalculateCliqueAndGetMappings(Subgraph sub1,Subgraph sub2,Arena* arena){
   Array<IndexMapping> mapping = PushArray<IndexMapping>(arena,999);
   Array<int> cliqueTable = PushArray<int>(arena,999);
   Array<IndexMapping> cliqueTableMappings = PushArray<IndexMapping>(arena,999);

   ConsolidationResult result = GenerateConsolidationGraphForSubgraphs(sub1,sub2,{},arena);

   #if 0
   for(int i = 0; i < result.graph.nodes.size; i++){
      MappingNode node = result.graph.nodes[i];

      String rep = Repr(node,arena);
      printf("%.*s\n",UNPACK_SS(rep));
   }
   #endif

   CliqueState* state = InitMaxClique(result.graph,999,arena);
   RunMaxClique(state,arena);
   ConsolidationGraph clique = state->clique;
   Memcpy(cliqueTable.data,state->table.data,state->table.size);

   for(int i = 0; i < state->table.size; i++){
      #if 0
      printf("%d ",state->table[i]);
      #endif

      cliqueTableMappings[i] = GetMappingNodeIndexes(clique.nodes[i],sub1.edges.start,sub2.edges.start);
   }

   int index = 0;
   for(int i : clique.validNodes){
      MappingNode node = clique.nodes[i];

      Assert(clique.validNodes.Get(i));

      mapping[index] = GetMappingNodeIndexes(node,sub1.edges.start,sub2.edges.start);
      index += 1;
   }

   mapping.size = index;
   cliqueTable.size = state->table.size;
   cliqueTableMappings.size = state->table.size;

   CalculateCliqueAndGetMappingsResult res = {};
   res.mappings = mapping;
   res.cliqueTable = cliqueTable;
   res.cliqueTableMappings = cliqueTableMappings;

   return res;
}

Subgraph DefaultSubgraph(Graph* graph){
   Subgraph sub = {};
   sub.nodes.start = graph->instances;
   sub.edges.start = graph->edges;

   return sub;
}

void AddSpecificsToMapping(GraphMapping& mapping,Pool<MappingNode> specifics){
   for(MappingNode* n : specifics){
      Assert(n->type == MappingNode::NODE);

      MergeEdge edge = n->nodes;
      ComplexFUInstance* i0 = edge.instances[0];
      ComplexFUInstance* i1 = edge.instances[1];

      mapping.instanceMap.insert({i1,i0});
      mapping.reverseInstanceMap.insert({i0,i1});
   }
}

Subgraph RemoveInputs(Subgraph sub){
   Subgraph res = {};
   res.nodes.end = res.nodes.end;
   res.edges.end = res.edges.end;

   FOREACH_SUBLIST(ptr,sub.nodes){
      if(ptr->declaration != BasicDeclaration::input){
         res.nodes.start = ptr;
         break;
      }
   }

   if(res.nodes.start == res.nodes.end){
      return (Subgraph){}; // It's empty
   }

   FOREACH_SUBLIST(ptr,sub.edges){
      if(ptr->out.inst->declaration != BasicDeclaration::input){
         res.edges.start = ptr;
         break;
      }
   }

   return res;
}

Subgraph RemoveInputsAndOutputs(Subgraph sub){
   Subgraph res = {};

{  ComplexFUInstance* ptr = sub.nodes.start;
   for(; ptr != nullptr; ptr = ptr->next){
      if(ptr->declaration != BasicDeclaration::input){
         break;
      }
   }
   res.nodes.start = ptr;

   for(; ptr != nullptr; ptr = ptr->next){
      if(ptr->declaration == BasicDeclaration::output){
         break;
      }
   }
   res.nodes.end = ptr;
   if(res.nodes.start == res.nodes.end){
      return (Subgraph){}; // It's empty
   }
}

{  Edge* ptr = sub.edges.start;
   for(; ptr != nullptr; ptr = ptr->next){
      if(ptr->out.inst->declaration != BasicDeclaration::input){
         break;
      }
   }
   res.edges.start = ptr;
   res.edges.end = sub.edges.end;
   #if 0
   for(; ptr != nullptr; ptr = ptr->next){
      if(ptr->in.inst->declaration == BasicDeclaration::output){
         break;
      }
   }
   res.edgesEnd = ptr;
   #endif
}

   return res;
}

int GetMappingBit(ConsolidationGraph& cg,Graph* graph1,Graph* graph2,IndexMapping map){
   for(int i = 0; i < cg.nodes.size; i++){
      IndexMapping inside = GetMappingNodeIndexes(cg.nodes[i],graph1->edges,graph2->edges);

      if(inside == map){
         return i;
      }
   }

   UNREACHABLE;
}

Array<IndexMapping> TransformGenericIntoSpecific(Array<IndexMapping> generic,FlatteningTemp* t1,FlatteningTemp* t2,Arena* perm){
   Array<IndexMapping> res = PushArray<IndexMapping>(perm,generic.size);

   for(int i = 0; i < generic.size; i++){
      IndexMapping gen = generic[i];

      res[i].index0 = gen.index0 + t1->edgeStart;
      res[i].index1 = gen.index1 + t2->edgeStart;
   }

   return res;
}

void Swap(BitArray* arr1,BitArray* arr2){
   Assert(arr1->byteSize == arr2->byteSize);

   for(int i = 0; i < arr1->byteSize; i++){
      SWAP(arr1->memory[i],arr2->memory[i]);
   }
}

void PrintGraphEdges(ConsolidationGraph* graph,Arena* arena){
   for(BitArray& arr : graph->edges){
      ArenaMarker marker(arena);
      String res = arr.PrintRepresentation(arena);

      printf("%.*s\n",UNPACK_SS(res));
   }
}

Array<int> CalculateDegrees(ConsolidationGraph graph,Arena* arena){
   int size = graph.nodes.size;
   Array<BitArray> edges = graph.edges;

   Array<int> degrees = PushArray<int>(arena,size);
   for(int i = 0; i < size; i++){
      degrees[i] = edges[i].GetNumberBitsSet();
   }
   return degrees;
}

MinResult FindSmallest(Array<int> array){
   Assert(array.size);

   int smallest = array[0];
   int smallestIndex = 0;
   for(int i = 1; i < array.size; i++){
      if(array[i] < smallest){
         smallest = array[i];
         smallestIndex = i;
      }
   }

   MinResult res = {};
   res.value = smallest;
   res.index = smallestIndex;
   return res;
}

Array<int> CountValues(Array<int> array,int maximumValue,Arena* out){
   Array<int> res = PushArray<int>(out,maximumValue);
   Memset(res,0);

   for(int i = 0; i < array.size; i++){
      Assert(array[i] < maximumValue);
      res[array[i]] += 1;
   }

   return res;
}

void AssertIndexedArray(Array<int> array,Arena* arena){
   int size = array.size;
   region(arena){
      Array<int> seen = CountValues(array,size,arena);

      bool assertion = true;
      for(int i = 0; i < size; i++){
         if(seen[i] != 1){
            printf("Found a non one: %d on index: %d\n",seen[i],i);
            assertion = false;
         }
      }
      Assert(assertion);
   };
}

Array<int> DegreeReordering(ConsolidationGraph graph,Arena* arena){
   int size = graph.nodes.size;
   Array<int> sortedIndexes = PushArray<int>(arena,size);

   region(arena){
      Array<int> degrees = CalculateDegrees(graph,arena);

      for(int i = 0; i < size; i++){
         MinResult smallest = FindSmallest(degrees);
         int index = smallest.index;

         sortedIndexes[i] = index;
         degrees[index] = size + 1;

         BitArray neighbors = graph.edges[index];
         for(int neigh : neighbors){
            degrees[neigh] -= 1;
         }
      }
   };

   AssertIndexedArray(sortedIndexes,arena);
   return sortedIndexes;
}

Array<int> RadixReordering(ConsolidationGraph graph,Arena* arena){
   int size = graph.nodes.size;
   Array<int> sortedIndexes = PushArray<int>(arena,size);

   region(arena){
      auto indexed = IndexArray(graph.edges,arena);

      auto zero = PushArray<IndexedStruct<BitArray>>(arena,size);
      auto one = PushArray<IndexedStruct<BitArray>>(arena,size);

      for(int i = 0; i < size; i++){
         int zeroIndex = 0;
         int oneIndex = 0;

         for(int ii = 0; ii < size; ii++){
            int val = indexed[ii].data.Get(i);

            if(val){
               one[oneIndex++] = indexed[ii];
            } else {
               zero[zeroIndex++] = indexed[ii];
            }
         }

         int index = 0;
         for(int ii = 0; ii < oneIndex; ii++){
            indexed[index++] = one[ii];
         }
         for(int ii = 0; ii < zeroIndex; ii++){
            indexed[index++] = zero[ii];
         }
         Assert(index == size);
      }

      for(int i = 0; i < size; i++){
         sortedIndexes[i] = indexed[i].index;
      }
   };

   AssertIndexedArray(sortedIndexes,arena);
   return sortedIndexes;
}

void PrintIntTable(Array<int> array,Arena* arena,int digitSize = 0){
   region(arena){
      String repr = PushIntTableRepresentation(arena,array,digitSize);
      printf("%.*s",UNPACK_SS(repr));
   };
}

ColoredOrderingResult ColoredOrdering(ConsolidationGraph graph,Arena* arena){
   int size = graph.nodes.size;
   Array<int> sortedIndexes = PushArray<int>(arena,size);
   int biggestColorFound = 0;

   region(arena){
      Array<int> colors = PushArray<int>(arena,size);
      Memset(colors,-1);

      Array<int> neighColor = PushArray<int>(arena,size);
      PushPtr<int> colorsSeenPush;
      colorsSeenPush.Init(neighColor);

      colors[0] = 0;
      for(int index = 1; index < size; index++){
         BLOCK_REGION(arena);

         BitArray neighbors = graph.edges[index];

         colorsSeenPush.Reset();

         for(int neigh : neighbors){
            int value = colors[neigh];
            if(value == -1){
               continue;
            }

            int* space = colorsSeenPush.Push(1);
            *space = value;
         }

         Array<int> colorsSeen = colorsSeenPush.AsArray();

         if(colorsSeen.size == 0){
            colors[index] = 0;
            continue;
         }

         Array<int> count = CountValues(colorsSeen,index,arena);

         MinResult thisColor = FindSmallest(count);

         if(thisColor.value != 0){
            colors[index] = thisColor.index + 1;
         } else {
            colors[index] = thisColor.index;
         }
         biggestColorFound = std::max(biggestColorFound,colors[index]);
      }

      #if 0
      PrintIntTable(colors,arena,3);
      printf("\n");
      #endif

      PushPtr<int> sortedPush = {};
      sortedPush.Init(sortedIndexes);
      for(int colorValue = 0; colorValue < biggestColorFound + 1; colorValue++){
         for(int i = 0; i < size; i++){
            if(colors[i] == colorValue){
               int* pos = sortedPush.Push(1);
               *pos = i;
            }
         }
      }
      #if 0
      PrintIntTable(sortedIndexes,arena,3);
      printf("\n");
      #endif
   };

   AssertIndexedArray(sortedIndexes,arena);

   ColoredOrderingResult res = {};
   res.order = sortedIndexes;
   res.upperBound = biggestColorFound + 1;

   return res;
}

GenerateMappingNodesResult GenerateMappingNodes(Subgraph accel1,Subgraph accel2,Arena* out,Arena* temp,Array<MappingNode> implicitNodes){
   BLOCK_REGION(temp);

   Hashmap<ComplexFUInstance*,MergeEdge> specificsMapping;
   specificsMapping.Init(temp,1000);
   Pool<MappingNode> specificsAdded = {};
   Hashmap<ComplexFUInstance*,int> nodeExists;
   nodeExists.Init(temp,Size(accel1.nodes) + Size(accel2.nodes));

   FOREACH_SUBLIST(ptr,accel1.nodes){
      nodeExists.InsertIfNotExist(ptr,0);
   }
   FOREACH_SUBLIST(ptr,accel2.nodes){
      nodeExists.InsertIfNotExist(ptr,0);
   }

   // Map outputs
   Node* accel1Output = GetOutputInstance(accel1);
   Node* accel2Output = GetOutputInstance(accel2);
   if(accel1Output && accel2Output){
      MergeEdge node = {};
      node.instances[0] = accel1Output;
      node.instances[1] = accel2Output;

      specificsMapping.Insert(accel1Output,node);
      specificsMapping.Insert(accel2Output,node);

      MappingNode* n = specificsAdded.Alloc();
      n->nodes = node;
      n->type = MappingNode::NODE;
   }

   // Map inputs
   FOREACH_SUBLIST(instA,accel1.nodes){
      if(instA->declaration != BasicDeclaration::input){
         continue;
      }

      FOREACH_SUBLIST(instB,accel2.nodes){
         if(instB->declaration != BasicDeclaration::input){
            continue;
         }

         if(instA->id != instB->id){
            continue;
         }

         MergeEdge node = {};
         node.instances[0] = instA;
         node.instances[1] = instB;

         specificsMapping.Insert(instA,node);
         specificsMapping.Insert(instB,node);

         MappingNode* n = specificsAdded.Alloc();
         n->nodes = node;
         n->type = MappingNode::NODE;
      }
   }

   Byte* nodeStart = MarkArena(out);

   // Check possible edge mapping
   FOREACH_SUBLIST(edge1,accel1.edges){
      if(!(nodeExists.Get(edge1->units[0].inst) && nodeExists.Get(edge1->units[1].inst))){
         continue;
      }

      FOREACH_SUBLIST(edge2,accel2.edges){
         if(!(nodeExists.Get(edge2->units[0].inst) && nodeExists.Get(edge2->units[1].inst))){
            continue;
         }

         // TODO: some nodes do not care about which port is connected (think the inputs for common operations, like multiplication, adders and the likes)
         // Can augment the algorithm further to find more mappings
         if(!(EqualPortMapping(edge1->units[0],edge2->units[0]) &&
            EqualPortMapping(edge1->units[1],edge2->units[1]))){
            continue;
         }

         if(edge1->units[0].inst->declaration->type == FUDeclaration::SPECIAL ||
            edge1->units[1].inst->declaration->type == FUDeclaration::SPECIAL ||
            edge2->units[0].inst->declaration->type == FUDeclaration::SPECIAL ||
            edge2->units[1].inst->declaration->type == FUDeclaration::SPECIAL){
            continue;
         }

         MappingNode node = {};
         node.type = MappingNode::EDGE;

         node.edges[0].units[0] = edge1->units[0];
         node.edges[0].units[1] = edge1->units[1];
         node.edges[1].units[0] = edge2->units[0];
         node.edges[1].units[1] = edge2->units[1];

         // Checks to see if we are in conflict with any specific node.
         MergeEdge* possibleSpecificConflict = specificsMapping.Get(node.edges[0].units[0].inst);
         if(!possibleSpecificConflict) possibleSpecificConflict = specificsMapping.Get(node.edges[0].units[1].inst);
         if(!possibleSpecificConflict) possibleSpecificConflict = specificsMapping.Get(node.edges[1].units[0].inst);
         if(!possibleSpecificConflict) possibleSpecificConflict = specificsMapping.Get(node.edges[1].units[1].inst);

         if(possibleSpecificConflict){
            MappingNode specificNode = {};
            specificNode.type = MappingNode::NODE;
            specificNode.nodes = *possibleSpecificConflict;

            if(MappingConflict(node,specificNode)){
               continue;
            }
         }

         bool continueLoop = false;
         for(MappingNode specificNode : implicitNodes){
            if(specificNode == node){
               continueLoop = true;
               break;
            }

            if(MappingConflict(node,specificNode)){
               continueLoop = true;
               break;
            }
         }

         if(continueLoop){
            continue;
         }

         MappingNode* space = PushStruct<MappingNode>(out);
         *space = node;
      }
   }

   Array<MappingNode> nodes = PointArray<MappingNode>(out,nodeStart);

   GenerateMappingNodesResult res = {};
   res.nodes = nodes;
   res.specificsAdded = specificsAdded;

   return res;
}

int EdgeSize(ConsolidationGraph graph){
   int count = 0;
   for(BitArray& arr : graph.edges){
      count += arr.GetNumberBitsSet();
   }
   return count;
}

ConsolidationResult GenerateConsolidationGraphForSubgraphs(Subgraph accel1,Subgraph accel2,Array<MappingNode> implicitNodes,Arena* arena){
   // Should be temp memory instead of using memory intended for the graph, but since the graph is expected to use a lot of memory and we are technically saving memory using this mapping, no major problem
   Byte* mark = MarkArena(arena);
   Arena nodeSpace = SubArena(arena,Size(accel1.edges) * Size(accel2.edges) * sizeof(MappingNode));

   //TimeIt t1("Node construction");
   GenerateMappingNodesResult nodesResult = GenerateMappingNodes(accel1,accel2,&nodeSpace,arena,implicitNodes);
   //t1.End();

   Array<MappingNode> nodes = nodesResult.nodes;
   //printf("Node Size: %d\n",nodes.size);

   #if 0
   int amountOfMappings = CalculateAmountOfMappings(nodes,arena);
   printf("Amount of edges: %d\n",amountOfMappings);
   #endif

   if(nodes.size > 1000000){
      //printf("Too many nodes, zero graph\n");

      PopMark(arena,mark);
      ConsolidationResult res = {};

      return res;
   }

   PopToSubArena(arena,nodeSpace);

   if(nodes.size == 0){
      ConsolidationResult res = {};
      res.graph = (ConsolidationGraph){};
      res.upperBound = 0;
      res.specificsAdded = nodesResult.specificsAdded;
      return res;
   }

   int upperBound = 999999;
   ConsolidationGraph graph = {};

   #if 0
   timeRegion("Edge construction"){
   graph = BuildConsolidationGraphFromNodes(nodes,arena);
   };
   timeRegion("Node Coloring"){
   region(arena){
      ColoredOrderingResult orderingResult = ColoredOrdering(graph,arena);
      Array<int> newOrder = orderingResult.order;
      upperBound = orderingResult.upperBound;
   };
   };
   #endif

   #if 1
   //timeRegion("Node reordering"){
   region(arena){
      graph = BuildConsolidationGraphFromNodes(nodes,arena);
      //printf("Edge size: %d\n",EdgeSize(graph));
      //exit(0);

      #if 0
      Array<int> newOrder = DegreeReordering(graph,arena);
      upperBound = std::min(Size(accel1.nodes),Size(accel2.nodes));
      #endif
      #if 0
      Array<int> newOrder = RadixReordering(graph,arena);
      upperBound = std::min(Size(accel1.nodes),Size(accel2.nodes));
      #endif
      #if 01
      ColoredOrderingResult orderingResult = ColoredOrdering(graph,arena);
      Array<int> newOrder = orderingResult.order;
      upperBound = orderingResult.upperBound;
      //PrintIntTable(newOrder,arena);
      #endif

      Array<MappingNode> orderedNodes = ReorderMappingNodes(nodes,newOrder,arena);

      for(int i = 0; i < orderedNodes.size; i++){
         nodes[i] = orderedNodes[i];
      }
   };
   //};
   #endif

   #if 1
   //TimeIt t2("Edge construction");
   graph = BuildConsolidationGraphFromNodes(nodes,arena);
   //t2.End();
   #endif

   //printf("Edge size: %d\n",EdgeSize(graph));
   //exit(0);

   ConsolidationResult res = {};
   res.graph = graph;
   res.upperBound = upperBound;
   res.specificsAdded = nodesResult.specificsAdded;

   //printf("Upperbound:%d\n",upperBound);

   return res;
}

ConsolidationResult GenerateConsolidationGraphGivenInitialClique(Subgraph accel1,Subgraph accel2,ConsolidationGraph givenClique,Arena* arena){
   Byte* nodesMark = MarkArena(arena);

   for(int i : givenClique.validNodes){
      MappingNode* node = PushStruct<MappingNode>(arena);
      *node = givenClique.nodes[i];
   }
   Array<MappingNode> implicitNodes = PointArray<MappingNode>(arena,nodesMark);

   ConsolidationResult res = GenerateConsolidationGraphForSubgraphs(accel1,accel2,implicitNodes,arena);
   return res;
}

Array<int> CalculateCoreNumbers(ConsolidationGraph graph,Arena* arena){
   int size = graph.nodes.size;
   Array<BitArray> edges = graph.edges;

   Array<int> core = PushArray<int>(arena,size); // Return graph

   ArenaMarker marker(arena);

   Array<int> degrees = CalculateDegrees(graph,arena);

   #if 0
   region(arena){
      String repr = PushIntTableRepresentation(arena,degrees);
      printf("Degrees:\n %.*s\n",UNPACK_SS(repr));
   };
   #endif

   Array<bool> seen = PushArray<bool>(arena,size);
   Memset(seen,false);

   for(int iterations = 0; iterations < size; iterations++){
      int smallestIndex = -1;
      int smallestDegree = size + 1;

      for(int i = 0; i < degrees.size; i++){
         if(!seen[i] && degrees[i] < smallestDegree){
            smallestDegree = degrees[i];
            smallestIndex = i;
         }
      }

      int index = smallestIndex;

      Assert(index != -1);
      Assert(!seen[index]);
      seen[index] = true;

      core[index] = smallestDegree;

      for(int neigh : edges[index]){
         if(degrees[neigh] > smallestDegree){
            degrees[neigh] -= 1;
         }
      }
   }

   #if 0
   String res = PushIntTableRepresentation(arena,core);
   printf("Core:\n %.*s\n",UNPACK_SS(res));
   #endif

   return core;
}

void SwapCliqueNodes(CliqueState* state,int i0,int i1){
   SWAP(state->clique.nodes[i0],state->clique.nodes[i1]);
   //SWAP(state->clique.edges[i0],state->clique.edges[i1]);
   SWAP(state->table[i0],state->table[i1]);

   Swap(&state->clique.edges[i0],&state->clique.edges[i1]);

   {
   BitArray* valid = &state->clique.validNodes;
   bool temp = valid->Get(i0);
   valid->Set(i0,valid->Get(i1));
   valid->Set(i1,temp);
   }

   for(int i = 0; i < state->clique.edges.size; i++){
      BitArray* valid = &state->clique.edges[i];
      bool temp = valid->Get(i0);
      valid->Set(i0,valid->Get(i1));
      valid->Set(i1,temp);
   }
}

void SwapCliqueUntil(CliqueState* state,int start,int endIncluded){
   int times = endIncluded - start;
   Assert(times > 0);
   for(int i = 0; i < times; i++){
      SwapCliqueNodes(state,start + i,start+i+1);
   }
}

Array<FlattenMapping> GetSpecificMappings(FlatteningTemp* head1,FlatteningTemp* head2,Array<SubgraphMapping> mappings,Arena* arena){
   FOREACH_LIST(t1,head1){
      //printf("%.*s %d\n",UNPACK_SS(t1->name),t1->level);
      t1->flag = 0;
   }
   FOREACH_LIST(t2,head2){
      //printf("%.*s %d\n",UNPACK_SS(t2->name),t2->level);
      t2->flag = 0;
   }

   Byte* mark = MarkArena(arena);

   bool mappedOne = true;
   while(mappedOne){
      mappedOne = false;

      int maxWeight = 0;
      FOREACH_LIST(t1,head1){
         FOREACH_LIST(t2,head2){
            if(t1->flag || t2->flag){
               continue;
            }

            for(SubgraphMapping& sub : mappings){
               //printf("Sub: %.*s - %.*s %d\n",UNPACK_SS(sub.decl1->name),UNPACK_SS(sub.decl2->name),sub.edgeMappings.size);
               if(sub.decl1 == t1->decl && sub.decl2 == t2->decl){
                  int weight = sub.edgeMappings.size;

                  //printf("%.*s - %.*s = %d\n",UNPACK_SS(sub.decl1->name),UNPACK_SS(sub.decl2->name),weight);
                  maxWeight = std::max(maxWeight,weight);
               }
            }
         }
      }
      if(maxWeight == 0){
         break;
      }

      FOREACH_LIST(t1,head1){
         FOREACH_LIST(t2,head2){
            if(t1->flag || t2->flag){
               continue;
            }
            for(SubgraphMapping& sub : mappings){
               if(sub.decl1 == t1->decl && sub.decl2 == t2->decl){
                  int weight = sub.edgeMappings.size;

                  #if 0
                  if(weight != maxWeight){
                     continue;
                  }
                  #else
                  if(weight != maxWeight || maxWeight == 0){
                     continue;
                  }
                  #endif

                  FlattenMapping* map = PushStruct<FlattenMapping>(arena);
                  map->t1 = t1;
                  map->t2 = t2;
                  map->mapping = &sub;
                  map->weight = weight;
                  t1->flag = 1;
                  t2->flag = 1;

                  mappedOne = true;
                  goto endLoop;
               }
            }
         }
      }
endLoop:;
   }

   Array<FlattenMapping> res = PointArray<FlattenMapping>(arena,mark);

   return res;
}

#if 0
Array<IndexMapping> ParallelHierarchicalHeuristic(Graph* graph1,Graph* graph2,FlatteningTemp* t1,FlatteningTemp* t2,PushPtr<SubgraphMapping>& allMappings,Arena* arena){
   Hashmap<FUDeclaration*,FlatteningTemp*> decl1 = {};
   decl1.Init(arena,999);

   Hashmap<FUDeclaration*,FlatteningTemp*> decl2 = {};
   decl2.Init(arena,999);

   FOREACH_LIST(ptr,t1->child){
      decl1.InsertIfNotExist(ptr->decl,ptr);
   }

   FOREACH_LIST(ptr,t2->child){
      decl2.InsertIfNotExist(ptr->decl,ptr);
   }

   int numberMappings = 0;
   // Get any mapping missing from allMappings
   for(auto pair1 : decl1){
      for(auto pair2 : decl2){
         if(pair1.first == pair2.first){
            continue;
         }

         bool doContinue = false;
         for(SubgraphMapping& map : allMappings.AsArray()){
            if(map.decl1 == pair1.first && map.decl2 == pair2.first){
               doContinue = true;
               break;
            }
         }
         if(doContinue){
            continue;
         }

         numberMappings += 1;
      }
   }
   printf("Mappings: %d\n",numberMappings);

   exit(0);

   // Get any mapping missing from allMappings
   for(auto pair1 : decl1){
      for(auto pair2 : decl2){
         if(pair1.first == pair2.first){
            continue;
         }

         bool doContinue = false;
         for(SubgraphMapping& map : allMappings.AsArray()){
            if(map.decl1 == pair1.first && map.decl2 == pair2.first){
               doContinue = true;
               break;
            }
         }
         if(doContinue){
            continue;
         }

         FUDeclaration* t1 = pair1.first;
         FUDeclaration* t2 = pair2.first;

         Array<IndexMapping> indexMappings = ParallelHierarchicalHeuristic(graph1,graph2,pair1.second,pair2.second,allMappings,arena);

         SubgraphMapping* mapping = allMappings.Push(1);

         mapping->decl1 = t1;
         mapping->decl2 = t2;
         mapping->edgeMappings = indexMappings;
      }
   }

   Array<FlattenMapping> specificMappings = GetSpecificMappings(t1->child,t2->child,allMappings.AsArray(),arena);

   for(int i = 0; i < specificMappings.size; i++){
      for(int j = i + 1; j < specificMappings.size; j++){
         if(specificMappings[j].t1->index > specificMappings[i].t1->index){
            SWAP(specificMappings[i],specificMappings[j]);
         }
      }
   }

   TIME_IT("Finding heuristic");

   #if 01
   printf("=== Finding heuristic for %.*s [%.*s] %.*s [%.*s] ===\n",UNPACK_SS(t1->name),UNPACK_SS(t1->decl->name),UNPACK_SS(t2->name),UNPACK_SS(t2->decl->name));

   for(FlattenMapping& mapping : specificMappings){
      printf("Mapping %.*s to %.*s\n",UNPACK_SS(mapping.t1->name),UNPACK_SS(mapping.t2->name));
   }
   #endif

   int specificSize = 0;
   for(FlattenMapping& m : specificMappings){
      specificSize += m.mapping->edgeMappings.size;
   }

   printf("Specific size: %d\n",specificSize);

   Array<MappingNode> implicitNodes = PushArray<MappingNode>(arena,specificSize);
   PushPtr<MappingNode> nodesPush = {};
   nodesPush.Init(implicitNodes);

   for(FlattenMapping& m : specificMappings){
      Subgraph sub1 = {};
      sub1.nodes = SimpleSublist(ListGet(graph1->instances,m.t1->index),m.t1->subunitsCount);
      sub1.edges = SimpleSublist(ListGet(graph1->edges,m.t1->edgeStart),m.t1->edgeCount);

      Subgraph sub2 = {};
      sub2.nodes = SimpleSublist(ListGet(graph2->instances,m.t2->index),m.t2->subunitsCount);
      sub2.edges = SimpleSublist(ListGet(graph2->edges,m.t2->edgeStart),m.t2->edgeCount);

      ArenaMarker marker(arena);
      Array<IndexMapping> graphMappings = m.mapping->edgeMappings;

      for(IndexMapping map : graphMappings){
         MappingNode* node = nodesPush.Push(1);

         node->type = MappingNode::EDGE;
         node->edges[0] = ListGet(sub1.edges.start,map.index0)->edge;
         node->edges[1] = ListGet(sub2.edges.start,map.index1)->edge;
         //printf("G: %d %d\n",map.index0,map.index1);
      }
   }

   Subgraph sub1 = {};
   sub1.nodes = SimpleSublist(ListGet(graph1->instances,t1->index),t1->subunitsCount);
   sub1.edges = SimpleSublist(ListGet(graph1->edges,t1->edgeStart),t1->edgeCount);

   Subgraph sub2 = {};
   sub2.nodes = SimpleSublist(ListGet(graph2->instances,t2->index),t2->subunitsCount);
   sub2.edges = SimpleSublist(ListGet(graph2->edges,t2->edgeStart),t2->edgeCount);

   ConsolidationResult result = GenerateConsolidationGraphForSubgraphs(sub1,sub2,implicitNodes,arena);
   ConsolidationGraph graph = result.graph;

   #if 01
   CliqueState* state = InitMaxClique(graph,result.upperBound,arena);
   RunMaxClique(state,arena);
   ConsolidationGraph extraClique = state->clique;
   #else
   TimeIt pc("Parallel clique");
   ConsolidationGraph extraClique = ParallelMaxClique(graph,result.upperBound,arena);
   pc.End();
   #endif

   Assert(IsClique(extraClique).result);

   #if 0
   OutputDotGraph(sub1,StaticFormat("debug/sub_%.*s_sub.dot",UNPACK_SS(t1->name)),arena);
   OutputDotGraph(sub2,StaticFormat("debug/sub_%.*s_sub.dot",UNPACK_SS(t2->name)),arena);

   printf("Mapping nodes:\n");
   for(MappingNode& node : implicitNodes){
      region(arena){
         String repr = Repr(node,arena);
         printf("%.*s\n",UNPACK_SS(repr));
      };
   }
   printf("\n\n");
   #endif

   #if 0
   printf("\n");
   printf("Nodes from children: %d\n",implicitNodes.size);
   printf("Extra CG size: %d\n",graph.nodes.size);

   printf("Extra clique size: %d\n",extraClique.validNodes.GetNumberBitsSet());
   #endif

   Array<IndexMapping> cliqueMappings = GetIndexMappingFromClique(extraClique,sub1,sub2,arena);

   Array<IndexMapping> fullMappings = PushArray<IndexMapping>(arena,specificSize + extraClique.validNodes.GetNumberBitsSet());
   PushPtr<IndexMapping> pushPtr = {};
   pushPtr.Init(fullMappings);

   region(arena){
      for(FlattenMapping& m : specificMappings){
         Subgraph sub1 = {};
         sub1.nodes = SimpleSublist(ListGet(graph1->instances,m.t1->index),m.t1->subunitsCount);
         sub1.edges = SimpleSublist(ListGet(graph1->edges,m.t1->edgeStart),m.t1->edgeCount);

         Subgraph sub2 = {};
         sub2.nodes = SimpleSublist(ListGet(graph2->instances,m.t2->index),m.t2->subunitsCount);
         sub2.edges = SimpleSublist(ListGet(graph2->edges,m.t2->edgeStart),m.t2->edgeCount);

         //Array<IndexMapping> specific = TransformGenericIntoSpecific(m.mapping->edgeMappings,m.t1,m.t2,arena);
         for(IndexMapping mapping : m.mapping->edgeMappings){
            IndexMapping specific = mapping;
            specific.index0 += m.t1->edgeStart - t1->edgeStart;
            specific.index1 += m.t2->edgeStart - t2->edgeStart;

            pushPtr.PushValue(specific);
         }
      }
   };
   pushPtr.Push(cliqueMappings);

   #if 0
   for(int i = 0; i < fullMappings.size; i++){
      IndexMapping& m = fullMappings[i];
      Edge* edge0 = ListGet(sub1.edges.start,m.index0);
      Edge* edge1 = ListGet(sub2.edges.start,m.index1);

      printf("%d %d\n",m.index0,m.index1);
   }

   for(int i = 0; i < fullMappings.size; i++){
      IndexMapping& m = fullMappings[i];
      Edge* edge0 = ListGet(sub1.edges.start,m.index0);
      Edge* edge1 = ListGet(sub2.edges.start,m.index1);

      region(arena){
         String str1 = Repr(edge0->edge,GRAPH_DOT_FORMAT_NAME,arena);
         String str2 = Repr(edge1->edge,GRAPH_DOT_FORMAT_NAME,arena);

         if(i < specificSize){
            printf("From specific: %.*s // %.*s\n",UNPACK_SS(str1),UNPACK_SS(str2));
         } else {
            printf("From extra   : %.*s // %.*s\n",UNPACK_SS(str1),UNPACK_SS(str2));
         }
      };
   }
   printf("\n");

   printf("Mapping result: \n");
   for(int i = 0; i < fullMappings.size; i++){
      IndexMapping& m = fullMappings[i];

      printf("%d %d\n",m.index0,m.index1);
   }
   #endif

   printf("Size found: %d\n",fullMappings.size);

   return fullMappings;
}
#endif

Array<IndexMapping> HierarchicalHeuristic(Graph* graph1,Graph* graph2,FlatteningTemp* t1,FlatteningTemp* t2,PushPtr<SubgraphMapping>& allMappings,Arena* arena){
   Hashmap<FUDeclaration*,FlatteningTemp*> decl1 = {};
   decl1.Init(arena,999);

   Hashmap<FUDeclaration*,FlatteningTemp*> decl2 = {};
   decl2.Init(arena,999);

   FOREACH_LIST(ptr,t1->child){
      decl1.InsertIfNotExist(ptr->decl,ptr);
   }

   FOREACH_LIST(ptr,t2->child){
      decl2.InsertIfNotExist(ptr->decl,ptr);
   }

   // Get any mapping missing from allMappings
   for(auto pair1 : decl1){
      for(auto pair2 : decl2){
         if(pair1.first == pair2.first){
            continue;
         }

         bool doContinue = false;
         for(SubgraphMapping& map : allMappings.AsArray()){
            if(map.decl1 == pair1.first && map.decl2 == pair2.first){
               doContinue = true;
               break;
            }
         }
         if(doContinue){
            continue;
         }

         FUDeclaration* t1 = pair1.first;
         FUDeclaration* t2 = pair2.first;

         #if 0
         Subgraph sub1 = {};
         sub1.nodes = SimpleSublist(ListGet(graph1->instances,pair1.second->index),pair1.second->subunitsCount);
         sub1.edges = SimpleSublist(ListGet(graph1->edges,pair1.second->edgeStart),pair1.second->edgeCount);

         Subgraph sub2 = {};
         sub2.nodes = SimpleSublist(ListGet(graph2->instances,pair2.second->index),pair2.second->subunitsCount);
         sub2.edges = SimpleSublist(ListGet(graph2->edges,pair2.second->edgeStart),pair2.second->edgeCount);

         OutputDotGraph(sub1,StaticFormat("debug/sub_%.*s_sub.dot",UNPACK_SS(t1->name)),arena);
         OutputDotGraph(sub2,StaticFormat("debug/sub_%.*s_sub.dot",UNPACK_SS(t2->name)),arena);
         #endif

         //CalculateCliqueAndGetMappingsResult res = HierarchicalHeuristic(sub1,sub2,t1,t2,arena); // Should be, in theory,a pretty good aproximation
         //Array<IndexMapping> indexMappings = res.mappings;
         Array<IndexMapping> indexMappings = HierarchicalHeuristic(graph1,graph2,pair1.second,pair2.second,allMappings,arena);

         SubgraphMapping* mapping = allMappings.Push(1);

         mapping->decl1 = t1;
         mapping->decl2 = t2;
         mapping->edgeMappings = indexMappings;
      }
   }

   Array<FlattenMapping> specificMappings = GetSpecificMappings(t1->child,t2->child,allMappings.AsArray(),arena);

   for(int i = 0; i < specificMappings.size; i++){
      for(int j = i + 1; j < specificMappings.size; j++){
         if(specificMappings[j].t1->index > specificMappings[i].t1->index){
            SWAP(specificMappings[i],specificMappings[j]);
         }
      }
   }

   TIME_IT("Finding heuristic");

   #if 01
   printf("=== Finding heuristic for %.*s [%.*s] %.*s [%.*s] ===\n",UNPACK_SS(t1->name),UNPACK_SS(t1->decl->name),UNPACK_SS(t2->name),UNPACK_SS(t2->decl->name));

   for(FlattenMapping& mapping : specificMappings){
      printf("Mapping %.*s to %.*s\n",UNPACK_SS(mapping.t1->name),UNPACK_SS(mapping.t2->name));
   }
   #endif

   int specificSize = 0;
   for(FlattenMapping& m : specificMappings){
      specificSize += m.mapping->edgeMappings.size;
   }

   printf("Specific size: %d\n",specificSize);

   Array<MappingNode> implicitNodes = PushArray<MappingNode>(arena,specificSize);
   PushPtr<MappingNode> nodesPush = {};
   nodesPush.Init(implicitNodes);

   for(FlattenMapping& m : specificMappings){
      Subgraph sub1 = {};
      sub1.nodes = SimpleSublist(ListGet(graph1->instances,m.t1->index),m.t1->subunitsCount);
      sub1.edges = SimpleSublist(ListGet(graph1->edges,m.t1->edgeStart),m.t1->edgeCount);

      Subgraph sub2 = {};
      sub2.nodes = SimpleSublist(ListGet(graph2->instances,m.t2->index),m.t2->subunitsCount);
      sub2.edges = SimpleSublist(ListGet(graph2->edges,m.t2->edgeStart),m.t2->edgeCount);

      ArenaMarker marker(arena);
      Array<IndexMapping> graphMappings = m.mapping->edgeMappings;

      for(IndexMapping map : graphMappings){
         MappingNode* node = nodesPush.Push(1);

         node->type = MappingNode::EDGE;
         node->edges[0] = ListGet(sub1.edges.start,map.index0)->edge;
         node->edges[1] = ListGet(sub2.edges.start,map.index1)->edge;
         //printf("G: %d %d\n",map.index0,map.index1);
      }
   }

   if(CompareString(t1->decl->name,"AES")){
      exit(0);
   }

   Subgraph sub1 = {};
   sub1.nodes = SimpleSublist(ListGet(graph1->instances,t1->index),t1->subunitsCount);
   sub1.edges = SimpleSublist(ListGet(graph1->edges,t1->edgeStart),t1->edgeCount);

   Subgraph sub2 = {};
   sub2.nodes = SimpleSublist(ListGet(graph2->instances,t2->index),t2->subunitsCount);
   sub2.edges = SimpleSublist(ListGet(graph2->edges,t2->edgeStart),t2->edgeCount);

   ConsolidationResult result = GenerateConsolidationGraphForSubgraphs(sub1,sub2,implicitNodes,arena);
   ConsolidationGraph graph = result.graph;

   #if 01
   CliqueState* state = InitMaxClique(graph,result.upperBound,arena);
   RunMaxClique(state,arena);
   ConsolidationGraph extraClique = state->clique;
   #else
   TimeIt pc("Parallel clique");
   ConsolidationGraph extraClique = ParallelMaxClique(graph,result.upperBound,arena);
   pc.End();
   #endif

   Assert(IsClique(extraClique).result);

   #if 0
   OutputDotGraph(sub1,StaticFormat("debug/sub_%.*s_sub.dot",UNPACK_SS(t1->name)),arena);
   OutputDotGraph(sub2,StaticFormat("debug/sub_%.*s_sub.dot",UNPACK_SS(t2->name)),arena);

   printf("Mapping nodes:\n");
   for(MappingNode& node : implicitNodes){
      region(arena){
         String repr = Repr(node,arena);
         printf("%.*s\n",UNPACK_SS(repr));
      };
   }
   printf("\n\n");
   #endif

   #if 0
   printf("\n");
   printf("Nodes from children: %d\n",implicitNodes.size);
   printf("Extra CG size: %d\n",graph.nodes.size);

   printf("Extra clique size: %d\n",extraClique.validNodes.GetNumberBitsSet());
   #endif

   Array<IndexMapping> cliqueMappings = GetIndexMappingFromClique(extraClique,sub1,sub2,arena);

   Array<IndexMapping> fullMappings = PushArray<IndexMapping>(arena,specificSize + extraClique.validNodes.GetNumberBitsSet());
   PushPtr<IndexMapping> pushPtr = {};
   pushPtr.Init(fullMappings);

   region(arena){
      for(FlattenMapping& m : specificMappings){
         Subgraph sub1 = {};
         sub1.nodes = SimpleSublist(ListGet(graph1->instances,m.t1->index),m.t1->subunitsCount);
         sub1.edges = SimpleSublist(ListGet(graph1->edges,m.t1->edgeStart),m.t1->edgeCount);

         Subgraph sub2 = {};
         sub2.nodes = SimpleSublist(ListGet(graph2->instances,m.t2->index),m.t2->subunitsCount);
         sub2.edges = SimpleSublist(ListGet(graph2->edges,m.t2->edgeStart),m.t2->edgeCount);

         //Array<IndexMapping> specific = TransformGenericIntoSpecific(m.mapping->edgeMappings,m.t1,m.t2,arena);
         for(IndexMapping mapping : m.mapping->edgeMappings){
            IndexMapping specific = mapping;
            specific.index0 += m.t1->edgeStart - t1->edgeStart;
            specific.index1 += m.t2->edgeStart - t2->edgeStart;

            pushPtr.PushValue(specific);
         }
      }
   };
   pushPtr.Push(cliqueMappings);

   #if 0
   for(int i = 0; i < fullMappings.size; i++){
      IndexMapping& m = fullMappings[i];
      Edge* edge0 = ListGet(sub1.edges.start,m.index0);
      Edge* edge1 = ListGet(sub2.edges.start,m.index1);

      printf("%d %d\n",m.index0,m.index1);
   }

   for(int i = 0; i < fullMappings.size; i++){
      IndexMapping& m = fullMappings[i];
      Edge* edge0 = ListGet(sub1.edges.start,m.index0);
      Edge* edge1 = ListGet(sub2.edges.start,m.index1);

      region(arena){
         String str1 = Repr(edge0->edge,GRAPH_DOT_FORMAT_NAME,arena);
         String str2 = Repr(edge1->edge,GRAPH_DOT_FORMAT_NAME,arena);

         if(i < specificSize){
            printf("From specific: %.*s // %.*s\n",UNPACK_SS(str1),UNPACK_SS(str2));
         } else {
            printf("From extra   : %.*s // %.*s\n",UNPACK_SS(str1),UNPACK_SS(str2));
         }
      };
   }
   printf("\n");

   printf("Mapping result: \n");
   for(int i = 0; i < fullMappings.size; i++){
      IndexMapping& m = fullMappings[i];

      printf("%d %d\n",m.index0,m.index1);
   }
   #endif

   printf("Size found: %d\n",fullMappings.size);

   return fullMappings;
}

void PrintGraphInfo(Graph* graph){
   printf("N nodes: %d\n",Size(graph->instances));
   printf("N edges: %d\n",Size(graph->edges));
}

MergeGraphResult HierarchicalHeuristic(Versat* versat,FUDeclaration* decl1,FUDeclaration* decl2,String name){
   Arena* arena = &versat->temp;
   ArenaMarker marker(arena);

   Accelerator* accel1 = decl1->baseCircuit;
   Accelerator* accel2 = decl2->baseCircuit;

   Graph* graph1 = accel1;
   Array<FlatteningTemp> res1;
   timeRegion("Flattening graph1"){
      res1 = HierarchicalFlattening(graph1,arena);
   };

   FlatteningTemp graph1All = {};
   graph1All.index = 0;
   graph1All.parent = decl1;
   graph1All.decl = decl1;
   graph1All.name = decl1->name;
   graph1All.subunitsCount = graph1->numberInstances;
   graph1All.edgeCount = graph1->numberEdges;
   graph1All.child = &res1[0];

   #if 0
   printf("Graph1: \n");
   PrintGraphInfo(graph1);
   #endif

   Graph* graph2 = accel2;
   Array<FlatteningTemp> res2;
   timeRegion("Flattening graph2"){
      res2 = HierarchicalFlattening(graph2,arena);
   };

   FlatteningTemp graph2All = {};
   graph2All.index = 0;
   graph2All.parent = decl2;
   graph2All.decl = decl2;
   graph2All.name = decl2->name;
   graph2All.subunitsCount = graph2->numberInstances;
   graph2All.edgeCount = graph2->numberEdges;
   graph2All.child = &res2[0];

   #if 0
   printf("Graph2: \n");
   PrintGraphInfo(graph2);

   printf("Finding a clique for %.*s\n\n",UNPACK_SS(name));
   #endif

   Array<SubgraphMapping> allsubgraphMappingsBuffer = PushArray<SubgraphMapping>(arena,99);
   Memset(allsubgraphMappingsBuffer,{});
   PushPtr<SubgraphMapping> allsubgraphMappings;
   allsubgraphMappings.Init(allsubgraphMappingsBuffer);

   Array<IndexMapping> heuristicMapping = HierarchicalHeuristic(graph1,graph2,&graph1All,&graph2All,allsubgraphMappings,arena);

   heuristicMapping = TransformGenericIntoSpecific(heuristicMapping,&graph1All,&graph2All,arena);

   printf("%d\n",heuristicMapping.size);
   GraphMapping mappingRes = {};

   for(IndexMapping& m : heuristicMapping){
      Edge* edge0 = ListGet(graph1->edges,m.index0);
      Edge* edge1 = ListGet(graph2->edges,m.index1);

      #if 0
      region(arena){
         String str1 = Repr(edge0->edge,GRAPH_DOT_FORMAT_NAME,arena);
         String str2 = Repr(edge1->edge,GRAPH_DOT_FORMAT_NAME,arena);
         printf("Going to insert %.*s // %.*s\n",UNPACK_SS(str1),UNPACK_SS(str2));
      };
      FlushStdout();
      #endif

      InsertMapping(mappingRes,edge0->edge,edge1->edge);
   }

   printf("%d\n",mappingRes.edgeMap.size());

   return MergeGraph(versat,graph1,graph2,mappingRes,STRING("Test"));
}

MergeGraphResult HierarchicalMergeAcceleratorsFullClique(Versat* versat,Accelerator* accel1,Accelerator* accel2,String name){
   Arena* arena = &versat->temp;
   ArenaMarker marker(arena);

   Graph* graph1 = accel1;
   Array<FlatteningTemp> res1 = HierarchicalFlattening(graph1,arena);

   Graph* graph2 = accel2;
   Array<FlatteningTemp> res2 = HierarchicalFlattening(graph2,arena);

   // Looks good until here

   Subgraph sub1 = DefaultSubgraph(graph1);
   Subgraph sub2 = DefaultSubgraph(graph2);

   TimeIt time0("Building CG");
   ConsolidationResult result = GenerateConsolidationGraphForSubgraphs(sub1,sub2,{},arena);
   ConsolidationGraph graph = result.graph;
   time0.End();

   GraphMapping mapping = {};

   printf("Full clique:\n");

   TimeIt time1("Finding clique");
   #if 1
   CliqueState cliqueState = MaxClique(graph,result.upperBound,arena);
   ConsolidationGraph clique = cliqueState.clique;
   #else
   ConsolidationGraph clique = ParallelMaxClique(graph,result.upperBound,arena);
   #endif
   time1.End();

   printf("Size: %d\n",graph.nodes.size);
   printf("Size of clique: %d\n",clique.validNodes.GetNumberBitsSet());

   AddCliqueToMapping(mapping,clique);
   AddSpecificsToMapping(mapping,result.specificsAdded);

   printf("%d\n",mapping.edgeMap.size());

   return MergeGraph(versat,graph1,graph2,mapping,STRING("Test"));
}

#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

//Array<IndexMapping> ParallelHierarchicalHeuristic(Graph* graph1,Graph* graph2,Array<FlatteningTemp> flattening1,Array<FlatteningTemp> flattening2,Arena* arena){
void ParallelHierarchicalHeuristicTask(int i,void* arg){
   HeuristicState* state = (HeuristicState*) arg;

   Graph* graph1 = state->graph1;
   Graph* graph2 = state->graph2;
   FlatteningTemp* t1 = state->t1;
   FlatteningTemp* t2 = state->t2;

   Arena arenaInst = InitArena(Gigabyte(16));
   Arena* arena = &arenaInst;

   Array<SubgraphMapping> mappings = state->mappings;
   mappings.size = *state->inserted;

   Array<FlattenMapping> specificMappings = GetSpecificMappings(t1->child,t2->child,mappings,arena);

   for(int i = 0; i < specificMappings.size; i++){
      for(int j = i + 1; j < specificMappings.size; j++){
         if(specificMappings[j].t1->index > specificMappings[i].t1->index){
            SWAP(specificMappings[i],specificMappings[j]);
         }
      }
   }

   #if 1
   TIME_IT("Finding heuristic");

   printf("=== Finding heuristic for %.*s [%.*s] %.*s [%.*s] ===\n",UNPACK_SS(t1->name),UNPACK_SS(t1->decl->name),UNPACK_SS(t2->name),UNPACK_SS(t2->decl->name));

   for(FlattenMapping& mapping : specificMappings){
      printf("Mapping %.*s to %.*s\n",UNPACK_SS(mapping.t1->name),UNPACK_SS(mapping.t2->name));
   }
   #endif

   int specificSize = 0;
   for(FlattenMapping& m : specificMappings){
      specificSize += m.mapping->edgeMappings.size;
   }

   //printf("Specific size: %d\n",specificSize);

   Array<MappingNode> implicitNodes = PushArray<MappingNode>(arena,specificSize);
   PushPtr<MappingNode> nodesPush = {};
   nodesPush.Init(implicitNodes);

   for(FlattenMapping& m : specificMappings){
      Subgraph sub1 = {};
      sub1.nodes = SimpleSublist(ListGet(graph1->instances,m.t1->index),m.t1->subunitsCount);
      sub1.edges = SimpleSublist(ListGet(graph1->edges,m.t1->edgeStart),m.t1->edgeCount);

      Subgraph sub2 = {};
      sub2.nodes = SimpleSublist(ListGet(graph2->instances,m.t2->index),m.t2->subunitsCount);
      sub2.edges = SimpleSublist(ListGet(graph2->edges,m.t2->edgeStart),m.t2->edgeCount);

      ArenaMarker marker(arena);
      Array<IndexMapping> graphMappings = m.mapping->edgeMappings;

      for(IndexMapping map : graphMappings){
         MappingNode* node = nodesPush.Push(1);

         node->type = MappingNode::EDGE;
         node->edges[0] = ListGet(sub1.edges.start,map.index0)->edge;
         node->edges[1] = ListGet(sub2.edges.start,map.index1)->edge;
      }
   }

   Subgraph sub1 = {};
   sub1.nodes = SimpleSublist(ListGet(graph1->instances,t1->index),t1->subunitsCount);
   sub1.edges = SimpleSublist(ListGet(graph1->edges,t1->edgeStart),t1->edgeCount);

   Subgraph sub2 = {};
   sub2.nodes = SimpleSublist(ListGet(graph2->instances,t2->index),t2->subunitsCount);
   sub2.edges = SimpleSublist(ListGet(graph2->edges,t2->edgeStart),t2->edgeCount);

   ConsolidationResult result = GenerateConsolidationGraphForSubgraphs(sub1,sub2,implicitNodes,arena);
   ConsolidationGraph graph = result.graph;

   #if 01
   CliqueState* cliqueState = InitMaxClique(graph,result.upperBound,arena);
   RunMaxClique(cliqueState,arena);
   ConsolidationGraph extraClique = cliqueState->clique;
   #else
   TimeIt pc("Parallel clique");
   ConsolidationGraph extraClique = ParallelMaxClique(graph,result.upperBound,arena);
   pc.End();
   #endif

   Assert(IsClique(extraClique).result);

   Array<IndexMapping> cliqueMappings = GetIndexMappingFromClique(extraClique,sub1,sub2,arena);

   pthread_mutex_lock(state->mappingMutex);
   Array<IndexMapping> fullMappings = PushArray<IndexMapping>(state->resultArena,specificSize + extraClique.validNodes.GetNumberBitsSet());
   pthread_mutex_unlock(state->mappingMutex);

   PushPtr<IndexMapping> pushPtr = {};
   pushPtr.Init(fullMappings);

   region(arena){
      for(FlattenMapping& m : specificMappings){
         Subgraph sub1 = {};
         sub1.nodes = SimpleSublist(ListGet(graph1->instances,m.t1->index),m.t1->subunitsCount);
         sub1.edges = SimpleSublist(ListGet(graph1->edges,m.t1->edgeStart),m.t1->edgeCount);

         Subgraph sub2 = {};
         sub2.nodes = SimpleSublist(ListGet(graph2->instances,m.t2->index),m.t2->subunitsCount);
         sub2.edges = SimpleSublist(ListGet(graph2->edges,m.t2->edgeStart),m.t2->edgeCount);

         //Array<IndexMapping> specific = TransformGenericIntoSpecific(m.mapping->edgeMappings,m.t1,m.t2,arena);
         for(IndexMapping mapping : m.mapping->edgeMappings){
            IndexMapping specific = mapping;
            specific.index0 += m.t1->edgeStart - t1->edgeStart;
            specific.index1 += m.t2->edgeStart - t2->edgeStart;

            pushPtr.PushValue(specific);
         }
      }
   };
   pushPtr.Push(cliqueMappings);

   //printf("Size found: %d\n",fullMappings.size);

   pthread_mutex_lock(state->mappingMutex);
   int finalPos = *state->inserted;
   state->mappings[finalPos].decl2 = state->t2->decl;
   state->mappings[finalPos].decl1 = state->t1->decl;
   state->mappings[finalPos].edgeMappings = fullMappings;
   *state->inserted += 1;

   pthread_mutex_unlock(state->mappingMutex);

   //printf("Finished: %.*s - %.*s (%d)\n",UNPACK_SS(state->t1->decl->name),UNPACK_SS(state->t2->decl->name),finalPos);

   state->finished = true;
   Signal(state->cond);
   Free(arena);
}

bool CheckIfMappingIsPossible(FlatteningTemp* head1,FlatteningTemp* head2,Array<SubgraphMapping> mappings){
   FOREACH_LIST(t1,head1->child){
      FOREACH_LIST(t2,head2->child){
         bool exists = false;
         for(SubgraphMapping& sub : mappings){
            if(sub.decl1 == t1->decl && sub.decl2 == t2->decl){
               exists = true;
               break;
            }
         }

         if(!exists){
            //printf("Cannot do: %.*s %.*s because cannot find %.*s %.*s (%d)\n",UNPACK_SS(head1->decl->name),UNPACK_SS(head2->decl->name),UNPACK_SS(t1->decl->name),UNPACK_SS(t2->decl->name),mappings.size);
            return false;
         }
      }
   }

   return true;
}

MergeGraphResult ParallelHierarchicalHeuristic(Versat* versat,FUDeclaration* decl1,FUDeclaration* decl2,String name){
   Arena* arena = &versat->temp;
   ArenaMarker marker(arena);

   Accelerator* accel1 = decl1->baseCircuit;
   Accelerator* accel2 = decl2->baseCircuit;

   Graph* graph1 = accel1;
   Array<FlatteningTemp> flattening1;
   timeRegion("Flattening graph1"){
      flattening1 = HierarchicalFlattening(graph1,arena);
   };

   Graph* graph2 = accel2;
   Array<FlatteningTemp> flattening2;
   timeRegion("Flattening graph2"){
      flattening2 = HierarchicalFlattening(graph2,arena);
   };

   Arena resultInst = InitArena(Megabyte(1));
   Arena* resultArena = &resultInst;

   Array<HeuristicState> mappings = PushArray<HeuristicState>(arena,99);

   int numberMappings = 0;
   for(auto& t1 : flattening1){
      for(auto& t2 : flattening2){
         if(t1.level != t2.level){
            continue;
         }

         FUDeclaration* decl1 = t1.decl;
         FUDeclaration* decl2 = t2.decl;

         bool doContinue = false;
         for(int i = 0; i < numberMappings; i++){
            if(mappings[i].t1->decl == decl1 && mappings[i].t2->decl == decl2){
               doContinue = true;
               break;
            }
         }
         if(doContinue){
            continue;
         }

         mappings[numberMappings] = {};
         mappings[numberMappings].graph1 = graph1;
         mappings[numberMappings].graph2 = graph2;
         mappings[numberMappings].t1 = &t1;
         mappings[numberMappings].t2 = &t2;

         numberMappings += 1;
      }
   }
   printf("Mappings: %d\n",numberMappings);
   mappings.size = numberMappings;

   Array<SubgraphMapping> subgraphMappings = PushArray<SubgraphMapping>(arena,numberMappings);

   volatile int inserted = 0;

   SimpleCondition cond = InitSimpleCondition();
   pthread_mutex_t mutex = {};
   pthread_mutex_init(&mutex,NULL);

   Byte* mark = MarkArena(arena);
   int index = 0;

   while(inserted != numberMappings){
      for(auto& map : mappings){
         Assert(map.t1->level == map.t2->level);

         if(map.finished || map.set){
            continue;
         }

         Array<SubgraphMapping> validMappings = subgraphMappings;
         validMappings.size = inserted;
         if(!CheckIfMappingIsPossible(map.t1,map.t2,validMappings)){
            continue;
         }
         //printf("Going to do %d: %.*s - %.*s\n",index,UNPACK_SS(map.t1->decl->name),UNPACK_SS(map.t2->decl->name));

         map.set = true;
         map.inserted = &inserted;
         map.mappings = subgraphMappings;
         map.input = index;
         map.mappingMutex = &mutex;
         map.resultArena = resultArena;
         map.cond = &cond;

         Task task = {};
         task.args = &map;
         task.function = ParallelHierarchicalHeuristicTask;
         task.order = map.t1->level;

         #if 1
         AddTask(task);
         #else
         ParallelHierarchicalHeuristicTask(0,(void*) &map);
         #endif

         index += 1;
      }

      Wait(&cond);
   }

   WaitCompletion();

   exit(0);

   FlatteningTemp graph1All = {};
   graph1All.index = 0;
   graph1All.parent = decl1;
   graph1All.decl = decl1;
   graph1All.name = decl1->name;
   graph1All.subunitsCount = graph1->numberInstances;
   graph1All.edgeCount = graph1->numberEdges;
   graph1All.child = &flattening1[0];

   FlatteningTemp graph2All = {};
   graph2All.index = 0;
   graph2All.parent = decl2;
   graph2All.decl = decl2;
   graph2All.name = decl2->name;
   graph2All.subunitsCount = graph2->numberInstances;
   graph2All.edgeCount = graph2->numberEdges;
   graph2All.child = &flattening2[0];

   /*
   Array<SubgraphMapping> allsubgraphMappingsBuffer = PushArray<SubgraphMapping>(arena,99);
   Memset(allsubgraphMappingsBuffer,{});
   PushPtr<SubgraphMapping> allsubgraphMappings;
   allsubgraphMappings.Init(allsubgraphMappingsBuffer);
   */

   subgraphMappings.size += 1;

   HeuristicState finalState = {};
   finalState.graph1 = graph1;
   finalState.graph2 = graph2;
   finalState.t1 = &graph1All;
   finalState.t2 = &graph2All;
   finalState.inserted = &inserted;
   finalState.mappings = subgraphMappings;
   finalState.input = 999;
   finalState.mappingMutex = &mutex;
   finalState.resultArena = resultArena;
   finalState.cond = &cond;

   ParallelHierarchicalHeuristicTask(0,(void*) &finalState);
   Array<IndexMapping> heuristicMapping = subgraphMappings[subgraphMappings.size - 1].edgeMappings;

   //Array<IndexMapping> heuristicMapping = HierarchicalHeuristic(graph1,graph2,&graph1All,&graph2All,subgraphMappings,arena);

   heuristicMapping = TransformGenericIntoSpecific(heuristicMapping,&graph1All,&graph2All,arena);

   printf("%d\n",heuristicMapping.size);
   GraphMapping mappingRes = {};

   for(IndexMapping& m : heuristicMapping){
      Edge* edge0 = ListGet(graph1->edges,m.index0);
      Edge* edge1 = ListGet(graph2->edges,m.index1);

      #if 0
      region(arena){
         String str1 = Repr(edge0->edge,GRAPH_DOT_FORMAT_NAME,arena);
         String str2 = Repr(edge1->edge,GRAPH_DOT_FORMAT_NAME,arena);
         printf("Going to insert %.*s // %.*s\n",UNPACK_SS(str1),UNPACK_SS(str2));
      };
      FlushStdout();
      #endif

      InsertMapping(mappingRes,edge0->edge,edge1->edge);
   }

   printf("%d\n",mappingRes.edgeMap.size());

   return (MergeGraphResult){};
}



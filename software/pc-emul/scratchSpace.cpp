#include "versatPrivate.hpp"

#include <unistd.h>

#include "merge.hpp"
#include "graph.hpp"
#include "textualRepresentation.hpp"
#include "debug.hpp"
#include "intrinsics.hpp"


void Clique(CliqueState* state,ConsolidationGraph graphArg,int index,IndexRecord* record,int size,Arena* arena){
   static int iterations = 0;
   printf("%d\n",iterations++);

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

   auto end = clock();
   float elapsed = (end - state->start) / CLOCKS_PER_SEC;
   if(elapsed > MAX_CLIQUE_TIME){
      state->found = true;
      return;
   }

   int lastI = index;
   do{
      if(size + num <= state->max){
         return;
      }

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

   state->start = clock();

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

      Clique(state,graph,i,&record,1,arena);
      state->table[i] = state->max;

      PopMark(arena,mark);

      if(state->max == state->upperBound){
         printf("Upperbound finish, index: %d (from %d)\n",i,graph.nodes.size);
         break;
      }

      auto end = clock();
      float elapsed = (end - state->start) / CLOCKS_PER_SEC;
      if(elapsed > MAX_CLIQUE_TIME){
         printf("Timeout, index: %d (from %d)\n",i,graph.nodes.size);
         break;
      }
   }

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

bool Contains(IndexRecord* record,int toCheck){
   FOREACH_LIST(ptr,record){
      if(ptr->index == toCheck){
         return true;
      }
   }
   return false;
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

Array<FlatteningTemp> HierarchicalFlattening(Graph* graph,Arena* arena){
   Array<FlatteningTemp> result = PushArray<FlatteningTemp>(arena,999);

   //ArenaMarker marker(arena);
   String str = OutputDotGraph(graph,arena);

   #if 1
   FILE* file = OpenFileAndCreateDirectories("debug/test.dot","w");
   fprintf(file,"%.*s",UNPACK_SS(str));
   fclose(file);
   #endif

   // Flattening
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
         ptr = res.flatUnitEnd;

         result[index].flattenedUnits = res.flattenedUnits;

         count += nonSpecialSubunits;
         noFlatten = false;

         #if 1
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
   // Calculate number of edges and edge offset
   for(int i = 0; i < index; i++){
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

   #if 1
   for(int i = 0; i < index; i++){
      auto d = result[i];
      printf("%.*s Decl:%.*s N:%d +: %d Edge:%d +: %d\n",UNPACK_SS(d.name),UNPACK_SS(d.decl->name),d.index,d.subunitsCount,d.edgeStart,d.edgeCount);
   }
   #endif

   return result;
}

struct IndexMapping{
   int index0;
   int index1;
};

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

struct SubgraphMapping{
   FUDeclaration* decl1;
   FUDeclaration* decl2;
   Array<IndexMapping> edgeMappings;
   Array<int> table;
   Array<IndexMapping> tableMappings;
};

static ConsolidationResult GenerateConsolidationGraphForSubgraphs(Versat* versat,Arena* arena,Subgraph accel1,Subgraph accel2){
   ConsolidationGraph graph = {};

   // Should be temp memory instead of using memory intended for the graph, but since the graph is expected to use a lot of memory and we are technically saving memory using this mapping, no major problem
   Hashmap<ComplexFUInstance*,MergeEdge> specificsMapping;
   specificsMapping.Init(arena,1000);
   Pool<MappingNode> specificsAdded = {};
   Hashmap<ComplexFUInstance*,int> nodeExists;
   nodeExists.Init(arena,Size(accel1.nodes) + Size(accel2.nodes));

   FOREACH_SUBLIST(ptr,accel1.nodes){
      nodeExists.InsertIfNotExist(ptr,0);
   }
   FOREACH_SUBLIST(ptr,accel2.nodes){
      nodeExists.InsertIfNotExist(ptr,0);
   }

   #if 1
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
   #endif

   // Insert specific mappings first (the first mapping nodes are the specifics)
   #if 0
   for(int i = 0; i < options.nSpecifics; i++){
      SpecificMergeNodes specific = options.specifics[i];

      MappingNode* node = PushStruct<MappingNode>(arena);

      node->type = MappingNode::NODE;
      node->nodes.instances[0] = specific.instA;
      node->nodes.instances[1] = specific.instB;

      graph.nodes.size += 1;
   }
   #endif

   graph.nodes.data = (MappingNode*) MarkArena(arena);

   #if 1
   // Check possible edge mapping
   FOREACH_SUBLIST(edge1,accel1.edges){
      #if 01
      if(!(nodeExists.Get(edge1->units[0].inst) && nodeExists.Get(edge1->units[1].inst))){
         continue;
      }
      #endif

      FOREACH_SUBLIST(edge2,accel2.edges){
         #if 01
         if(!(nodeExists.Get(edge2->units[0].inst) && nodeExists.Get(edge2->units[1].inst))){
            continue;
         }
         #endif

         // TODO: some nodes do not care about which port is connected (think the inputs for common operations, like multiplication, adders and the likes)
         // Can augment the algorithm further to find more mappings
         if(!(EqualPortMapping(versat,edge1->units[0],edge2->units[0]) &&
            EqualPortMapping(versat,edge1->units[1],edge2->units[1]))){
            continue;
         }

         // No mapping input or output units
         #if 1
         if(edge1->units[0].inst->declaration->type == FUDeclaration::SPECIAL ||
            edge1->units[1].inst->declaration->type == FUDeclaration::SPECIAL ||
            edge2->units[0].inst->declaration->type == FUDeclaration::SPECIAL ||
            edge2->units[1].inst->declaration->type == FUDeclaration::SPECIAL){
            continue;
         }
         #endif

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

         #if 0
         // Check to see that the edge maps with all the specific nodes. Specific nodes must be part of the final clique, and therefore we can not insert any edge that could potently make it not happen
         bool conflict = false;
         for(int i = 0; i < options.nSpecifics; i++){
            MappingNode specificNode = graph.nodes[i];

            if(MappingConflict(node,specificNode)){
               conflict = true;
               break;
            }
         }
         if(conflict){
            continue;
         }
         #endif

         MappingNode* space = PushStruct<MappingNode>(arena);

         *space = node;
         graph.nodes.size += 1;
      }
   }
   #endif

   // Check node mapping
   #if 0
   if(options.mapNodes){
      ComplexFUInstance* accel1Output = GetOutputInstance(accel1);
      ComplexFUInstance* accel2Output = GetOutputInstance(accel2);

      for(ComplexFUInstance* instA : accel1->instances){
         PortInstance portA = {};
         portA.inst = instA;

         int deltaA = instA->graphData->order - options.order;

         if(options.type == ConsolidationGraphOptions::EXACT_ORDER && deltaA >= 0 && deltaA <= options.difference){
            continue;
         }

         if(instA == accel1Output || instA->declaration == BasicDeclaration::input){
            continue;
         }

         for(ComplexFUInstance* instB : accel2->instances){
            PortInstance portB = {};
            portB.inst = instB;

            int deltaB = instB->graphData->order - options.order;

            if(options.type == ConsolidationGraphOptions::SAME_ORDER && instA->graphData->order != instB->graphData->order){
               continue;
            }
            if(options.type == ConsolidationGraphOptions::EXACT_ORDER && deltaB >= 0 && deltaB <= options.difference){
               continue;
            }
            if(instB == accel2Output || instB->declaration == BasicDeclaration::input){
               continue;
            }

            if(!EqualPortMapping(versat,portA,portB)){
               continue;
            }

            MappingNode node = {};
            node.type = MappingNode::NODE;
            node.nodes.instances[0] = instA;
            node.nodes.instances[1] = instB;

            MergeEdge* possibleSpecificConflict = specificsMapping.Get(node.nodes.instances[0]);
            if(!possibleSpecificConflict) possibleSpecificConflict = specificsMapping.Get(node.nodes.instances[1]);

            if(possibleSpecificConflict){
               MappingNode specificNode = {};
               specificNode.type = MappingNode::NODE;
               specificNode.nodes = *possibleSpecificConflict;

               if(MappingConflict(node,specificNode)){
                  continue;
               }
            }

            #if 0
            // Same check for nodes in regards to specifics
            bool conflict = false;
            for(int i = 0; i < options.nSpecifics; i++){
               MappingNode specificNode = graph.nodes[i];

               // No point adding same exact node
               if(specificNode.nodes.instances[0] == instA &&
                  specificNode.nodes.instances[1] == instB){
                  conflict = true;
                  break;
               }

               if(MappingConflict(node,specificNode)){
                  conflict = true;
                  break;
               }
            }
            if(conflict){
               continue;
            }
            #endif

            MappingNode* space = PushStruct<MappingNode>(arena);

            *space = node;
            graph.nodes.size += 1;
         }
      }
   }
   #endif

   #if 0
   printf("Size (MB):%d ",((graph.nodes.size / 8) * graph.nodes.size) / Megabyte(1));
   exit(0);
   #endif

   // Order nodes based on how equal in depth they are
   #if 0
   //{  ArenaMarker marker(arena);
      AcceleratorView view1 = CreateAcceleratorView(accel1,arena);
      AcceleratorView view2 = CreateAcceleratorView(accel2,arena);
      view1.CalculateDAGOrdering(arena);
      view2.CalculateDAGOrdering(arena);

      Array<int> count = PushArray<int>(arena,graph.nodes.size);
      Memset(count,0);
      int max = 0;
      for(int i = 0; i < graph.nodes.size; i++){
         int depth = NodeDepth(graph.nodes[i]);
         max = std::max(max,depth);

         if(depth >= graph.nodes.size){
            depth = graph.nodes.size - 1;
         }

         count[depth] += 1;
      }
      Array<int> offsets = PushArray<int>(arena,max + 1);
      Memset(offsets,0);
      for(int i = 1; i < offsets.size; i++){
         offsets[i] = offsets[i-1] + count[i-1];
      }
      Array<MappingNode> sorted = PushArray<MappingNode>(arena,graph.nodes.size);
      for(int i = 0; i < graph.nodes.size; i++){
         int depth = NodeDepth(graph.nodes[i]);

         int offset = offsets[depth];
         offsets[depth] += 1;

         sorted[offset] = graph.nodes[i];
      }

      for(int i = 0; i < sorted.size; i++){
         graph.nodes[i] = sorted[sorted.size - i - 1];
      }
      #if 1
      for(int i = 0; i < graph.nodes.size; i++){
         int depth = NodeDepth(graph.nodes[i]);
         //printf("%d\n",depth);
      }
      #endif
   //}
   #endif

   int upperBound = std::min(Size(accel1.nodes),Size(accel2.nodes));
   Array<BitArray> neighbors = PushArray<BitArray>(arena,graph.nodes.size);
   int times = 0;
   for(int i = 0; i < graph.nodes.size; i++){
      neighbors[i].Init(arena,graph.nodes.size);
      neighbors[i].Fill(0);
   }
   for(int i = 0; i < graph.nodes.size; i++){
      MappingNode node1 = graph.nodes[i];

      for(int ii = 0; ii < i; ii++){
         times += 1;

         MappingNode node2 = graph.nodes[ii];

         if(MappingConflict(node1,node2)){
            continue;
         }

         neighbors.data[i].Set(ii,1);
         neighbors.data[ii].Set(i,1);
      }
   }
   graph.edges = neighbors;

   // Reorder based on degree of nodes
   #if 0
   {  ArenaMarker marker(arena);
      Array<int> degree = PushArray<int>(arena,graph.nodes.size);
      Memset(degree,0);
      for(int i = 0; i < graph.nodes.size; i++){
         degree[i] = graph.edges[i].GetNumberBitsSet();
      }



   #endif

   graph.validNodes.Init(arena,graph.nodes.size);
   graph.validNodes.Fill(1);

   ConsolidationResult res = {};
   res.graph = graph;
   res.upperBound = upperBound;
   res.specificsAdded = specificsAdded;

   return res;
}

struct CalculateCliqueAndGetMappingsResult{
   Array<IndexMapping> mappings;
   Array<int> cliqueTable;
   Array<IndexMapping> cliqueTableMappings;
};

CalculateCliqueAndGetMappingsResult CalculateCliqueAndGetMappings(Versat* versat,Subgraph sub1,Subgraph sub2,Arena* arena){
   Array<IndexMapping> mapping = PushArray<IndexMapping>(arena,999);
   Array<int> cliqueTable = PushArray<int>(arena,999);
   Array<IndexMapping> cliqueTableMappings = PushArray<IndexMapping>(arena,999);

   ConsolidationResult result = GenerateConsolidationGraphForSubgraphs(versat,arena,sub1,sub2);

   for(int i = 0; i < result.graph.nodes.size; i++){
      MappingNode node = result.graph.nodes[i];

      String rep = Repr(node,arena);
      printf("%.*s\n",UNPACK_SS(rep));
   }

   CliqueState* state = InitMaxClique(result.graph,999,arena);
   RunMaxClique(state,arena);
   ConsolidationGraph clique = state->clique;
   Memcpy(cliqueTable.data,state->table.data,state->table.size);

   for(int i = 0; i < state->table.size; i++){
      printf("%d ",state->table[i]);

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

void SortEdgesByVertices(Graph* graph);

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

bool operator==(const IndexMapping& m0,const IndexMapping& m1){
   bool res = (m0.index0 == m1.index0 && m0.index1 == m1.index1);
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

struct FlattenMapping{
   FlatteningTemp* t1;
   FlatteningTemp* t2;
   SubgraphMapping* mapping;
   int weight;
};

#define SWAP(A,B) do { \
   auto TEMP = A; \
   A = B; \
   B = TEMP; \
   } while(0)

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

#if 0
Array<int> CalculateCoreNumbers(ConsolidationGraph graph,Arena* arena){
   ArenaMarker marker(arena);
   int size = graph.nodes.size;
   Array<BitArray> edges = graph.edges;

   Array<int> degrees = PushArray<int>(arena,size);

   for(int i = 0; i < size; i++){
      degrees[i] = edges[i].GetNumberBitsSet();
   }

   Array<int> bins = PushArray<int>(arena,size);
   for(int val : degrees){
      bins[val] += 1;
   }

   Array<int> sortedDegrees = PushArray<int>(arena,size);
   int index = 0;
   for(int i = 0; i < bins.size; i++){
      int numberOfRun = bins[i];

      for(int ii = 0; ii < numberOfRun; ii++){
         sortedDegrees[index++] = i;
      }
   }

   Array<int> offsets = PushArray<int>(arena,size + 1);
   offsets[0] = 0;
   for(int i = 1; i < size + 1; i++){
      offsets[i] = offsets[i - 1] + bins[i - 1];
   }

   Array<int> newIndex = PushArray<int>(arena,size);
   for(int i = 0; i < size; i++){
      int degree = degrees[i];
      int newPos = offsets[degree];
      offsets[degree] += 1;

      newIndex[i] = newPos;
   }
   Array<int> newToOld = PushArray<int>(arena,size);
   for(int i = 0; i < size; i++){
      int oldPos = i;
      int newPos = newIndex[i];

      newToOld[newPos] = oldPos;
   }

   String newToOldS = PushIntTableRepresentation(arena,newToOld);
   printf("%.*s\n",UNPACK_SS(newToOldS));

   String newPos = PushIntTableRepresentation(arena,newIndex);
   printf("%.*s\n",UNPACK_SS(newPos));

   String nonOrdered = PushIntTableRepresentation(arena,degrees);
   printf("%.*s\n",UNPACK_SS(nonOrdered));

   #if 01
   String res = PushIntTableRepresentation(arena,sortedDegrees);
   printf("%.*s\n",UNPACK_SS(res));
   #endif
}
#endif

Array<int> CalculateCoreNumbers(ConsolidationGraph graph,Arena* arena){
   int size = graph.nodes.size;
   Array<BitArray> edges = graph.edges;

   Array<int> core = PushArray<int>(arena,size); // Return graph

   ArenaMarker marker(arena);

   Array<int> degrees = PushArray<int>(arena,size);
   for(int i = 0; i < size; i++){
      degrees[i] = edges[i].GetNumberBitsSet();
   }

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
      seen[index] = true;

      core[index] = smallestDegree;

      for(int neigh : edges[index]){
         if(degrees[neigh] > smallestDegree){
            degrees[neigh] -= 1;
         }
      }
   }

   String res = PushIntTableRepresentation(arena,core);
   printf("%.*s\n",UNPACK_SS(res));

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

FUDeclaration* HierarchicalMergeAccelerators(Versat* versat,Accelerator* accel1,Accelerator* accel2,String name){
   Arena* arena = &versat->temp;
   ArenaMarker marker(arena);

   Graph* graph1 = accel1;
   Array<FlatteningTemp> res1 = HierarchicalFlattening(graph1,arena);

   Graph* graph2 = accel2;
   Array<FlatteningTemp> res2 = HierarchicalFlattening(graph2,arena);

   Hashmap<FUDeclaration*,FlatteningTemp*> decl1 = {};
   decl1.Init(arena,999);

   Hashmap<FUDeclaration*,FlatteningTemp*> decl2 = {};
   decl2.Init(arena,999);

   for(FlatteningTemp& t : res1){
      decl1.InsertIfNotExist(t.decl,&t);
   }
   for(FlatteningTemp& t : res2){
      decl2.InsertIfNotExist(t.decl,&t);
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


         Subgraph sub1 = {};
         sub1.nodes = SimpleSublist(ListGet(graph1->instances,pair1.second->index),pair1.second->subunitsCount);
         sub1.edges = SimpleSublist(ListGet(graph1->edges,pair1.second->edgeStart),pair1.second->edgeCount);

         Subgraph sub2 = {};
         sub2.nodes = SimpleSublist(ListGet(graph2->instances,pair2.second->index),pair2.second->subunitsCount);
         sub2.edges = SimpleSublist(ListGet(graph2->edges,pair2.second->edgeStart),pair2.second->edgeCount);

         OutputDotGraph(sub1,StaticFormat("debug/sub_%.*s_sub.dot",UNPACK_SS(t1->name)),arena);
         OutputDotGraph(sub2,StaticFormat("debug/sub_%.*s_sub.dot",UNPACK_SS(t2->name)),arena);

         CalculateCliqueAndGetMappingsResult res = CalculateCliqueAndGetMappings(versat,sub1,sub2,arena); // Should be, in theory,a pretty good aproximation
         Array<IndexMapping> indexMappings = res.mappings;

         #if 1
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

   ConsolidationGraphOptions options = {};
   ConsolidationResult result = GenerateConsolidationGraph(versat,arena,sub1,sub2,options);
   ConsolidationGraph graph = result.graph;

   for(FlatteningTemp& t1 : res1){
      t1.flag = 0;
   }
   for(FlatteningTemp& t2 : res2){
      t2.flag = 0;
   }

   Byte* mark = MarkArena(arena);
   for(FlatteningTemp& t1 : res1){
      int maxWeight = 0;
      for(FlatteningTemp& t2 : res2){
         if(t1.flag || t2.flag){
            continue;
         }

         for(SubgraphMapping& sub : mappings){
            if(sub.decl1 == t1.decl && sub.decl2 == t2.decl){
               int weight = sub.edgeMappings.size;
               maxWeight = std::max(maxWeight,weight);
            }
         }
      }

      for(FlatteningTemp& t2 : res2){
         if(t1.flag || t2.flag){
            continue;
         }
         for(SubgraphMapping& sub : mappings){
            if(sub.decl1 == t1.decl && sub.decl2 == t2.decl){
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
               map->t1 = &t1;
               map->t2 = &t2;
               map->mapping = &sub;
               map->weight = weight;
               t1.flag = 1;
               t2.flag = 1;

               break;
            }
         }
      }
   }

   Array<FlattenMapping> specificMappings = PointArray<FlattenMapping>(arena,mark);

   for(FlattenMapping& m : specificMappings){
      printf("%.*s:%d %.*s:%d - %d\n",UNPACK_SS(m.t1->decl->name),m.t1->index,UNPACK_SS(m.t2->decl->name),m.t2->index,m.weight);
   }

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

   #if 01
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

/*

   IMPORTANT: Standardize the code. Put it into the format that you are already thinking in.
              The rest is simple. You already know what you need to do, just tackle the things as they come.

*/

   #if 1
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

   SwapCliqueUntil(state,91,95);
   SwapCliqueUntil(state,89,94);
   SwapCliqueUntil(state,88,93);
   SwapCliqueUntil(state,83,92);
   SwapCliqueUntil(state,81,91);
   SwapCliqueUntil(state,80,90);
   SwapCliqueUntil(state,75,89);
   SwapCliqueUntil(state,73,88);
   SwapCliqueUntil(state,72,87);
   SwapCliqueUntil(state,67,86);
   SwapCliqueUntil(state,65,85);
   SwapCliqueUntil(state,64,84);
   SwapCliqueUntil(state,59,83);
   SwapCliqueUntil(state,57,82);
   SwapCliqueUntil(state,56,81);
   SwapCliqueUntil(state,51,80);
   SwapCliqueUntil(state,49,79);
   SwapCliqueUntil(state,48,78);
   SwapCliqueUntil(state,21,77);
   SwapCliqueUntil(state,20,76);
   SwapCliqueUntil(state,19,75);
   SwapCliqueUntil(state,18,74);
   SwapCliqueUntil(state,17,73);
   SwapCliqueUntil(state,16,72);
   SwapCliqueUntil(state,15,71);
   SwapCliqueUntil(state,14,70);
   SwapCliqueUntil(state,13,69);
   SwapCliqueUntil(state,12,68);

/*
   Mess around with node ordering until you find something.
   Alternativly try to dig deep into the reason the max clique function is taking so long.'Put a bunch of printfs inside the function, check which nodes are causing the problems
*/

   #if 0
   SwapCliqueNodes(state,91,95);
   SwapCliqueNodes(state,89,94);
   SwapCliqueNodes(state,88,93);
   SwapCliqueNodes(state,83,92);
   SwapCliqueNodes(state,81,91);
   SwapCliqueNodes(state,80,90);
   SwapCliqueNodes(state,75,89);
   SwapCliqueNodes(state,73,88);
   SwapCliqueNodes(state,72,87);
   SwapCliqueNodes(state,67,86);
   SwapCliqueNodes(state,65,85);
   SwapCliqueNodes(state,64,84);
   SwapCliqueNodes(state,59,83);
   SwapCliqueNodes(state,57,82);
   SwapCliqueNodes(state,56,81);
   SwapCliqueNodes(state,51,80);
   SwapCliqueNodes(state,49,79);
   SwapCliqueNodes(state,48,78);
   SwapCliqueNodes(state,21,77);
   SwapCliqueNodes(state,20,76);
   SwapCliqueNodes(state,19,75);
   SwapCliqueNodes(state,18,74);
   SwapCliqueNodes(state,17,73);
   SwapCliqueNodes(state,16,72);
   SwapCliqueNodes(state,15,71);
   SwapCliqueNodes(state,14,70);
   SwapCliqueNodes(state,13,69);
   SwapCliqueNodes(state,12,68);
   #endif

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

   GraphMapping mapping = {};
   //AddCliqueToMapping(mapping,clique);
   //AddCliqueToMapping(mapping,approximateClique);
   AddSpecificsToMapping(mapping,result.specificsAdded);

   MergeGraph(versat,graph1,graph2,mapping,STRING("Test"));

   return nullptr;
}


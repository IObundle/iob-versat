#include "versatPrivate.hpp"
#include "debug.hpp"
#include "textualRepresentation.hpp"

#include <cstdio>
#include <cstdarg>
#include <unordered_map>

#include <chrono>

template<> class std::hash<PortEdge>{
   public:
   std::size_t operator()(PortEdge const& s) const noexcept{
      int res = SimpleHash(MakeSizedString((const char*) &s,sizeof(PortEdge)));

      return (std::size_t) res;
   }
};

template<> class std::hash<Edge>{
   public:
   std::size_t operator()(Edge const& s) const noexcept{
      int res = SimpleHash(MakeSizedString((const char*) &s,sizeof(Edge)));

      return (std::size_t) res;
   }
};

inline bool operator==(const PortEdge& e1,const PortEdge& e2){
   bool res = (e1.units[0] == e2.units[0] && e1.units[1] == e2.units[1]);
   return res;
}
inline bool operator!=(const PortEdge& e1,const PortEdge& e2){
   bool res = !(e1 == e2);
   return res;
}

inline bool operator==(PortInstance& p1,PortInstance& p2){
   bool sameInstance = (p1.inst == p2.inst);
   //bool samePort = (p1.port == p2.port);

   bool res = sameInstance;
   return res;
}
inline bool operator!=(PortInstance& p1,PortInstance& p2){
   bool res = !(p1 == p2);
   return res;
}

typedef std::unordered_map<ComplexFUInstance*,ComplexFUInstance*> InstanceMap;
typedef std::unordered_map<PortEdge,PortEdge> PortEdgeMap;
typedef std::unordered_map<Edge*,Edge*> EdgeMap;

struct GraphMapping{
   InstanceMap instanceMap;
   InstanceMap reverseInstanceMap;
   PortEdgeMap edgeMap;
};

#if 1
static bool NodeMappingConflict(PortEdge edge1,PortEdge edge2){
   PortInstance p00 = edge1.units[0];
   PortInstance p01 = edge1.units[1];
   PortInstance p10 = edge2.units[0];
   PortInstance p11 = edge2.units[1];

   // If different declarations, return early
   if(p00.inst->declaration != p10.inst->declaration){
      return false;
   }

   #if 0 // There is some bug here
   if(!(edge1.units[0].port == edge2.units[0].port && edge1.units[1].port == edge2.units[1].port)){
      return false;
   }
   #endif

   if(p00 == p10 && p01 != p11){
      return true;
   }
   if(p00 == p11 && p01 != p10){
      return true;
   }
   if(p00 != p10 && p01 == p11){
      return true;
   }
   if(p00 != p11 && p01 == p10){
      return true;
   }

   return false;
}
#endif

bool MappingConflict(MappingNode map1,MappingNode map2){
   if(map1.type == MappingNode::NODE && map2.type == MappingNode::NODE){
      FUInstance* p00 = map1.nodes.instances[0];
      FUInstance* p01 = map1.nodes.instances[1];
      FUInstance* p10 = map2.nodes.instances[0];
      FUInstance* p11 = map2.nodes.instances[1];

      if(p00 == p10 && p01 != p11){
         return true;
      }

      if(p00 != p10 && p01 == p11){
         return true;
      }
   } else if(map1.type == MappingNode::EDGE && map2.type == MappingNode::EDGE){
      #if 1
      PortEdge nodeMapping00 = {map1.edges[0].units[0],map1.edges[1].units[0]};
      PortEdge nodeMapping01 = {map1.edges[0].units[1],map1.edges[1].units[1]};
      PortEdge nodeMapping10 = {map2.edges[0].units[0],map2.edges[1].units[0]};
      PortEdge nodeMapping11 = {map2.edges[0].units[1],map2.edges[1].units[1]};

      bool res = NodeMappingConflict(nodeMapping00,nodeMapping10) ||
                 NodeMappingConflict(nodeMapping01,nodeMapping10) ||
                 NodeMappingConflict(nodeMapping00,nodeMapping11) ||
                 NodeMappingConflict(nodeMapping01,nodeMapping11);
      #endif

      return res;
   } else {
      // Set node mapping to map1 if not already

      if(map1.type == MappingNode::EDGE){
         MappingNode tmp = map2;
         map2 = map1;
         map1 = tmp;
      }

      FUInstance* p0 = map1.nodes.instances[0]; // Maps node p0 to p1
      FUInstance* p1 = map1.nodes.instances[1];

      FUInstance* e00 = map2.edges[0].units[0].inst; // Maps node e00 to e10
      FUInstance* e10 = map2.edges[1].units[0].inst;

      FUInstance* e01 = map2.edges[0].units[1].inst; // Maps node e10 to e11
      FUInstance* e11 = map2.edges[1].units[1].inst;

      #define XOR(A,B,C,D) ((A == B && !(C == D)) || (!(A == B) && C == D))

      if(XOR(p0,e00,p1,e10)){
         return true;
      }
      if(XOR(p0,e01,p1,e11)){
         return true;
      }
   }

   return false;
}

int EdgeEqual(PortEdge edge1,PortEdge edge2){
   int res = (memcmp(&edge1,&edge2,sizeof(PortEdge)) == 0);

   return res;
}

int MappingNodeEqual(MappingNode node1,MappingNode node2){
   int res = (EdgeEqual(node1.edges[0],node2.edges[0]) &&
              EdgeEqual(node1.edges[1],node2.edges[1]));

   return res;
}

ConsolidationGraph Copy(ConsolidationGraph graph,Arena* arena){
   ConsolidationGraph res = {};

   res = graph;
   res.validNodes.Init(arena,graph.nodes.size);
   res.validNodes.Copy(graph.validNodes);

   return res;
}

int NumberNodes(ConsolidationGraph graph){
   int num = 0;
   for(int i = 0; i < graph.nodes.size; i++){
      if(graph.validNodes.Get(i)){
         num += 1;
      }
   }

   return num;
}

int NodeIndex(ConsolidationGraph graph,MappingNode* node){
   int index = node - graph.nodes.data;
   return index;
}

int ValidNodes(ConsolidationGraph graph){
   int count = 0;
   for(int i = 0; i < graph.nodes.size; i++){
      if(graph.validNodes.Get(i)){
         count += 1;
      }
   }

   return count;
}

struct IsCliqueResult{
   bool result;
   int failedIndex;
};

IsCliqueResult IsClique(ConsolidationGraph graph,BitArray* neighbors,Arena* arena){
   IsCliqueResult res = {};
   res.result = true;

   int cliqueSize = ValidNodes(graph);

   // Zero or one are always cliques
   if(cliqueSize == 0 || cliqueSize == 1){
      return res;
   }

   for(int i = 0; i < graph.nodes.size; i++){
      MappingNode* node = &graph.nodes[i];

      if(!graph.validNodes.Get(i)){
         continue;
      }

      int count = 0;
      for(int ii = 0; ii < graph.edges.size; ii++){
         MappingEdge* edge = &graph.edges[ii];

         // Only count edge if both nodes are valid
         if(edge->nodes[0] == node){
            int index = NodeIndex(graph,edge->nodes[1]);
            if(graph.validNodes.Get(index)){
               count += 1;
            }
         } else if(edge->nodes[1] == node){
            int index = NodeIndex(graph,edge->nodes[0]);
            if(graph.validNodes.Get(index)){
               count += 1;
            }
         }
      }

      if(count != cliqueSize - 1){ // Each node must have (cliqueSize - 1) connections otherwise not a clique
         res.failedIndex = i;
         res.result = false;
         return res;
      }
   }

   return res;
}

struct CliqueState{
   int max;
   int* table;
   ConsolidationGraph clique;
   bool found;
   std::chrono::time_point<std::chrono::steady_clock> start;
};

struct IndexRecord{
   int index;
   IndexRecord* next;
};

#define MAX_CLIQUE_TIME 10.0f

void Clique(CliqueState* state,ConsolidationGraph graphArg,IndexRecord* record,int size,BitArray* neighbors,Arena* arena){
   ConsolidationGraph graph = Copy(graphArg,arena);

   if(NumberNodes(graph) == 0){
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

   auto end = std::chrono::steady_clock::now();
   std::chrono::duration<float> elapsed_seconds = end - state->start;

   if(elapsed_seconds.count() > MAX_CLIQUE_TIME){
      state->found = true;
      return;
   }

   int num = 0;
   while((num = NumberNodes(graph)) != 0){
      if(size + num <= state->max){
         return;
      }

      int i;
      for(i = 0; i < graph.nodes.size; i++){
         if(graph.validNodes.Get(i)){
            break;
         }
      }

      //printf("%d\n",i);

      if(size + state->table[i] <= state->max){
         return;
      }

      graph.validNodes.Set(i,0);

      Byte* mark = MarkArena(arena);
      ConsolidationGraph tempGraph = Copy(graph,arena);

      tempGraph.validNodes &= neighbors[i];

      IndexRecord newRecord = {};
      newRecord.index = i;
      newRecord.next = record;

      Clique(state,tempGraph,&newRecord,size + 1,neighbors,arena);

      PopMark(arena,mark);

      if(state->found == true){
         return;
      }
   }
}

BitArray* CalculateNeighborsTable(ConsolidationGraph graph,Arena* arena){
   BitArray* neighbors = PushArray(arena,graph.nodes.size,BitArray).data;
   for(int i = 0; i < graph.nodes.size; i++){
      neighbors[i].Init(arena,graph.nodes.size);
      neighbors[i].Fill(0);
   }

   for(int i = 0; i < graph.edges.size; i++){
      MappingEdge edge = graph.edges[i];

      int index0 = NodeIndex(graph,edge.nodes[0]);
      int index1 = NodeIndex(graph,edge.nodes[1]);

      neighbors[index0].Set(index1,1);
      neighbors[index1].Set(index0,1);
   }

   return neighbors;
}

ConsolidationGraph MaxClique(ConsolidationGraph graph,Arena* arena){
   CliqueState state = {};
   state.table = PushArray(arena,graph.nodes.size,int).data;
   state.clique = Copy(graph,arena); // Preserve nodes and edges, but allocates different valid nodes
   state.start = std::chrono::steady_clock::now();

   graph.validNodes.Fill(0);

   // Pre calculate nodes neighbors
   BitArray* neighbors = CalculateNeighborsTable(graph,arena);

   for(int i = graph.nodes.size - 1; i >= 0; i--){
      Byte* mark = MarkArena(arena);

      state.found = false;
      for(int j = i; j < graph.nodes.size; j++){
         graph.validNodes.Set(j,1);
      }

      graph.validNodes &= neighbors[i];

      IndexRecord record = {};
      record.index = i;

      Clique(&state,graph,&record,1,neighbors,arena);
      state.table[i] = state.max;

      //printf("T:%d %d\n",i,state.table[i]);

      PopMark(arena,mark);

      auto end = std::chrono::steady_clock::now();
      std::chrono::duration<float> elapsed_seconds = end - state.start;

      if(elapsed_seconds.count() > MAX_CLIQUE_TIME){
         break;
      }
   }

   Assert(IsClique(state.clique,neighbors,arena).result);

   #if 0
   auto end = std::chrono::steady_clock::now();
   std::chrono::duration<float> elapsed_seconds = end - state.start;

   printf("Clique time:%f\n",elapsed_seconds.count());
   #endif

   return state.clique;
}

// Checks if nodes are "equal" in terms of mapping
bool EqualPortMapping(Versat* versat,PortInstance p1,PortInstance p2){
   FUDeclaration* d1 = p1.inst->declaration;
   FUDeclaration* d2 = p2.inst->declaration;

   if(d1 == versat->input && d2 == versat->input){ // Check the special case for inputs
      if(GetInputPortNumber(versat,p1.inst) == GetInputPortNumber(versat,p2.inst)){
         return true;
      } else {
         return false;
      }
   }

   bool res = ((d1 == d2) && (p1.port == p2.port));

   return res;
}

inline bool EdgeOrder(Edge* edge,ConsolidationGraphOptions options){
   int order0 = edge->units[0].inst->graphData->order;
   int order1 = edge->units[1].inst->graphData->order;

   if(options.type == ConsolidationGraphOptions::EXACT_ORDER){
      int delta0 = order0 - options.order;
      int delta1 = order1 - options.order;

      if(delta0 < 0 || delta1 < 0){
         return false;
      }

      if(delta0 <= options.difference && delta1 <= options.difference){
         return true;
      } else {
         return false;
      }
   }

   return true;
}

ConsolidationGraph GenerateConsolidationGraph(Versat* versat,Arena* arena,Accelerator* accel1,Accelerator* accel2,ConsolidationGraphOptions options){
   ConsolidationGraph graph = {};

   graph.nodes.data = (MappingNode*) MarkArena(arena);

   // Insert specific mappings first (the first mapping nodes are the specifics)
   #if 1
   for(int i = 0; i < options.nSpecifics; i++){
      SpecificMergeNodes specific = options.specifics[i];

      MappingNode* node = PushStruct(arena,MappingNode);

      node->type = MappingNode::NODE;
      node->nodes.instances[0] = specific.instA;
      node->nodes.instances[1] = specific.instB;

      graph.nodes.size += 1;
   }
   #endif

   #if 1
   // Check possible edge mapping
   for(Edge* edge1 : accel1->edges){
      if(!EdgeOrder(edge1,options)){
         continue;
      }

      for(Edge* edge2 : accel2->edges){
         // TODO: some nodes do not care about which port is connected (think the inputs for common operations, like multiplication, adders and the likes)
         // Can augment the algorithm further to find more mappings
         if(!EdgeOrder(edge2,options)){
            continue;
         }

         if(!(EqualPortMapping(versat,edge1->units[0],edge2->units[0]) &&
            EqualPortMapping(versat,edge1->units[1],edge2->units[1]))){
            continue;
         }

         MappingNode node = {};
         node.type = MappingNode::EDGE;
         node.edges[0].units[0] = edge1->units[0];
         node.edges[0].units[1] = edge1->units[1];
         node.edges[1].units[0] = edge2->units[0];
         node.edges[1].units[1] = edge2->units[1];

         #if 1
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

         MappingNode* space = PushStruct(arena,MappingNode);

         *space = node;
         graph.nodes.size += 1;
      }
   }
   #endif

   // Check node mapping
   #if 1
   if(options.mapNodes){
      for(ComplexFUInstance* instA : accel1->instances){
         PortInstance portA = {};
         portA.inst = instA;

         int deltaA = instA->graphData->order - options.order;

         if(options.type == ConsolidationGraphOptions::EXACT_ORDER && deltaA >= 0 && deltaA <= options.difference){
            continue;
         }

         if(instA == accel1->outputInstance || instA->declaration == versat->input){
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
            if(instB == accel2->outputInstance || instB->declaration == versat->input){
               continue;
            }

            if(!EqualPortMapping(versat,portA,portB)){
               continue;
            }

            MappingNode node = {};
            node.type = MappingNode::NODE;
            node.nodes.instances[0] = instA;
            node.nodes.instances[1] = instB;

            #if 1
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

            MappingNode* space = PushStruct(arena,MappingNode);

            *space = node;
            graph.nodes.size += 1;
         }
      }
   }
   #endif

   #if 0
   // Map outputs
   if(accel1->outputInstance && accel2->outputInstance){
      MappingNode* node = PushStruct(arena,MappingNode);

      node->type = MappingNode::NODE;
      node->nodes.instances[0] = accel1->outputInstance;
      node->nodes.instances[1] = accel2->outputInstance;

      graph.nodes.size += 1;
   }

   // Map inputs
   for(ComplexFUInstance* instA : accel1->instances){
      if(instA->declaration != versat->input){
         continue;
      }

      for(ComplexFUInstance* instB : accel2->instances){
         if(instB->declaration != versat->input){
            continue;
         }

         if(instA->id != instB->id){
            continue;
         }

         MappingNode* node = PushStruct(arena,MappingNode);

         node->type = MappingNode::NODE;
         node->nodes.instances[0] = instA;
         node->nodes.instances[1] = instB;

         graph.nodes.size += 1;
      }
   }
   #endif

   graph.edges.data = (MappingEdge*) MarkArena(arena);
   int times = 0;
   bool runOutOfSpace = false;
   // Check edge conflicts
   for(int i = 0; i < graph.nodes.size; i++){
      for(int ii = 0; ii < i; ii++){
         times += 1;

         MappingNode* node1 = &graph.nodes[i];
         MappingNode* node2 = &graph.nodes[ii];

         if(MappingConflict(*node1,*node2)){
            continue;
         }

         #if 1
         if(arena->used + sizeof(MappingEdge) < arena->totalAllocated){
            MappingEdge* edge = PushStruct(arena,MappingEdge);

            edge->nodes[0] = node1;
            edge->nodes[1] = node2;
         } else {
            runOutOfSpace = true;
         }
         #endif

         graph.edges.size += 1;
      }
   }

   if(runOutOfSpace){
      printf("No more space Nodes: %d Edges: %d\n",graph.nodes.size,graph.edges.size);
      return graph;
   }

   // Reorder vertices
   #if 01
   int nodes = graph.nodes.size;
   int* degree = PushArray(arena,nodes,int).data;
   Memset(degree,0,nodes);

   for(int i = 0; i < graph.edges.size; i++){
      MappingEdge* edge = &graph.edges[i];

      int node0 = NodeIndex(graph,edge->nodes[0]);
      int node1 = NodeIndex(graph,edge->nodes[1]);

      degree[node0] += 1;
      degree[node1] += 1;
   }

   int* indexToMinimumNode = PushArray(arena,nodes,int).data;
   for(int i = 0; i < nodes; i++){
      int minimum = 999999;
      int minimumIndex = -1;

      // Find minimum
      for(int ii = 0; ii < nodes; ii++){
         if(degree[ii] < minimum){
            minimumIndex = ii;
            minimum = degree[ii];
         }
      }

      // Save value
      indexToMinimumNode[minimumIndex] = i;

      // Remove degree for every neighbor
      for(int ii = 0; ii < graph.edges.size; ii++){
         MappingEdge* edge = &graph.edges[ii];

         int node0 = NodeIndex(graph,edge->nodes[0]);
         int node1 = NodeIndex(graph,edge->nodes[1]);

         if(node0 == minimumIndex){
            degree[node1] -= 1;
         } else if(node1 == minimumIndex){
            degree[node0] -= 1;
         }
      }

      degree[minimumIndex] = 99999;
   }

   #if 0
   for(int i = 0; i < nodes; i++){
      ArenaMarker marker(arena);
      printf("%d->%d\n",i,indexToMinimumNode[i]);

      SizedString str = MappingNodeIdentifier(&graph.nodes[i],arena);

      printf("%.*s\n",UNPACK_SS(str));
   }
   #endif

   MappingNode* newMapping = PushArray(arena,nodes,MappingNode).data;

   for(int i = 0; i < nodes; i++){
      int mapped = indexToMinimumNode[i];

      newMapping[mapped] = graph.nodes[i];
   }

   // Update edges
   for(int i = 0; i < graph.edges.size; i++){
      MappingEdge* edge = &graph.edges[i];

      int node0 = NodeIndex(graph,edge->nodes[0]);
      int node1 = NodeIndex(graph,edge->nodes[1]);

      int m0 = indexToMinimumNode[node0];
      int m1 = indexToMinimumNode[node1];

      edge->nodes[0] = &newMapping[m0];
      edge->nodes[1] = &newMapping[m1];
   }

   graph.nodes.data = newMapping;

   // Check if reordering is correct
   #if 0
   int* testDegree = PushArray(arena,nodes,int);
   Memset(testDegree,0,nodes);

   for(int i = 0; i < graph.numberEdges; i++){
      MappingEdge* edge = &graph.edges[i];

      int node0 = NodeIndex(graph,edge->nodes[0]);
      int node1 = NodeIndex(graph,edge->nodes[1]);

      testDegree[node0] += 1;
      testDegree[node1] += 1;
   }

   for(int i = 0; i < nodes; i++){
      printf("%d\n",testDegree[i]);
   }
   #endif

   #endif

   graph.validNodes.Init(arena,graph.nodes.size);
   graph.validNodes.Fill(1);

   return graph;
}


static void OutputConsolidationGraph(ConsolidationGraph graph,Arena* memory,bool onlyOutputValid,const char* format,...){
   va_list args;
   va_start(args,format);

   SizedString fileName = vPushString(memory,format,args);
   PushNullByte(memory);

   va_end(args);

   FILE* outputFile = fopen(fileName.data,"w");

   fprintf(outputFile,"graph GraphName {\n\tnode [fontcolor=white,style=filled,color=\"160,60,176\"];\n");
   for(int i = 0; i < graph.nodes.size; i++){
      MappingNode* node = &graph.nodes[i];

      if(onlyOutputValid && !graph.validNodes.Get(i)){
         continue;
      }

      ArenaMarker marker(memory);
      fprintf(outputFile,"\t\"%.*s\";\n",UNPACK_SS(Repr(*node,memory)));
   }

   for(int i = 0; i < graph.edges.size; i++){
      MappingEdge* edge = &graph.edges[i];

      if(onlyOutputValid){
         int node0 = NodeIndex(graph,edge->nodes[0]);
         int node1 = NodeIndex(graph,edge->nodes[1]);

         if(!(graph.validNodes.Get(node0) && graph.validNodes.Get(node1))){
            continue;
         }
      }

      ArenaMarker marker(memory);
      SizedString node1 = Repr(*edge->nodes[0],memory);
      SizedString node2 = Repr(*edge->nodes[1],memory);

      fprintf(outputFile,"\t\"%.*s\" -- \"%.*s\";\n",UNPACK_SS(node1),UNPACK_SS(node2));
   }

   fprintf(outputFile,"}\n");
   fclose(outputFile);
}

void DoInsertMapping(InstanceMap& map,ComplexFUInstance* inst1,ComplexFUInstance* inst0){
   auto iter = map.find(inst1);

   if(iter != map.end()){
      Assert(iter->second == inst0);
      return;
   }

   map.insert({inst1,inst0});
}

void InsertMapping(GraphMapping& map,ComplexFUInstance* inst1,ComplexFUInstance* inst0){
   DoInsertMapping(map.instanceMap,inst1,inst0);
   DoInsertMapping(map.reverseInstanceMap,inst0,inst1);
}

GraphMapping ConsolidationGraphMapping(Versat* versat,Accelerator* accel1,Accelerator* accel2,ConsolidationGraphOptions options,Arena* arena){
   ArenaMarker marker(arena);

   GraphMapping res;

   ConsolidationGraph graph = GenerateConsolidationGraph(versat,arena,accel1,accel2,options);

   if(graph.validNodes.bitSize == 0){
      return res;
   }

   OutputConsolidationGraph(graph,arena,true,"debug/ConsolidationGraph.dot");

   //printf("Consolidation graph: Nodes:%d Edges:%d\n",graph.numberNodes,graph.numberEdges);

   ConsolidationGraph clique = MaxClique(graph,arena);
   OutputConsolidationGraph(clique,arena,true,"debug/Clique1.dot");

   //printf("Clique size: %d\n",ValidNodes(clique));
   #if 0
   options.mapNodes = true;
   ConsolidationGraph nodeClique = GenerateConsolidationGraph(versat,arena,accel1,accel2,options);
   OutputConsolidationGraph(nodeClique,arena,true,"debug/nodeClique.dot");

   nodeClique.validNodes.Copy(clique.validNodes);
   BitArray* neighbors = CalculateNeighborsTable(nodeClique,arena);
   for(int i = 0; i < nodeClique.numberNodes; i++){
      MappingNode* node = &nodeClique.nodes[i];

      if(node->type != MappingNode::NODE){
         continue;
      }

      nodeClique.validNodes.Set(i,1);

      IsCliqueResult res = IsClique(nodeClique,neighbors,arena);

      if(!res.result){
         nodeClique.validNodes.Set(i,0);
      }
   }
   OutputConsolidationGraph(nodeClique,arena,true,"debug/Clique2.dot");
   clique = nodeClique;
   #endif

   for(int i = 0; i < clique.nodes.size; i++){
      MappingNode node = clique.nodes[i];

      if(!clique.validNodes.Get(i)){
         continue;
      }

      if(node.type == MappingNode::NODE){
         MergeEdge& nodes = node.nodes;

         InsertMapping(res,nodes.instances[1],nodes.instances[0]);
      } else { // Edge mapping
         PortEdge& edge0 = node.edges[0]; // Edge in graph 1
         PortEdge& edge1 = node.edges[1]; // Edge in graph 2

         InsertMapping(res,edge1.units[0].inst,edge0.units[0].inst);
         InsertMapping(res,edge1.units[1].inst,edge0.units[1].inst);

         res.edgeMap.insert({edge1,edge0});
      }
   }

   #if 0
   for(ComplexFUInstance* inst : accel1->instances){
      inst->tag = 0;
   }
   for(ComplexFUInstance* inst : accel2->instances){
      inst->tag = 0;
   }
   for(auto iter : res.instanceMap){
      iter.first->tag = 1;
      iter.second->tag = 1;
   }
   for(ComplexFUInstance* inst1 : accel1->instances){
      if(inst1->tag){
         continue;
      }
      for(ComplexFUInstance* inst2 : accel2->instances){
         if(inst2->tag){
            continue;
         }

         if(inst1->declaration != inst2->declaration){
            continue;
         }

         InsertMapping(res,inst2,inst1);
         inst1->tag = 1;
         inst2->tag = 1;
         break;
      }
   }
   #endif

   return res;
}

static GraphMapping FirstFitGraphMapping(Versat* versat,Accelerator* accel1,Accelerator* accel2,Arena* arena){
   GraphMapping res;

   for(ComplexFUInstance* inst : accel1->instances){
      inst->tag = 0;
   }
   for(ComplexFUInstance* inst : accel2->instances){
      inst->tag = 0;
   }

   for(ComplexFUInstance* inst1 : accel1->instances){
      if(inst1->tag){
         continue;
      }

      for(ComplexFUInstance* inst2 : accel2->instances){
         if(inst2->tag){
            continue;
         }

         if(inst1->declaration == inst2->declaration){
            res.instanceMap.insert({inst2,inst1});
            inst1->tag = 1;
            inst2->tag = 1;
            break;
         }
      }
   }

   NOT_IMPLEMENTED; // Not fully implemented. Missing edge mappings
   return res;
}

#include <algorithm>

struct {
   bool operator()(ComplexFUInstance* f1, ComplexFUInstance* f2) const {
      bool res = false;
      if(f1->graphData->order == f2->graphData->order){
         res = (f1->declaration < f2->declaration);
      } else {
         res = (f1->graphData->order < f2->graphData->order);
      }
      return res;
   }
} compare;

void OrderedMatch(std::vector<ComplexFUInstance*>& order1,std::vector<ComplexFUInstance*>& order2,InstanceMap& map,int orderOffset){
   auto iter1 = order1.begin();
   auto iter2 = order2.begin();

   // Match same order
   for(; iter1 != order1.end() && iter2 != order2.end();){
      ComplexFUInstance* inst1 = *iter1;
      ComplexFUInstance* inst2 = *iter2;

      int val1 = inst1->graphData->order;
      int val2 = inst1->graphData->order;

      if(abs(val1 - val2) <= orderOffset && inst1->declaration == inst2->declaration){
         Assert(!inst1->tag);
         Assert(!inst2->tag);

         inst1->tag = 1;
         inst2->tag = 1;

         map.insert({inst2,inst1});

         ++iter1;
         ++iter2;
         continue;
      }

      if(compare(inst1,inst2)){
         ++iter1;
      } else if(!compare(inst1,inst2)){
         ++iter2;
      } else {
         ++iter1;
         ++iter2;
      }
   }
}

static GraphMapping OrderedFitGraphMapping(Versat* versat,Accelerator* accel0,Accelerator* accel1,Arena* arena){
   ArenaMarker marker(arena);

   AcceleratorView view0 = CreateAcceleratorView(accel0,arena);
   view0.CalculateDAGOrdering(arena);
   AcceleratorView view1 = CreateAcceleratorView(accel1,arena);
   view1.CalculateDAGOrdering(arena);

   GraphMapping res = {};

   for(ComplexFUInstance* inst : accel0->instances){
      inst->tag = 0;
   }

   for(ComplexFUInstance* inst : accel1->instances){
      inst->tag = 0;
   }

   int minIterations = std::min(accel0->instances.Size(),accel1->instances.Size());

   auto BinaryNext = [](int i){
      if(i == 0){
         return 1;
      } else {
         return (i * 2);
      }
   };

   std::vector<ComplexFUInstance*> order1;
   std::vector<ComplexFUInstance*> order2;
   for(int i = 0; i < minIterations * 2; i = BinaryNext(i)){
      order1.clear();
      order2.clear();
      for(ComplexFUInstance* inst : accel0->instances){
         if(!inst->tag){
            order1.push_back(inst);
         }
      }
      for(ComplexFUInstance* inst : accel1->instances){
         if(!inst->tag){
            order2.push_back(inst);
         }
      }

      std::sort(order1.begin(),order1.end(),compare);
      std::sort(order2.begin(),order2.end(),compare);

      OrderedMatch(order1,order2,res.instanceMap,i);
   }

   NOT_IMPLEMENTED; // Missing edge mapping
   return res;
}

GraphMapping TestingGraphMapping(Versat* versat,Accelerator* accel0,Accelerator* accel1,Arena* arena){
   ArenaMarker marker(arena);

   AcceleratorView view0 = CreateAcceleratorView(accel0,arena);
   view0.CalculateDAGOrdering(arena);
   AcceleratorView view1 = CreateAcceleratorView(accel1,arena);
   view1.CalculateDAGOrdering(arena);

   int maxOrder1 = 0;
   for(ComplexFUInstance* inst : accel0->instances){
      maxOrder1 = std::max(maxOrder1,inst->graphData->order);
   }
   int maxOrder2 = 0;
   for(ComplexFUInstance* inst : accel1->instances){
      maxOrder2 = std::max(maxOrder2,inst->graphData->order);
   }

   int maxOrder = std::min(maxOrder1,maxOrder2);

   int difference = 2;

   ConsolidationGraphOptions options = {};
   options.type = ConsolidationGraphOptions::EXACT_ORDER;
   options.difference = difference - 1;
   options.mapNodes = true;

   GraphMapping res;
   for(int i = 0; i < maxOrder - 1; i += difference){
      ArenaMarker marker(arena);

      options.order = i;

      ConsolidationGraph graph = GenerateConsolidationGraph(versat,arena,accel0,accel1,options);

      if(graph.validNodes.bitSize == 0){
         return res;
      }

      BitArray* neighbors = CalculateNeighborsTable(graph,arena);

      printf("%d\n",ValidNodes(graph));

      ConsolidationGraph clique = MaxClique(graph,arena);

      Assert(IsClique(clique,neighbors,arena).result);

      //printf("%d %d\n",i,NumberNodes(clique));

      for(int ii = 0; ii < clique.nodes.size; ii++){
         MappingNode node = clique.nodes[ii];

         if(!clique.validNodes.Get(ii)){
            continue;
         }

         if(node.type == MappingNode::NODE){
            MergeEdge& nodes = node.nodes;

            InsertMapping(res,nodes.instances[1],nodes.instances[0]);
         } else { // Edge mapping
            PortEdge& edge0 = node.edges[0]; // Edge in graph 1
            PortEdge& edge1 = node.edges[1]; // Edge in graph 2

            InsertMapping(res,edge1.units[0].inst,edge0.units[0].inst);
            InsertMapping(res,edge1.units[1].inst,edge0.units[1].inst);

            res.edgeMap.insert({edge1,edge0});
         }
      }

      #if 0
      printf("%d %d %d\n",i,graph.numberNodes,graph.numberEdges);
      #endif
   }

   return res;
}


struct OverheadCount{
   int muxes;
   int buffers;
};

OverheadCount CountOverheadUnits(Versat* versat,Accelerator* accel){
   OverheadCount res = {};

   for(FUInstance* inst : accel->instances){
      FUDeclaration* decl = inst->declaration;

      if(decl == versat->multiplexer || decl == versat->combMultiplexer){
         res.muxes += 1;
      }

      if(decl == versat->buffer || decl == versat->fixedBuffer){
         res.buffers += 1;
      }
   }

   return res;
}

void PrintAcceleratorStats(Versat* versat,Accelerator* accel){
   OverheadCount count = CountOverheadUnits(versat,accel);
   printf("Instances:%d Edges:%d Muxes:%d Buffer:%d",accel->instances.Size(),accel->edges.Size(),count.muxes,count.buffers);
}

static int Weight(Versat* versat,Accelerator* accel,bool countBuffer = false){
   int weight = 0;

   for(FUInstance* inst : accel->instances){
      FUDeclaration* decl = inst->declaration;

      if(decl->type == FUDeclaration::SPECIAL){
         continue;
      }

      //weight += 1;

      #if 1
      if(decl->isOperation){
         weight += 1;
         continue;
      }

      if(decl == versat->multiplexer || decl == versat->combMultiplexer){
         weight += 1;
         continue;
      }

      if(decl == versat->buffer || decl == versat->fixedBuffer){
         if(countBuffer){
            weight += 1;
         }
         continue;
      }

      weight += 2;
      #endif
   }

   return weight;
}

void PrintSavedByMerge(Versat* versat,Accelerator* finalAccel,Accelerator* accel1,Accelerator* accel2){
   printf("Accel1: ");
   PrintAcceleratorStats(versat,accel1);
   printf("\nAccel2: ");
   PrintAcceleratorStats(versat,accel2);
   printf("\nFinal: ");
   PrintAcceleratorStats(versat,finalAccel);
   printf("\n");

   Accelerator* simpleCombine = MergeAccelerator(versat,accel1,accel2,nullptr,0,MergingStrategy::SIMPLE_COMBINATION);
   NOT_IMPLEMENTED; //
   //FixMultipleInputs(versat,simpleCombine);
   //CalculateDelay(versat,simpleCombine);

   printf("Simple Combine: %d %d\n",simpleCombine->instances.Size(),simpleCombine->edges.Size());

   float change1,change2;

   {
   float base = (float) Weight(versat,simpleCombine,false);
   float finalW = (float) Weight(versat,finalAccel,false);
   change1 = (base / finalW) - 1.0f;
   }

   {
   float base = (float) Weight(versat,simpleCombine,true);
   float finalW = (float) Weight(versat,finalAccel,true);
   change2 = (base / finalW) - 1.0f;
   }

   printf("Saved Weight: %.02f (with buffers %.02f) \n",change1,change2);
}

void PrintMergePossibility(Versat* versat,Accelerator* accel1,Accelerator* accel2){
   for(ComplexFUInstance* inst1 : accel1->instances){
      inst1->tag = 0;
   }
   for(ComplexFUInstance* inst2 : accel2->instances){
      inst2->tag = 0;
   }

   int count = 0;
   int total = std::max(accel1->instances.Size(),accel2->instances.Size());
   for(ComplexFUInstance* inst1 : accel1->instances){
      Assert(!inst1->tag);

      for(ComplexFUInstance* inst2 : accel2->instances){
         if(inst2->tag){
            continue;
         }
         if(inst1->declaration != inst2->declaration){
            continue;
         }

         count += 1;
         inst1->tag = 1;
         inst2->tag = 1;
         break;
      }
   }

   printf("Likeness: %d/%d\n",count,total);
}

Accelerator* MergeAccelerator(Versat* versat,Accelerator* accel1,Accelerator* accel2,SpecificMergeNodes* specificNodes,int nSpecifics,MergingStrategy strategy){
   Arena* arena = &versat->temp;
   ArenaMarker marker(arena);

   //PrintMergePossibility(versat,accel1,accel2);

   GraphMapping graphMapping;

   switch(strategy){
   case MergingStrategy::SIMPLE_COMBINATION:{
      // Do nothing, no mapping leads to simple combination
   }break;
   case MergingStrategy::CONSOLIDATION_GRAPH:{
      ConsolidationGraphOptions options = {};
      options.mapNodes = false;
      options.specifics = specificNodes;
      options.nSpecifics = nSpecifics;

      graphMapping = ConsolidationGraphMapping(versat,accel1,accel2,options,arena);
   }break;
   case MergingStrategy::PIECEWISE_CONSOLIDATION_GRAPH:{
      graphMapping = TestingGraphMapping(versat,accel1,accel2,arena);
   }break;
   case MergingStrategy::FIRST_FIT:{
      graphMapping = FirstFitGraphMapping(versat,accel1,accel2,arena);
   }break;
   case MergingStrategy::ORDERED_FIT:{
      graphMapping = OrderedFitGraphMapping(versat,accel1,accel2,arena);
   }break;
   }

   std::vector<Edge*> accel1EdgeMap;
   std::vector<Edge*> accel2EdgeMap;
   InstanceMap map = {};
   Accelerator* newGraph = CreateAccelerator(versat);

   // Create base instances from accel 1
   for(ComplexFUInstance* inst : accel1->instances){
      ComplexFUInstance* newNode = (ComplexFUInstance*) CreateFUInstance(newGraph,inst->declaration,inst->name);

      map.insert({inst,newNode});
   }

   #if 1
   // Create base instances from accel 2, unless they are mapped to nodes from accel1
   for(ComplexFUInstance* inst : accel2->instances){
      auto mapping = graphMapping.instanceMap.find(inst); // Returns mapping from accel2 to accel1

      ComplexFUInstance* mappedNode = nullptr;
      if(mapping != graphMapping.instanceMap.end()){
         auto iter = map.find(mapping->second);

         Assert(iter != map.end());
         mappedNode = iter->second;

         #if 0
         for(int i = 0; i < mappedNode->name.size; i++){
            if(mappedNode->name[i] == '/'){
               Assert(false);
            }
         }
         #endif

         // If names are equal, nothing to do
         if(!CompareString(inst->name,mappedNode->name)){
            SizedString newName = PushString(&versat->permanent,"%.*s/%.*s",UNPACK_SS(mappedNode->name),UNPACK_SS(inst->name));

            mappedNode->name = newName;
         }
      } else {
         mappedNode = (ComplexFUInstance*) CreateFUInstance(newGraph,inst->declaration,inst->name);
      }
      map.insert({inst,mappedNode});
   }
   #endif

   #if 1
   // Create base edges from accel 1
   for(Edge* edge : accel1->edges){
      ComplexFUInstance* mappedInst = map.find(edge->units[0].inst)->second;
      ComplexFUInstance* mappedOther = map.find(edge->units[1].inst)->second;

      Edge* newEdge = ConnectUnitsGetEdge(mappedInst,edge->units[0].port,mappedOther,edge->units[1].port,edge->delay);
      accel1EdgeMap.push_back(newEdge);
   }
   #endif

   #if 1
   // Create base edges from accel 2, unless they are mapped to edges from accel 1
   for(Edge* edge : accel2->edges){
      PortEdge searchEdge = {};
      searchEdge.units[0] = edge->units[0];
      searchEdge.units[1] = edge->units[1];

      auto iter = graphMapping.edgeMap.find(searchEdge);

      #if 1
      if(iter != graphMapping.edgeMap.end()){
         FUInstance* mappedInst = map.find(edge->units[0].inst)->second;
         FUInstance* mappedOther = map.find(edge->units[1].inst)->second;

         Edge* oldEdge = FindEdge(mappedInst,edge->units[0].port,mappedOther,edge->units[1].port,edge->delay);

         if(oldEdge){ // Edge between port instances might exist but different delay means we need to create another one
            accel2EdgeMap.push_back(oldEdge);

            continue;
         }
      }
      #endif

      FUInstance* mappedInst = map.find(edge->units[0].inst)->second;
      FUInstance* mappedOther = map.find(edge->units[1].inst)->second;

      Edge* newEdge = ConnectUnitsIfNotConnectedGetEdge(mappedInst,edge->units[0].port,mappedOther,edge->units[1].port,edge->delay);
      accel2EdgeMap.push_back(newEdge);
   }
   #endif

   {
      ArenaMarker marker(arena);
      AcceleratorView view = CreateAcceleratorView(newGraph,arena);
      view.CalculateGraphData(arena);
      IsGraphValid(view);
      OutputGraphDotFile(versat,view,true,"debug/Merged.dot");
   }

   {
      ArenaMarker marker(arena);
      AcceleratorView view = CreateAcceleratorView(newGraph,accel1EdgeMap,arena);
      view.CalculateGraphData(arena);
      IsGraphValid(view);
      OutputGraphDotFile(versat,view,true,"debug/Merged1View.dot");
      view.CalculateDelay(arena);
   }

   {
      ArenaMarker marker(arena);
      AcceleratorView view = CreateAcceleratorView(newGraph,accel2EdgeMap,arena);
      view.CalculateGraphData(arena);
      IsGraphValid(view);
      OutputGraphDotFile(versat,view,true,"debug/Merged2View.dot");
   }

   //printf("Nodes mapping size: %d\n",graphMapping.instanceMap.size());

   return newGraph;
}

FUDeclaration* MergeAccelerators(Versat* versat,FUDeclaration* accel1,FUDeclaration* accel2,SizedString name,int flatteningOrder,
                                 MergingStrategy strategy,SpecificMerge* specifics,int nSpecifics){

   Accelerator* flatten1 = Flatten(versat,accel1->baseCircuit,flatteningOrder);
   Accelerator* flatten2 = Flatten(versat,accel2->baseCircuit,flatteningOrder);

   Arena* arena = &versat->temp;
   ArenaMarker marker(arena);

   SpecificMergeNodes* specificNodes = PushArray(arena,nSpecifics,SpecificMergeNodes).data;

   for(int i = 0; i < nSpecifics; i++){
      specificNodes[i].instA = (ComplexFUInstance*) GetInstanceByName(flatten1,specifics[i].instA);
      specificNodes[i].instB = (ComplexFUInstance*) GetInstanceByName(flatten2,specifics[i].instB);
   }

   for(ComplexFUInstance* inst : flatten1->instances){
      ArenaMarker marker(arena);
      AcceleratorView view = SubGraphAroundInstance(versat,flatten1,inst,1,arena);
      view.CalculateGraphData(arena);
      OutputGraphDotFile(versat,view,true,"debug/test.dot");
      break;
   }

   {
      ArenaMarker marker(arena);
      AcceleratorView view = CreateAcceleratorView(flatten1,arena);
      view.CalculateGraphData(arena);
      OutputGraphDotFile(versat,view,true,"debug/flatten1.dot");
   }
   {
      ArenaMarker marker(arena);
      AcceleratorView view = CreateAcceleratorView(flatten2,arena);
      view.CalculateGraphData(arena);
      OutputGraphDotFile(versat,view,true,"debug/flatten2.dot");
   }

   Accelerator* newGraph = MergeAccelerator(versat,flatten1,flatten2,specificNodes,nSpecifics,strategy);

   //printf("No mux or buffers\n");
   //PrintSavedByMerge(versat,newGraph,flatten1,flatten2);

   FUDeclaration* decl = RegisterSubUnit(versat,name,newGraph);

   {
      ArenaMarker marker(arena);
      AcceleratorView view = CreateAcceleratorView(decl->fixedDelayCircuit,arena);
      view.CalculateGraphData(arena);
      OutputGraphDotFile(versat,view,true,"debug/FixedDelay.dot");
   }
   #if 0
   //CalculateDelay(versat,flatten1);
   //CalculateDelay(versat,flatten2);

   printf("\nFixed graphs\n");
   PrintSavedByMerge(versat,decl->fixedDelayCircuit,flatten1,flatten2);
   printf("\n");
   #endif

   //decl->type = FUDeclaration::MERGED;
   decl->mergedType[0] = accel1;
   decl->mergedType[1] = accel2;

   return decl;
}

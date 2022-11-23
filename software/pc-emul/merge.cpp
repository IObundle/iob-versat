#include "merge.hpp"

#include "versatPrivate.hpp"

#include <cstdarg>
#include <unordered_map>

template<> class std::hash<PortEdge>{
   public:
   std::size_t operator()(PortEdge const& s) const noexcept{
      int res = SimpleHash(MakeSizedString((const char*) &s,sizeof(PortEdge)));

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

typedef std::unordered_map<FUInstance*,FUInstance*> InstanceMap;
typedef std::unordered_map<PortEdge,PortEdge> EdgeMap;

struct GraphMapping{
   InstanceMap instanceMap;
   EdgeMap edgeMap;
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

#if 0
static bool NodeMappingConflict(PortEdge edge1,PortEdge edge2){
   ComplexFUInstance* list[4] = {edge1.units[0].inst,edge1.units[1].inst,edge2.units[0].inst,edge2.units[1].inst};
   ComplexFUInstance* instances[4] = {0,0,0,0};

   if(!(edge1.units[0].port == edge2.units[0].port && edge1.units[1].port == edge2.units[1].port)){
      return 0;
   }

   for(int i = 0; i < 4; i++){
      for(int ii = 0; ii < 4; ii++){
         if(instances[ii] == list[i]){
            return true;
         } else if(instances[i] == 0){
            instances[ii] = list[i];
            break;
         }
      }
   }

   return false;
}
#endif

static bool MappingConflict(MappingNode map1,MappingNode map2){
   if(map1.type != map2.type){
      return true;
   }

   if(map1.type == MappingNode::NODE){
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

      return false;
   } else {
      #if 0
      PortInstance m0e0n0 = map1.edges[0].units[0]; // 1A11
      PortInstance m0e1n0 = map1.edges[1].units[0]; // 1A21
      PortInstance m0e0n1 = map1.edges[0].units[1]; // 1B11
      PortInstance m0e1n1 = map1.edges[1].units[1]; // 1B21
      PortInstance m1e0n0 = map2.edges[0].units[0]; // 2A11
      PortInstance m1e1n0 = map2.edges[1].units[0]; // 2A22
      PortInstance m1e0n1 = map2.edges[0].units[1]; // 2B11
      PortInstance m1e1n1 = map2.edges[1].units[1]; // 2B21
      #endif

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

      #define XOR(A,B,C,D) ((A == B && !(C == D)) || (!(A == B) && C == D))

      #if 0
      if(XOR(m0e0n0,m1e0n0,m0e1n0,m1e1n0)){
         return true;
      }
      if(XOR(m0e0n1,m1e0n1,m0e1n1,m1e1n1)){
         return true;
      }
      #endif

      #if 0
      //   A         B           C         D
      if(  m0e0n0 == m1e0n0 && !(m0e1n0 == m1e1n0)){
         return true;
      }
      if(!(m0e0n0 == m1e0n0) &&  m0e1n0 == m1e1n0){
         return true;
      }
      #endif

      #if 0
      bool res = (NodeMappingConflict(map1.edges[0],map1.edges[1]) ||
                 NodeMappingConflict(map1.edges[0],map2.edges[0]) ||
                 NodeMappingConflict(map1.edges[0],map2.edges[1]) ||
                 NodeMappingConflict(map1.edges[1],map2.edges[0]) ||
                 NodeMappingConflict(map1.edges[1],map2.edges[1]) ||
                 NodeMappingConflict(map2.edges[0],map2.edges[1]));
      #endif

      return res;
   }
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
   res.validNodes = PushArray(arena,graph.numberNodes,int);
   Memcpy(res.validNodes,graph.validNodes,graph.numberNodes);

   return res;
}

ConsolidationGraph OnlyNeighbors(ConsolidationGraph graph,int nodeIndex,Arena* arena){
   MappingNode* node = &graph.nodes[nodeIndex];
   ConsolidationGraph neighborsGraph = Copy(graph,arena);

   Memset(neighborsGraph.validNodes,0,neighborsGraph.numberNodes);

   for(int i = 0; i < neighborsGraph.numberEdges; i++){
      MappingEdge edge = neighborsGraph.edges[i];

      if(MappingNodeEqual(*edge.nodes[0],*node)){
         for(int j = 0; j < neighborsGraph.numberNodes; j++){
            if(MappingNodeEqual(*edge.nodes[1],neighborsGraph.nodes[j])){
               neighborsGraph.validNodes[j] = 1;
            }
         }
      } else if(MappingNodeEqual(*edge.nodes[1],*node)){
         for(int j = 0; j < neighborsGraph.numberNodes; j++){
            if(MappingNodeEqual(*edge.nodes[0],neighborsGraph.nodes[j])){
               neighborsGraph.validNodes[j] = 1;
            }
         }
      }
   }

   return neighborsGraph;
}

int NumberNodes(ConsolidationGraph graph){
   int num = 0;
   for(int i = 0; i < graph.numberNodes; i++){
      if(graph.validNodes[i]){
         num += 1;
      }
   }

   return num;
}

int NodeIndex(ConsolidationGraph graph,MappingNode* node){
   int index = node - graph.nodes;
   return index;
}

void PrintValidNodes(ConsolidationGraph graph){
   for(int i = 0; i < graph.numberNodes; i++){
      printf("%d",graph.validNodes[i]);
   }
}

int ValidNodes(ConsolidationGraph graph){
   int count = 0;
   for(int i = 0; i < graph.numberNodes; i++){
      if(graph.validNodes[i]){
         count += 1;
      }
   }

   return count;
}

void PrintNeighbors(ConsolidationGraph graph,int node,bool includeNode,Arena* arena){
   Byte* mark = MarkArena(arena);

   ConsolidationGraph neighbor = OnlyNeighbors(graph,node,arena);
   if(includeNode){
      neighbor.validNodes[node] = 1;
   }

   PrintValidNodes(neighbor);

   PopMark(arena,mark);
}

void AssertIsClique(ConsolidationGraph graph,Arena* arena){
   int cliqueSize = ValidNodes(graph);

   // Zero or one are always cliques
   if(cliqueSize == 0 || cliqueSize == 1){
      return;
   }

   for(int i = 0; i < graph.numberNodes; i++){
      MappingNode* node = &graph.nodes[i];

      if(!graph.validNodes[i]){
         continue;
      }

      int count = 0;
      for(int ii = 0; ii < graph.numberEdges; ii++){
         MappingEdge* edge = &graph.edges[ii];

         // Only count edge if both nodes are valid
         if(edge->nodes[0] == node){
            int index = NodeIndex(graph,edge->nodes[1]);
            if(graph.validNodes[index]){
               count += 1;
            }
         } else if(edge->nodes[1] == node){
            int index = NodeIndex(graph,edge->nodes[0]);
            if(graph.validNodes[index]){
               count += 1;
            }
         }
      }

      if(count != cliqueSize - 1){ // Each node must have (cliqueSize - 1) connections otherwise not a clique
         PrintValidNodes(graph);
         for(int i = 0; i < graph.numberNodes; i++){
            PrintNeighbors(graph,i,true,arena);
            printf(" %d\n",i);
         }
         Assert(false);
      }
   }
}

#define TABLE_SIZE (1024*8)

static int max = 0;
static bool found = false;
static int table[TABLE_SIZE];
static ConsolidationGraph clique;

struct IndexRecord{
   int index;
   IndexRecord* next;
};

void InsersectGraphs(ConsolidationGraph* dest,ConsolidationGraph intersect){
   Assert(dest->numberNodes == intersect.numberNodes);
   for(int i = 0; i < intersect.numberNodes; i++){
      dest->validNodes[i] &= intersect.validNodes[i];
   }
}

void Clique(ConsolidationGraph graphArg,IndexRecord* record,int size,Arena* arena){
   ConsolidationGraph graph = Copy(graphArg,arena);

   if(NumberNodes(graph) == 0){
      if(size > max){
         max = size;

         // Record best clique found so far
         for(int i = 0; i < clique.numberNodes; i++){
            clique.validNodes[i] = 0;
         }
         for(IndexRecord* ptr = record; ptr != nullptr; ptr = ptr->next){
            clique.validNodes[ptr->index] = 1;
         }

         found = true;
      }
      return;
   }

   int num = 0;
   while((num = NumberNodes(graph)) != 0){
      if(size + num <= max){
         return;
      }

      int i;
      for(i = 0; i < graph.numberNodes; i++){
         if(graph.validNodes[i]){
            break;
         }
      }

      if(size + table[i] <= max){
         return;
      }

      graph.validNodes[i] = 0;

      Byte* mark = MarkArena(arena);
      ConsolidationGraph tempGraph = Copy(graph,arena);

      ConsolidationGraph neighbors = OnlyNeighbors(tempGraph,i,arena);
      InsersectGraphs(&tempGraph,neighbors);

      IndexRecord newRecord = {};
      newRecord.index = i;
      newRecord.next = record;

      Clique(tempGraph,&newRecord,size + 1,arena);

      PopMark(arena,mark);

      if(found == true){
         return;
      }
   }
}

ConsolidationGraph MaxClique(ConsolidationGraph graph,Arena* arena){
   max = 0;
   for(int i = 0; i < TABLE_SIZE; i++){
      table[i] = 0;
   }
   Assert(graph.numberNodes < TABLE_SIZE);

   // Preserve nodes and edges, but allocates different valid nodes
   clique = Copy(graph,arena);

   for(int i = graph.numberNodes - 1; i >= 0; i--){
      Byte* mark = MarkArena(arena);

      found = false;
      for(int j = 0; j < graph.numberNodes; j++){
         graph.validNodes[j] = (i <= j ? 1 : 0);
      }

      ConsolidationGraph neighbors = OnlyNeighbors(graph,i,arena);
      InsersectGraphs(&graph,neighbors);

      IndexRecord record = {};
      record.index = i;

      Clique(graph,&record,1,arena);
      table[i] = max;

      //printf("T:%d %d\n",i,table[i]);

      PopMark(arena,mark);
   }

   AssertIsClique(clique,arena);

   return clique;
}

FUInstance* NodeMapped(FUInstance* inst, ConsolidationGraph graph){
   for(int i = 0; i < graph.numberNodes; i++){
      MappingNode node = graph.nodes[i];

      if(!graph.validNodes[i]){
         continue;
      }

      if(node.type == MappingNode::NODE){
         if(node.nodes.instances[0] == inst){
            return node.nodes.instances[1];
         }
         if(node.nodes.instances[1] == inst){
            return node.nodes.instances[0];
         }
      } else { // Edge mapping
         if(node.edges[0].units[0].inst == inst){
            return node.edges[1].units[0].inst;
         }
         if(node.edges[0].units[1].inst == inst){
            return node.edges[1].units[1].inst;
         }
         if(node.edges[1].units[0].inst == inst){
            return node.edges[0].units[0].inst;
         }
         if(node.edges[1].units[1].inst == inst){
            return node.edges[0].units[1].inst;
         }
      }
   }

   return nullptr;
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

ConsolidationGraph GenerateConsolidationGraph(Versat* versat,Arena* arena,Accelerator* accel1,Accelerator* accel2){
   ConsolidationGraph graph = {};

   graph.nodes = (MappingNode*) MarkArena(arena);

   // Check node mapping
   #if 0
   for(ComplexFUInstance* instA : accel1->instances){
      for(ComplexFUInstance* instB : accel2->instances){
         if(instA->declaration == instB->declaration){
            MappingNode* node = PushStruct(arena,MappingNode);

            node->type = MappingNode::NODE;
            node->nodes.instances[0] = instA;
            node->nodes.instances[1] = instB;

            graph.numberNodes += 1;
         }
      }
   }
   #endif

   #if 1
   // Check possible edge mapping
   for(Edge* edge1 : accel1->edges){
      for(Edge* edge2 : accel2->edges){
         // TODO: some nodes do not care about which port is connected (think the inputs for common operations, like multiplication, adders and the likes)
         // Can augment the algorithm further to find more mappings
         if(EqualPortMapping(versat,edge1->units[0],edge2->units[0]) &&
            EqualPortMapping(versat,edge1->units[1],edge2->units[1])){
            MappingNode* node = PushStruct(arena,MappingNode);

            node->type = MappingNode::EDGE;

            node->edges[0].units[0] = edge1->units[0];
            node->edges[0].units[1] = edge1->units[1];
            node->edges[1].units[0] = edge2->units[0];
            node->edges[1].units[1] = edge2->units[1];

            graph.numberNodes += 1;
         }
      }
   }
   #endif

   #if 1
   graph.edges = (MappingEdge*) MarkArena(arena);
   int times = 0;
   // Check edge conflicts
   for(int i = 0; i < graph.numberNodes; i++){
      for(int ii = 0; ii < i; ii++){
         times += 1;

         MappingNode* node1 = &graph.nodes[i];
         MappingNode* node2 = &graph.nodes[ii];

         if(MappingConflict(*node1,*node2)){
            continue;
         }

         #if 1
         MappingEdge* edge = PushStruct(arena,MappingEdge);

         edge->nodes[0] = node1;
         edge->nodes[1] = node2;
         #endif

         graph.numberEdges += 1;
      }
   }
   #endif

   graph.validNodes = (int*) PushArray(arena,graph.numberNodes,int);
   memset(graph.validNodes,1,sizeof(int) * graph.numberNodes);

   return graph;
}

SizedString PortInstanceIdentifier(PortInstance port,Arena* memory){
   FUInstance* inst = port.inst;
   FUDeclaration* decl = inst->declaration;

   SizedString res = PushString(memory,"%.*s_%d:%.*s",UNPACK_SS(inst->name),port.port,UNPACK_SS(decl->name));
   return res;
}

SizedString MappingNodeIdentifier(MappingNode* node,Arena* memory){
   SizedString name = {};
   if(node->type == MappingNode::NODE){
      FUInstance* n0 = node->nodes.instances[0];
      FUInstance* n1 = node->nodes.instances[1];

      name = PushString(memory,"%.*s -- %.*s",UNPACK_SS(n0->name),UNPACK_SS(n1->name));
   } else if(node->type == MappingNode::EDGE){
      PortEdge e0 = node->edges[0];
      PortEdge e1 = node->edges[1];

      Byte* mark = MarkArena(memory);
      PortInstanceIdentifier(e0.units[0],memory);
      PushString(memory," -- ");
      PortInstanceIdentifier(e0.units[1],memory);
      PushString(memory,"/");
      PortInstanceIdentifier(e1.units[0],memory);
      PushString(memory," -- ");
      PortInstanceIdentifier(e1.units[1],memory);
      name = PointArena(memory,mark);
   } else {
      NOT_IMPLEMENTED;
   }

   return name;
}

static void OutputConsolidationGraph(ConsolidationGraph graph,Arena* memory,bool onlyOutputValid,const char* format,...){
   va_list args;
   va_start(args,format);

   SizedString fileName = vPushString(memory,format,args);
   PushNullByte(memory);

   va_end(args);

   FILE* outputFile = fopen(fileName.str,"w");

   fprintf(outputFile,"graph GraphName {\n\tnode [fontcolor=white,style=filled,color=\"160,60,176\"];\n");
   for(int i = 0; i < graph.numberNodes; i++){
      MappingNode* node = &graph.nodes[i];

      if(onlyOutputValid && !graph.validNodes[i]){
         continue;
      }

      fprintf(outputFile,"\t\"%.*s\";\n",UNPACK_SS(MappingNodeIdentifier(node,memory)));
   }

   for(int i = 0; i < graph.numberEdges; i++){
      MappingEdge* edge = &graph.edges[i];

      if(onlyOutputValid){
         int node0 = NodeIndex(graph,edge->nodes[0]);
         int node1 = NodeIndex(graph,edge->nodes[1]);

         if(!(graph.validNodes[node0] && graph.validNodes[node1])){
            continue;
         }
      }

      SizedString node1 = MappingNodeIdentifier(edge->nodes[0],memory);
      SizedString node2 = MappingNodeIdentifier(edge->nodes[1],memory);

      fprintf(outputFile,"\t\"%.*s\" -- \"%.*s\";\n",UNPACK_SS(node1),UNPACK_SS(node2));
   }

   fprintf(outputFile,"}\n");
   fclose(outputFile);
}

static GraphMapping ConsolidationGraphMapping(Versat* versat,Arena* arena,Accelerator* accel1,Accelerator* accel2){
   ConsolidationGraph graph = GenerateConsolidationGraph(versat,arena,accel1,accel2);
   ConsolidationGraph clique = MaxClique(graph,arena);

   GraphMapping res;

   for(int i = 0; i < clique.numberNodes; i++){
      MappingNode node = clique.nodes[i];

      if(!clique.validNodes[i]){
         continue;
      }

      if(node.type == MappingNode::NODE){
         MergeEdge& nodes = node.nodes;

         res.instanceMap.insert({nodes.instances[1],nodes.instances[0]});
      } else { // Edge mapping
         PortEdge& edge0 = node.edges[0]; // Edge in graph 1
         PortEdge& edge1 = node.edges[1]; // Edge in graph 2

         res.instanceMap.insert({edge1.units[0].inst,edge0.units[0].inst});
         res.instanceMap.insert({edge1.units[1].inst,edge0.units[1].inst});

         res.edgeMap.insert({edge1,edge0});
      }
   }

   return res;
}

static InstanceMap FirstFitGraphMapping(Versat* versat,Accelerator* accel1,Accelerator* accel2,Arena* arena){
   InstanceMap res;

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
            res.insert({inst2,inst1});
            inst1->tag = 1;
            inst2->tag = 1;
            break;
         }
      }
   }

   return res;
}

#include <algorithm>

struct {
   bool operator()(ComplexFUInstance* f1, ComplexFUInstance* f2) const {
      bool res = false;
      if(f1->tempData->order == f2->tempData->order){
         res = (f1->declaration < f2->declaration);
      } else {
         res = (f1->tempData->order < f2->tempData->order);
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

      int val1 = inst1->tempData->order;
      int val2 = inst1->tempData->order;

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

static InstanceMap OrderedFitGraphMapping(Versat* versat,Accelerator* accel1,Accelerator* accel2){
   LockAccelerator(accel1,Accelerator::Locked::ORDERED);
   LockAccelerator(accel2,Accelerator::Locked::ORDERED);

   InstanceMap map;

   for(ComplexFUInstance* inst : accel1->instances){
      inst->tag = 0;
   }

   for(ComplexFUInstance* inst : accel2->instances){
      inst->tag = 0;
   }

   int minIterations = mini(accel1->instances.Size(),accel2->instances.Size());

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
      for(ComplexFUInstance* inst : accel1->instances){
         if(!inst->tag){
            order1.push_back(inst);
         }
      }
      for(ComplexFUInstance* inst : accel2->instances){
         if(!inst->tag){
            order2.push_back(inst);
         }
      }

      std::sort(order1.begin(),order1.end(),compare);
      std::sort(order2.begin(),order2.end(),compare);

      OrderedMatch(order1,order2,map,i);
   }

   return map;
}

ConsolidationGraph GenerateOrderedConsolidationGraph(Versat* versat,Arena* arena,Accelerator* accel1,Accelerator* accel2,int order){
   ConsolidationGraph graph = {};

   graph.nodes = (MappingNode*) MarkArena(arena);

   // Check node mapping
   #if 1
   for(ComplexFUInstance* instA : accel1->instances){
      if(instA->tempData->order != order){
         continue;
      }

      for(ComplexFUInstance* instB : accel2->instances){
         if(instB->tempData->order != order){
            continue;
         }

         if(instA->declaration == instB->declaration){
            MappingNode* node = PushStruct(arena,MappingNode);

            node->type = MappingNode::NODE;
            node->nodes.instances[0] = instA;
            node->nodes.instances[1] = instB;

            graph.numberNodes += 1;
         }
      }
   }
   #endif

   #if 1
   // Check possible edge mapping
   for(Edge* edge1 : accel1->edges){
      if(!(edge1->units[0].inst->tempData->order == order || edge1->units[1].inst->tempData->order == order)){
         continue;
      }

      for(Edge* edge2 : accel2->edges){
         if(!(edge2->units[0].inst->tempData->order == order || edge2->units[1].inst->tempData->order == order)){
            continue;
         }

         // TODO: some nodes do not care about which port is connected (think the inputs for common operations, like multiplication, adders and the likes)
         // Can augment the algorithm further to find more mappings
         if(EqualPortMapping(versat,edge1->units[0],edge2->units[0]) &&
            EqualPortMapping(versat,edge1->units[1],edge2->units[1])){
            MappingNode* node = PushStruct(arena,MappingNode);

            node->type = MappingNode::EDGE;

            node->edges[0].units[0] = edge1->units[0];
            node->edges[0].units[1] = edge1->units[1];
            node->edges[1].units[0] = edge2->units[0];
            node->edges[1].units[1] = edge2->units[1];

            graph.numberNodes += 1;
         }
      }
   }
   #endif

   #if 1
   graph.edges = (MappingEdge*) MarkArena(arena);
   int times = 0;
   // Check edge conflicts
   for(int i = 0; i < graph.numberNodes; i++){
      for(int ii = 0; ii < i; ii++){
         times += 1;

         MappingNode* node1 = &graph.nodes[i];
         MappingNode* node2 = &graph.nodes[ii];

         if(MappingConflict(*node1,*node2)){
            continue;
         }

         #if 1
         MappingEdge* edge = PushStruct(arena,MappingEdge);

         edge->nodes[0] = node1;
         edge->nodes[1] = node2;
         #endif

         graph.numberEdges += 1;
      }
   }
   #endif

   graph.validNodes = (int*) PushArray(arena,graph.numberNodes,int);
   Memset(graph.validNodes,1,graph.numberNodes);

   return graph;
}

GraphMapping TestingGraphMapping(Versat* versat,Arena* arena,Accelerator* accel1,Accelerator* accel2){
   LockAccelerator(accel1,Accelerator::Locked::ORDERED);
   LockAccelerator(accel2,Accelerator::Locked::ORDERED);

   int maxOrder1 = 0;
   for(ComplexFUInstance* inst : accel1->instances){
      maxOrder1 = maxi(maxOrder1,inst->tempData->order);
   }
   int maxOrder2 = 0;
   for(ComplexFUInstance* inst : accel2->instances){
      maxOrder2 = maxi(maxOrder2,inst->tempData->order);
   }

   int maxOrder = mini(maxOrder1,maxOrder2);

   GraphMapping res;
   for(int i = 0; i < maxOrder - 1; i += 2){
      ConsolidationGraph graph = GenerateOrderedConsolidationGraph(versat,arena,accel1,accel2,i);

      ConsolidationGraph clique = MaxClique(graph,arena);

      //printf("%d %d\n",i,NumberNodes(clique));

      for(int i = 0; i < clique.numberNodes; i++){
         MappingNode node = clique.nodes[i];

         if(!clique.validNodes[i]){
            continue;
         }

         if(node.type == MappingNode::NODE){
            MergeEdge& nodes = node.nodes;

            res.instanceMap.insert({nodes.instances[1],nodes.instances[0]});
         } else { // Edge mapping
            PortEdge& edge0 = node.edges[0]; // Edge in graph 1
            PortEdge& edge1 = node.edges[1]; // Edge in graph 2

            res.instanceMap.insert({edge1.units[0].inst,edge0.units[0].inst});
            res.instanceMap.insert({edge1.units[1].inst,edge0.units[1].inst});

            res.edgeMap.insert({edge1,edge0});
         }
      }

      #if 0
      printf("%d %d %d\n",i,graph.numberNodes,graph.numberEdges);
      #endif
   }

   return res;
}

void PrintAcceleratorStats(Accelerator* accel){
   printf("Instances:%d Edges:%d",accel->instances.Size(),accel->edges.Size());
}

void PrintSavedByMerge(Accelerator* finalAccel,Accelerator* accel1,Accelerator* accel2){
   printf("Accel1: ");
   PrintAcceleratorStats(accel1);
   printf("\nAccel2: ");
   PrintAcceleratorStats(accel2);
   printf("\nFinal: ");
   PrintAcceleratorStats(finalAccel);
   printf("\n");

   int nonMergedNodes = accel1->instances.Size() + accel2->instances.Size();
   int nonMergedEdges = accel1->edges.Size() + accel2->edges.Size();

   printf("NonMergedNodes: %d\n",nonMergedNodes);
   printf("NonMergedEdges: %d\n",nonMergedEdges);

   float savedNodes = (((float) nonMergedNodes) / ((float) finalAccel->instances.Size())) - 1.0f;
   float savedEdges = (((float) nonMergedEdges) / ((float) finalAccel->edges.Size())) - 1.0f;

   printf("SavedNodes: %.2f\n",savedNodes * 100.0f);
   printf("SavedEdges: %.2f\n",savedEdges * 100.0f);
}

FUDeclaration* MergeAccelerators(Versat* versat,FUDeclaration* accel1,FUDeclaration* accel2,SizedString name,int flatteningOrder){
   Assert(accel1->type == FUDeclaration::COMPOSITE && accel2->type == FUDeclaration::COMPOSITE);

   Arena* arena = &versat->temp;
   Byte* mark = MarkArena(&versat->temp);

   Accelerator* flatten1 = Flatten(versat,accel1->baseCircuit,flatteningOrder);
   Accelerator* flatten2 = Flatten(versat,accel2->baseCircuit,flatteningOrder);

   GraphMapping graphMapping;

   #if 1
   graphMapping = TestingGraphMapping(versat,arena,flatten1,flatten2);
   #else
   graphMapping = ConsolidationGraphMapping(versat,arena,flatten1,flatten2);
   #endif

   #if 0
   if(true){
      graphMapping = OrderedFitGraphMapping(versat,flatten1,flatten2);
   } else if(flatten1->instances.Size() >= 500 || flatten2->instances.Size() >= 500){
      graphMapping = FirstFitGraphMapping(versat,flatten1,flatten2,arena);
   } else {
      graphMapping = ConsolidationGraphMapping(arena,flatten1,flatten2);
   }
   #endif

   PopMark(arena,mark);

   #if 0
   for(auto mapping : graphMapping.instanceMap){
      printf("%.*s -- %.*s\n",UNPACK_SS(mapping.first->name),UNPACK_SS(mapping.second->name));
   }

   printf("Edge:%d Instance:%d\n",graphMapping.edgeMap.size(),graphMapping.instanceMap.size());
   #endif

   if(flatten2->outputInstance && flatten1->outputInstance){
      graphMapping.instanceMap.insert({flatten2->outputInstance,flatten1->outputInstance});
   }

   InstanceMap map = {};

   Accelerator* newGraph = CreateAccelerator(versat);

   // Create base instances from accel 1
   for(FUInstance* inst : flatten1->instances){
      FUInstance* newNode = CreateFUInstance(newGraph,inst->declaration,inst->name);

      map.insert({inst,newNode});
   }

   #if 1
   // Create base instances from accel 2, unless they are mapped to nodes from accel 1
   for(FUInstance* inst : flatten2->instances){
      auto mapping = graphMapping.instanceMap.find(inst); // Returns mapping from flatten2 to flatten1

      if(mapping != graphMapping.instanceMap.end()){
         FUInstance* mappedNode = map.find(mapping->second)->second;

         // If names are equal, nothing to do
         if(!CompareString(inst->name,mappedNode->name)){
            SizedString newName = PushString(&versat->permanent,"%.*s/%.*s",UNPACK_SS(mappedNode->name),UNPACK_SS(inst->name));

            mappedNode->name = newName;
         }

         map.insert({inst,mappedNode});
      } else {
         FUInstance* mappedNode = CreateFUInstance(newGraph,inst->declaration,inst->name);
         map.insert({inst,mappedNode});
      }
   }
   #endif

   #if 1
   // Create base edges from accel 1
   for(Edge* edge : flatten1->edges){
      FUInstance* mappedInst = map.find(edge->units[0].inst)->second;
      FUInstance* mappedOther = map.find(edge->units[1].inst)->second;

      ConnectUnits(mappedInst,edge->units[0].port,mappedOther,edge->units[1].port);
   }
   #endif

   #if 1
   // Create base edges from accel 2, unless they are mapped to edges from accel 1
   for(Edge* edge : flatten2->edges){
      PortEdge searchEdge = {};
      searchEdge.units[0] = edge->units[0];
      searchEdge.units[1] = edge->units[1];

      auto iter = graphMapping.edgeMap.find(searchEdge);

      #if 1
      if(iter != graphMapping.edgeMap.end()){
         continue;
      }
      #endif

      FUInstance* mappedInst = map.find(edge->units[0].inst)->second;
      FUInstance* mappedOther = map.find(edge->units[1].inst)->second;

      ConnectUnitsIfNotConnected(mappedInst,edge->units[0].port,mappedOther,edge->units[1].port);
   }
   #endif

   Assert(IsGraphValid(newGraph));
   LockAccelerator(newGraph,Accelerator::Locked::FREE);

   FUDeclaration* decl = RegisterSubUnit(versat,name,newGraph);

   OutputGraphDotFile(versat,newGraph,false,"debug/Merged.dot");
   OutputGraphDotFile(versat,decl->fixedDelayCircuit,false,"debug/FixedDelay.dot");

   #if 0
   CalculateDelay(versat,flatten1);
   CalculateDelay(versat,flatten2);

   PrintSavedByMerge(newGraph,flatten1,flatten2);
   #endif

   //decl->type = FUDeclaration::MERGED;
   decl->mergedType[0] = accel1;
   decl->mergedType[1] = accel2;

   return decl;
}

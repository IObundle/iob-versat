#include "merge.hpp"

#include "versatPrivate.hpp"

#include <unordered_map>

typedef std::unordered_map<FUInstance*,FUInstance*> InstanceMap;

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
      bool res = (NodeMappingConflict(map1.edges[0],map1.edges[1]) ||
                 NodeMappingConflict(map1.edges[0],map2.edges[0]) ||
                 NodeMappingConflict(map1.edges[0],map2.edges[1]) ||
                 NodeMappingConflict(map1.edges[1],map2.edges[0]) ||
                 NodeMappingConflict(map1.edges[1],map2.edges[1]) ||
                 NodeMappingConflict(map2.edges[0],map2.edges[1]));

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

void OnlyNeighbors(ConsolidationGraph* graph,int validNode){
   int neighbors[1024];

   MappingNode* node = &graph->nodes[validNode];

   for(int i = 0; i < 1024; i++){
      neighbors[i] = 0;
   }

   for(int i = 0; i < graph->numberEdges; i++){
      MappingEdge* edge = &graph->edges[i];

      if(MappingNodeEqual(edge->nodes[0],*node)){
         for(int j = 0; j < graph->numberNodes; j++){
            if(MappingNodeEqual(edge->nodes[1],graph->nodes[j])){
               neighbors[j] = 1;
            }
         }
      }
      if(MappingNodeEqual(edge->nodes[1],*node)){
         for(int j = 0; j < graph->numberNodes; j++){
            if(MappingNodeEqual(edge->nodes[0],graph->nodes[j])){
               neighbors[j] = 1;
            }
         }
      }
   }

   for(int i = 0; i < graph->numberNodes; i++){
      graph->validNodes[i] &= neighbors[i];
   }
}

ConsolidationGraph Copy(ConsolidationGraph graph){
   ConsolidationGraph res = {};

   NOT_IMPLEMENTED;

   #if 0
   res.nodes = (MappingNode*) calloc(graph.numberNodes,sizeof(MappingNode));
   memcpy(res.nodes,graph.nodes,sizeof(MappingNode) * graph.numberNodes);
   res.numberNodes = graph.numberNodes;

   res.edges = (MappingEdge*) calloc(graph.numberEdges,sizeof(MappingEdge));
   memcpy(res.edges,graph.edges,sizeof(MappingEdge) * graph.numberEdges);
   res.numberEdges = graph.numberEdges;

   res.validNodes = (int*) calloc(graph.numberNodes,sizeof(int));
   memcpy(res.validNodes,graph.validNodes,sizeof(int) * graph.numberNodes);
   #endif

   return res;
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

#define TABLE_SIZE (1024*8)

static int max = 0;
static int found = 0;
static int table[TABLE_SIZE];
static ConsolidationGraph clique;

void Clique(ConsolidationGraph graphArg,int size){
   if(NumberNodes(graphArg) == 0){
      if(size > max){
         max = size;
         clique = graphArg;
         found = true;
      }
      return;
   }

   ConsolidationGraph graph = graphArg;

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

      OnlyNeighbors(&graph,i);
      Clique(graph,size + 1);
      if(found == true){
         return;
      }
   }
}

ConsolidationGraph MaxClique(ConsolidationGraph graph){
   max = 0;
   found = 0;
   for(int i = 0; i < TABLE_SIZE; i++){
      table[i] = 0;
   }

   Assert(graph.validNodes);
   //graph.validNodes = (int*) calloc(sizeof(int),graph.numberNodes);
   for(int i = graph.numberNodes - 1; i >= 0; i--){
      for(int j = 0; j < graph.numberNodes; j++){
         graph.validNodes[j] = (i <= j ? 1 : 0);
         printf("%d ",graph.validNodes[j]);
      }
      printf("\n");

      OnlyNeighbors(&graph,i);
      graph.validNodes[i] = 1;

      #if 1
      for(int j = 0; j < graph.numberNodes; j++){
         printf("%d ",graph.validNodes[j]);
      }
      printf("\n");
      printf("\n");
      #endif

      Clique(graph,1);

      table[i] = max;
   }

   return graph;
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

ConsolidationGraph GenerateConsolidationGraph(Arena* arena,Accelerator* accel1,Accelerator* accel2){
   ConsolidationGraph graph = {};

   graph.nodes = (MappingNode*) MarkArena(arena);

   // Check node mapping
   #if 1
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
         if(edge1->units[0].inst->declaration == edge2->units[0].inst->declaration
         && edge1->units[1].inst->declaration == edge2->units[1].inst->declaration
         && edge1->units[0].port == edge2->units[0].port
         && edge1->units[1].port == edge2->units[1].port) {
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

         MappingNode node1 = graph.nodes[i];
         MappingNode node2 = graph.nodes[ii];

         if(MappingConflict(node1,node2)){
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
   memset(graph.validNodes,0,sizeof(int) * graph.numberNodes);

   return graph;
}

SizedString MappingNodeIdentifier(MappingNode* node,Arena* memory){
   SizedString name = {};
   if(node->type == MappingNode::NODE){
      FUInstance* n0 = node->nodes.instances[0];
      FUInstance* n1 = node->nodes.instances[1];

      name = PushString(memory,"A%.*s_B%.*s",UNPACK_SS(n0->name),UNPACK_SS(n1->name));
   } else if(node->type == MappingNode::EDGE){
      PortEdge e0 = node->edges[0];
      PortEdge e1 = node->edges[1];

      SizedString e00 = e0.units[0].inst->name;
      SizedString e01 = e0.units[1].inst->name;
      SizedString e10 = e1.units[0].inst->name;
      SizedString e11 = e1.units[1].inst->name;

      int p00 = e0.units[0].port;
      int p01 = e0.units[1].port;
      int p10 = e1.units[0].port;
      int p11 = e1.units[1].port;

      name = PushString(memory,"A%.*s_%d_%.*s_%d_B%.*s_%d_%.*s_%d",UNPACK_SS(e00),p00,UNPACK_SS(e01),p01,UNPACK_SS(e10),p10,UNPACK_SS(e11),p11);
   } else {
      NOT_IMPLEMENTED;
   }

   return name;
}

static void OutputConsolidationGraph(ConsolidationGraph graph,Arena* memory){
   FILE* outputFile = fopen("ConsolidationGraph.dot","w");

   fprintf(outputFile,"graph consolidationGraph {\n\tnode [fontcolor=white,style=filled,color=\"160,60,176\"];\n");
   for(int i = 0; i < graph.numberNodes; i++){
      MappingNode* node = &graph.nodes[i];

      fprintf(outputFile,"\t%.*s;\n",UNPACK_SS(MappingNodeIdentifier(node,memory)));
   }

   for(int i = 0; i < graph.numberEdges; i++){
      MappingEdge* edge = &graph.edges[i];

      SizedString node1 = MappingNodeIdentifier(&edge->nodes[0],memory);
      SizedString node2 = MappingNodeIdentifier(&edge->nodes[1],memory);

      fprintf(outputFile,"\t%.*s -- %.*s;\n",UNPACK_SS(node1),UNPACK_SS(node2));
   }

   fprintf(outputFile,"}\n");
   fclose(outputFile);
}

static InstanceMap ConsolidationGraphMapping(Versat* versat,Accelerator* accel1,Accelerator* accel2,Arena* arena){
   ConsolidationGraph graph = GenerateConsolidationGraph(arena,accel1,accel2);

   OutputConsolidationGraph(graph,arena);

   ConsolidationGraph clique = MaxClique(graph);

   InstanceMap res;

   for(int i = 0; i < clique.numberNodes; i++){
      MappingNode node = clique.nodes[i];

      if(!clique.validNodes[i]){
         continue;
      }

      if(node.type == MappingNode::NODE){
         MergeEdge& nodes = node.nodes;

         res.insert({nodes.instances[1],nodes.instances[0]});
      } else { // Edge mapping
         PortEdge& edge0 = node.edges[0]; // Edge in graph 1
         PortEdge& edge1 = node.edges[1]; // Edge in graph 2

         res.insert({edge1.units[0].inst,edge0.units[0].inst});
         res.insert({edge1.units[1].inst,edge0.units[1].inst});
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

FUDeclaration* MergeAccelerators(Versat* versat,FUDeclaration* accel1,FUDeclaration* accel2,SizedString name){
   Assert(accel1->type == FUDeclaration::COMPOSITE && accel2->type == FUDeclaration::COMPOSITE);

   Arena* arena = &versat->temp;
   Byte* mark = MarkArena(&versat->temp);

   Accelerator* flatten1 = Flatten(versat,accel1->baseCircuit,99);
   Accelerator* flatten2 = Flatten(versat,accel2->baseCircuit,99);

   InstanceMap graphMapping;
   if(true){
      graphMapping = OrderedFitGraphMapping(versat,flatten1,flatten2);
   } else if(flatten1->instances.Size() >= 500 || flatten2->instances.Size() >= 500){
      graphMapping = FirstFitGraphMapping(versat,flatten1,flatten2,arena);
   } else {
      graphMapping = ConsolidationGraphMapping(versat,flatten1,flatten2,arena);
   }

   PopMark(arena,mark);

   InstanceMap map = {};

   Accelerator* newGraph = CreateAccelerator(versat);
   //newGraph->type = Accelerator::CIRCUIT;

   // Create base instances from accel 1
   for(FUInstance* inst : flatten1->instances){
      FUInstance* newNode = CreateFUInstance(newGraph,inst->declaration,inst->name);

      map.insert({inst,newNode});
   }

   #if 1
   // Create base instances from accel 2, unless they are mapped to nodes from accel 1
   for(FUInstance* inst : flatten2->instances){
      auto mapping = graphMapping.find(inst); // Returns mapping from flatten2 to flatten1

      if(mapping != graphMapping.end()){
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
   for(Edge* edge : flatten1->edges){
      FUInstance* mappedInst = map.find(edge->units[0].inst)->second;
      FUInstance* mappedOther = map.find(edge->units[1].inst)->second;

      ConnectUnits(mappedInst,edge->units[0].port,mappedOther,edge->units[1].port);
   }
   #endif

   #if 1
   // Add edges from accel 2
   for(Edge* edge : flatten2->edges){
      FUInstance* mappedInst = map.find(edge->units[0].inst)->second;
      FUInstance* mappedOther = map.find(edge->units[1].inst)->second;

      ConnectUnits(mappedInst,edge->units[0].port,mappedOther,edge->units[1].port);
   }
   #endif

   Assert(IsGraphValid(newGraph));

   OutputGraphDotFile(versat,newGraph,false,"Merged.dot");

   FUDeclaration* decl = RegisterSubUnit(versat,name,newGraph);

   //decl->type = FUDeclaration::MERGED;
   decl->mergedType[0] = accel1;
   decl->mergedType[1] = accel2;

   return decl;
}

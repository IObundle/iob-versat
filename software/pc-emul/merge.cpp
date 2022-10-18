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

static int max = 0;
static int found = 0;
static int table[1024];
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
   ConsolidationGraph res = {};

   max = 0;
   found = 0;
   for(int i = 0; i < 1024; i++){
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

      for(int j = 0; j < graph.numberNodes; j++){
         printf("%d ",graph.validNodes[j]);
      }
      printf("\n");
      printf("\n");

      Clique(graph,1);

      table[i] = max;
   }

   return graph;
}

void AddMapping(Mapping* mappings, FUInstance* source,FUInstance* sink){
   for(int i = 0; i < 1024; i++){
      if(mappings[i].source == nullptr){
         mappings[i].source = source;
         mappings[i].sink = sink;
         return;
      }
   }
}

Mapping* GetMapping(Mapping* mappings, FUInstance* source){
   for(int i = 0; i < 1024; i++){
      if(mappings[i].source == source){
         return &mappings[i];
      }
   }

   return nullptr;
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

#if 0
Accelerator* MergeGraphs(Versat* versat,Accelerator* accel1,Accelerator* accel2,ConsolidationGraph graph){
   Mapping* graphToFinal = (Mapping*) calloc(sizeof(Mapping),1024);

   InstanceMap map = {};

   Accelerator* newGraph = CreateAccelerator(versat);

   //LockAccelerator(accel1);
   //LockAccelerator(accel2);

   // Create base instances (accel 1)
   for(ComplexFUInstance* inst : accel1->instances){
      ComplexFUInstance* newNode = CreateFUInstance(newGraph,inst->declaration);

      map.insert({inst,newNode});
   }

   #if 1
   for(ComplexFUInstance* inst : accel2->instances){
      ComplexFUInstance* mappedNode = NodeMapped(inst,graph); // Returns node in graph 1

      if(mappedNode){
         mappedNode = map.find(mappedNode)->second;
      } else {
         mappedNode = CreateFUInstance(newGraph,inst->declaration);
      }

      AddMapping(graphToFinal,inst,mappedNode);
   }
   #endif

   // Add edges from accel1
   for(ComplexFUInstance* inst : accel1->instances){
      for(int ii = 0; ii < inst->tempData->numberOutputs; ii++){
         ComplexFUInstance* other = inst->tempData->outputs[ii].inst.inst;

         ComplexFUInstance* mappedInst = map.find(inst)->second;
         ComplexFUInstance* mappedOther = map.find(other)->second;

         //PortEdge portEdge = GetPort(inst,other);

         //ConnectUnits(mappedInst,portEdge.outPort,mappedOther,portEdge.inPort);
      }
   }

   #if 1
   for(ComplexFUInstance* inst : accel2->instances){
      for(int ii = 0; ii < inst->tempData->numberOutputs; ii++){
         ComplexFUInstance* other = inst->tempData->outputs[ii].inst.inst;

         ComplexFUInstance* mappedInst =  map.find(inst)->second;
         ComplexFUInstance* mappedOther = map.find(other)->second;

         //PortEdge portEdge = GetPort(inst,other);

         //ConnectUnits(mappedInst,portEdge.outPort,mappedOther,portEdge.inPort);
      }
   }
   #endif

   return newGraph;
}
#endif

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

   graph.edges = (MappingEdge*) MarkArena(arena);
   // Check edge conflicts
   for(int i = 0; i < graph.numberNodes; i++){
      for(int ii = 0; ii < graph.numberNodes; ii++){
         if(i >= ii){
            continue;
         }

         MappingNode node1 = graph.nodes[i];
         MappingNode node2 = graph.nodes[ii];

         if(MappingConflict(node1,node2)){
            continue;
         }

         MappingEdge* edge = PushStruct(arena,MappingEdge);

         edge->nodes[0] = node1;
         edge->nodes[1] = node2;

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

      name = PushString(memory,"A%s_B%s",n0->name.str,n1->name.str);
   } else if(node->type == MappingNode::EDGE){
      PortEdge e0 = node->edges[0];
      PortEdge e1 = node->edges[1];

      char* e00 = e0.units[0].inst->name.str;
      char* e01 = e0.units[1].inst->name.str;
      char* e10 = e1.units[0].inst->name.str;
      char* e11 = e1.units[1].inst->name.str;

      int p00 = e0.units[0].port;
      int p01 = e0.units[1].port;
      int p10 = e1.units[0].port;
      int p11 = e1.units[1].port;

      name = PushString(memory,"A%s_%d_%s_%d_B%s_%d_%s_%d",e00,p00,e01,p01,e10,p10,e11,p11);
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

   #if 0
   std::set<std::pair<ComplexFUInstance*,ComplexFUInstance*>> sameEdgeCounter;

   for(Edge* edge : accel->edges){
      if(collapseSameEdges){
         std::pair<ComplexFUInstance*,ComplexFUInstance*> key{edge->units[0].inst,edge->units[1].inst};

         if(sameEdgeCounter.count(key) == 1){
            continue;
         }

         sameEdgeCounter.insert(key);
      }

      fprintf(outputFile,"\t%s -> ",FormatNameToOutput(edge->units[0].inst));
      fprintf(outputFile,"%s",FormatNameToOutput(edge->units[1].inst));

      #if 1
      ComplexFUInstance* outputInst = edge->units[0].inst;
      int delay = 0;
      for(int i = 0; i < outputInst->tempData->numberOutputs; i++){
         if(edge->units[1].inst == outputInst->tempData->outputs[i].inst.inst && edge->units[1].port == outputInst->tempData->outputs[i].inst.port){
            delay = outputInst->tempData->outputs[i].delay;
            break;
         }
      }

      fprintf(outputFile,"[label=\"%d\"]",delay);
      //fprintf(outputFile,"[label=\"[%d:%d;%d:%d]\"]",outputInst->declaration->latencies[0],delay,edge->units[1].inst->declaration->inputDelays[edge->units[1].port],edge->delay);
      #endif

      fprintf(outputFile,";\n");
   }

   fprintf(outputFile,"}\n");
   #endif
}

FUDeclaration* MergeAccelerators(Versat* versat,FUDeclaration* accel1,FUDeclaration* accel2,SizedString name){
   Assert(accel1->type == FUDeclaration::COMPOSITE && accel2->type == FUDeclaration::COMPOSITE);

   Arena* arena = &versat->temp;
   Byte* mark = MarkArena(&versat->temp);

   ConsolidationGraph graph = GenerateConsolidationGraph(arena,accel1->baseCircuit,accel2->baseCircuit);

   OutputConsolidationGraph(graph,arena);

   ConsolidationGraph clique = MaxClique(graph);

   Mapping* graphToFinal = PushArray(arena,1024,Mapping);

   InstanceMap map = {};

   Accelerator* newGraph = CreateAccelerator(versat);
   newGraph->type = Accelerator::CIRCUIT;

   // Create base instances from accel 1
   for(FUInstance* inst : accel1->baseCircuit->instances){
      FUInstance* newNode = CreateFUInstance(newGraph,inst->declaration,MakeSizedString(inst->name.str));

      map.insert({inst,newNode});
   }

   #if 1
   // Create base instances from accel 2, unless they are mapped to nodes from accel 1
   for(FUInstance* inst : accel2->baseCircuit->instances){
      FUInstance* mappedNode = NodeMapped(inst,clique); // Returns node in graph 1

      if(mappedNode){
         mappedNode = map.find(mappedNode)->second;
         map.insert({inst,mappedNode});
         newGraph->nameToInstance.insert({inst->name.str,Test{mappedNode}});
      } else {
         mappedNode = CreateFUInstance(newGraph,inst->declaration,MakeSizedString(inst->name.str));
         map.insert({inst,mappedNode});
      }

      AddMapping(graphToFinal,inst,mappedNode);
   }
   #endif

   #if 1
   for(Edge* edge : accel1->baseCircuit->edges){
      FUInstance* mappedInst = map.find(edge->units[0].inst)->second;
      FUInstance* mappedOther = map.find(edge->units[1].inst)->second;

      ConnectUnits(mappedInst,edge->units[0].port,mappedOther,edge->units[1].port);
   }
   #endif

   #if 1
   // Add edges from accel 2
   for(Edge* edge : accel2->baseCircuit->edges){
      FUInstance* mappedInst = map.find(edge->units[0].inst)->second;
      FUInstance* mappedOther = map.find(edge->units[1].inst)->second;

      ConnectUnits(mappedInst,edge->units[0].port,mappedOther,edge->units[1].port);
   }
   #endif

   Assert(IsGraphValid(newGraph));

   OutputGraphDotFile(newGraph,false,"Merged.dot");

   //TODO: Since we are creating a new graph, we are instantiating memory and therefore RegisterSubUnit thinks we are setting default values for the units
   //      Need to rework this entire system.
   //Assert(false);
   FUDeclaration* decl = RegisterSubUnit(versat,name,newGraph);

   //decl->type = FUDeclaration::MERGED;
   decl->mergedType[0] = accel1;
   decl->mergedType[1] = accel2;

   return decl;
}

#include "merge.hpp"

#include "debug.hpp"
#include "textualRepresentation.hpp"

#include <cstdio>
#include <cstdarg>
#include <unordered_map>

bool NodeMappingConflict(PortEdge edge1,PortEdge edge2){
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

   if(p00.inst == p10.inst && p01.inst != p11.inst){
      return true;
   }
   if(p00.inst == p11.inst && p01.inst != p10.inst){
      return true;
   }
   if(p00.inst != p10.inst && p01.inst == p11.inst){
      return true;
   }
   if(p00.inst != p11.inst && p01.inst == p10.inst){
      return true;
   }

   return false;
}

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

int NodeIndex(ConsolidationGraph graph,MappingNode* node){
   int index = node - graph.nodes.data;
   return index;
}

IsCliqueResult IsClique(ConsolidationGraph graph){
   IsCliqueResult res = {};
   res.result = true;

   int cliqueSize = graph.validNodes.GetNumberBitsSet();

   // Zero or one are always cliques
   if(cliqueSize == 0 || cliqueSize == 1){
      return res;
   }

   for(int i = 0; i < graph.nodes.size; i++){
      if(!graph.validNodes.Get(i)){
         continue;
      }

      int count = 0;
      for(int ii = 0; ii < graph.edges.size; ii++){
         if(!graph.validNodes.Get(ii)){
            continue;
         }

         if(graph.edges[i].Get(ii)){
            count += 1;
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

// Checks if nodes are "equal" in terms of mapping
bool EqualPortMapping(Versat* versat,PortInstance p1,PortInstance p2){
   FUDeclaration* d1 = p1.inst->declaration;
   FUDeclaration* d2 = p2.inst->declaration;

   if(d1 == BasicDeclaration::input && d2 == BasicDeclaration::input){ // Check the special case for inputs
      if(GetInputPortNumber(versat,p1.inst) == GetInputPortNumber(versat,p2.inst)){
         return true;
      } else {
         return false;
      }
   }

   //bool res = ((d1 == d2) && (p1.port == p2.port));
   bool res = (d1 == d2);

   return res;
}

inline bool EdgeOrder(Edge* edge,ConsolidationGraphOptions options){
   if(options.type == ConsolidationGraphOptions::EXACT_ORDER){
      int order0 = edge->units[0].inst->graphData->order;
      int order1 = edge->units[1].inst->graphData->order;

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

int NodeDepth(MappingNode node){
   switch(node.type){
   case MappingNode::NODE:{
      ComplexFUInstance* inst0 = node.nodes.instances[0];
      ComplexFUInstance* inst1 = node.nodes.instances[1];

      return Abs(inst1->graphData->order - inst0->graphData->order);
   }break;
   case MappingNode::EDGE:{
      ComplexFUInstance* inst00 = node.edges[0].units[0].inst;
      ComplexFUInstance* inst01 = node.edges[0].units[1].inst;
      ComplexFUInstance* inst10 = node.edges[1].units[0].inst;
      ComplexFUInstance* inst11 = node.edges[1].units[1].inst;

      return Abs(Abs(inst01->graphData->order - inst00->graphData->order) - Abs(inst11->graphData->order - inst10->graphData->order));
   }break;
   default: Assert(false); break;
   }

   return 0;
}

ConsolidationResult GenerateConsolidationGraph(Versat* versat,Arena* arena,Accelerator* accel1,Accelerator* accel2,ConsolidationGraphOptions options){
   ConsolidationGraph graph = {};

   // Should be temp memory instead of using memory intended for the graph, but since the graph is expected to use a lot of memory and we are technically saving memory using this mapping, no major problem
   Hashmap<ComplexFUInstance*,MergeEdge> specificsMapping;
   specificsMapping.Init(arena,1000);
   Pool<MappingNode> specificsAdded = {};

   for(int i = 0; i < options.nSpecifics; i++){
      SpecificMergeNodes specific = options.specifics[i];

      MergeEdge node = {};
      node.instances[0] = specific.instA;
      node.instances[1] = specific.instB;

      specificsMapping.Insert(specific.instA,node);
      specificsMapping.Insert(specific.instB,node);

      MappingNode* n = specificsAdded.Alloc();
      n->nodes = node;
      n->type = MappingNode::NODE;
   }

   #if 0
   // Map outputs
   ComplexFUInstance* accel1Output = (ComplexFUInstance*) GetOutputInstance(accel1);
   ComplexFUInstance* accel2Output = (ComplexFUInstance*) GetOutputInstance(accel2);
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
   for(ComplexFUInstance* instA : accel1->instances){
      if(instA->declaration != BasicDeclaration::input){
         continue;
      }

      for(ComplexFUInstance* instB : accel2->instances){
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
   FOREACH_LIST(edge1,accel1->edges){
      if(!EdgeOrder(edge1,options)){
         continue;
      }

      FOREACH_LIST(edge2,accel2->edges){
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

   int upperBound = std::min(accel1->numberEdges,accel2->numberEdges);
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

void OutputConsolidationGraph(ConsolidationGraph graph,Arena* memory,bool onlyOutputValid,const char* format,...){
   va_list args;
   va_start(args,format);

   String fileName = vPushString(memory,format,args);
   PushNullByte(memory);

   va_end(args);

   FILE* outputFile = OpenFileAndCreateDirectories(fileName.data,"w");

   fprintf(outputFile,"graph GraphName {\n\tnode [fontcolor=white,style=filled,color=\"160,60,176\"];\n");
   for(int i = 0; i < graph.nodes.size; i++){
      MappingNode* node = &graph.nodes[i];

      if(onlyOutputValid && !graph.validNodes.Get(i)){
         continue;
      }

      ArenaMarker marker(memory);

      fprintf(outputFile,"\t\"%.*s\";\n",UNPACK_SS(Repr(*node,memory)));
   }

   #if 1
   for(int i = 0; i < graph.nodes.size; i++){
      if(!graph.validNodes.Get(i)){
         continue;
      }

      MappingNode* node1 = &graph.nodes[i];
      for(int ii = i + 1; ii < graph.edges.size; ii++){
         if(!graph.validNodes.Get(ii)){
            continue;
         }

         MappingNode* node2 = &graph.nodes[ii];

         ArenaMarker marker(memory);
         String str1 = Repr(*node1,memory);
         String str2 = Repr(*node2,memory);

         fprintf(outputFile,"\t\"%.*s\" -- \"%.*s\";\n",UNPACK_SS(str1),UNPACK_SS(str2));
      }
   }
   #endif

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

void AddCliqueToMapping(GraphMapping& res,ConsolidationGraph clique){
   for(int i : clique.validNodes){
      MappingNode node = clique.nodes[i];

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
}

GraphMapping ConsolidationGraphMapping(Versat* versat,Accelerator* accel1,Accelerator* accel2,ConsolidationGraphOptions options,Arena* arena,String name,GraphMapping& auxiliaryMapping){
   ArenaMarker marker(arena);

   ConsolidationResult result = GenerateConsolidationGraph(versat,arena,accel1,accel2,options);
   ConsolidationGraph graph = result.graph;

   #if 0
   printf("CQ:\n");
   for(int i = 0; i < result.graph.nodes.size; i++){
      String repr = Repr(result.graph.nodes[i],arena);
      printf("%.*s\n",UNPACK_SS(repr));
   }
   #endif

   GraphMapping res;

   for(MappingNode* n : result.specificsAdded){
      MappingNode node = *n;

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

   OutputConsolidationGraph(graph,arena,true,"debug/test.dot");

   if(graph.validNodes.bitSize == 0){
      return res;
   }

   OutputConsolidationGraph(graph,arena,true,"debug/%.*s/ConsolidationGraph.dot",UNPACK_SS(name));

   int upperBound = result.upperBound;

   #if 0
   ConsolidationGraph clique = ParallelMaxClique(graph,upperBound,arena);
   #else
   ConsolidationGraph clique = MaxClique(graph,upperBound,arena).clique;
   #endif
   #if 0
   printf("Clique: %d\n",ValidNodes(clique));
   for(int i = 0; i < clique.nodes.size; i++){
      if(clique.validNodes.Get(i)){
         String repr = Repr(result.graph.nodes[i],arena);
         printf("%.*s\n",UNPACK_SS(repr));
      }
   }
   #endif

   OutputConsolidationGraph(clique,arena,true,"debug/%.*s/Clique1.dot",UNPACK_SS(name));
   AddCliqueToMapping(res,clique);

   #if 0
   for(MappingNode* node : result.specificsAdded){
      String name = Repr(*node,arena);
      printf("%.*s\n",UNPACK_SS(name));
   }
   #endif

   return res;
}

static GraphMapping FirstFitGraphMapping(Versat* versat,Accelerator* accel1,Accelerator* accel2,Arena* arena){
   GraphMapping res;

   FOREACH_LIST(inst,accel1->instances){
      inst->tag = 0;
   }
   FOREACH_LIST(inst,accel2->instances){
      inst->tag = 0;
   }

   FOREACH_LIST(inst1,accel1->instances){
      if(inst1->tag){
         continue;
      }

      FOREACH_LIST(inst2,accel2->instances){
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

   FOREACH_LIST(inst,accel0->instances){
      inst->tag = 0;
   }

   FOREACH_LIST(inst,accel1->instances){
      inst->tag = 0;
   }

   int minIterations = std::min(accel0->numberInstances,accel1->numberInstances);

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
      FOREACH_LIST(inst,accel0->instances){
         if(!inst->tag){
            order1.push_back(inst);
         }
      }
      FOREACH_LIST(inst,accel1->instances){
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
   #if 0
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

      //BitArray* neighbors = CalculateNeighborsTable(graph,arena);
      BitArray* neighbors = graph.edges.data;

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
   #endif
   return (GraphMapping){};
}

struct OverheadCount{
   int muxes;
   int buffers;
};

OverheadCount CountOverheadUnits(Versat* versat,Accelerator* accel){
   OverheadCount res = {};

   FOREACH_LIST(inst,accel->instances){
      FUDeclaration* decl = inst->declaration;

      if(decl == BasicDeclaration::multiplexer || decl == BasicDeclaration::combMultiplexer){
         res.muxes += 1;
      }

      if(decl == BasicDeclaration::buffer || decl == BasicDeclaration::fixedBuffer){
         res.buffers += 1;
      }
   }

   return res;
}

void PrintAcceleratorStats(Versat* versat,Accelerator* accel){
   OverheadCount count = CountOverheadUnits(versat,accel);
   printf("Instances:%d Edges:%d Muxes:%d Buffer:%d",accel->numberInstances,accel->numberEdges,count.muxes,count.buffers);
}

static int Weight(Versat* versat,Accelerator* accel,bool countBuffer = false){
   int weight = 0;

   FOREACH_LIST(inst,accel->instances){
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

      if(decl == BasicDeclaration::multiplexer || decl == BasicDeclaration::combMultiplexer){
         weight += 1;
         continue;
      }

      if(decl == BasicDeclaration::buffer || decl == BasicDeclaration::fixedBuffer){
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

#if 0
void PrintSavedByMerge(Versat* versat,Accelerator* finalAccel,Accelerator* accel1,Accelerator* accel2){
   printf("Accel1: ");
   PrintAcceleratorStats(versat,accel1);
   printf("\nAccel2: ");
   PrintAcceleratorStats(versat,accel2);
   printf("\nFinal: ");
   PrintAcceleratorStats(versat,finalAccel);
   printf("\n");

   Accelerator* simpleCombine = MergeAccelerator(versat,accel1,accel2,nullptr,0,MergingStrategy::SIMPLE_COMBINATION,STRING("CheckSaved"));
   NOT_IMPLEMENTED; //
   //FixMultipleInputs(versat,simpleCombine);
   //CalculateDelay(versat,simpleCombine);

   printf("Simple Combine: %d %d\n",simpleCombine->numberInstances,simpleCombine->edges.Size());

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
#endif

void PrintMergePossibility(Versat* versat,Accelerator* accel1,Accelerator* accel2){
   FOREACH_LIST(inst1,accel1->instances){
      inst1->tag = 0;
   }
   FOREACH_LIST(inst2,accel2->instances){
      inst2->tag = 0;
   }

   int count = 0;
   int total = std::max(accel1->numberInstances,accel2->numberInstances);
   FOREACH_LIST(inst1,accel1->instances){
      Assert(!inst1->tag);

      FOREACH_LIST(inst2,accel2->instances){
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

static GraphMapping MergeAccelerator(Versat* versat,Accelerator* accel1,Accelerator* accel2,SpecificMergeNodes* specificNodes,int nSpecifics,MergingStrategy strategy,String name,GraphMapping& auxiliaryMapping){
   Arena* arena = &versat->temp;
   ArenaMarker marker(arena);

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

      graphMapping = ConsolidationGraphMapping(versat,accel1,accel2,options,arena,name,auxiliaryMapping);
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

   return graphMapping;
}

MergeGraphResult MergeGraph(Versat* versat,Accelerator* flatten1,Accelerator* flatten2,GraphMapping& graphMapping,String name){
   std::vector<Edge*> accel1EdgeMap;
   std::vector<Edge*> accel2EdgeMap;
   InstanceMap map = {};
   Accelerator* newGraph = CreateAccelerator(versat);

   // Create base instances from accel 1
   FOREACH_LIST(inst,flatten1->instances){
      ComplexFUInstance* newNode = (ComplexFUInstance*) CreateFUInstance(newGraph,inst->declaration,inst->name);

      map.insert({inst,newNode});
   }

   #if 1
   // Create base instances from accel 2, unless they are mapped to nodes from accel1
   FOREACH_LIST(inst,flatten2->instances){
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
            String newName = PushString(&versat->permanent,"%.*s/%.*s",UNPACK_SS(mappedNode->name),UNPACK_SS(inst->name));

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
   FOREACH_LIST(edge,flatten1->edges){
      ComplexFUInstance* mappedInst = map.find(edge->units[0].inst)->second;
      ComplexFUInstance* mappedOther = map.find(edge->units[1].inst)->second;

      Edge* newEdge = ConnectUnitsGetEdge(mappedInst,edge->units[0].port,mappedOther,edge->units[1].port,edge->delay);
      accel1EdgeMap.push_back(newEdge);
   }
   #endif

   #if 1
   // Create base edges from accel 2, unless they are mapped to edges from accel 1
   FOREACH_LIST(edge,flatten2->edges){
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

   Arena* arena = &versat->temp;
   ArenaMarker marker(arena);

   {
      ArenaMarker marker(arena);
      AcceleratorView view = CreateAcceleratorView(newGraph,arena);
      view.CalculateGraphData(arena);
      IsGraphValid(view);
      OutputGraphDotFile(versat,view,true,"debug/%.*s/Merged.dot",UNPACK_SS(name));
   }

   {
      ArenaMarker marker(arena);
      AcceleratorView view = CreateAcceleratorView(newGraph,accel1EdgeMap,arena);
      view.CalculateGraphData(arena);
      IsGraphValid(view);
      OutputGraphDotFile(versat,view,true,"debug/%.*s/Merged1View.dot",UNPACK_SS(name));
      view.CalculateDelay(arena);
   }

   {
      ArenaMarker marker(arena);
      AcceleratorView view = CreateAcceleratorView(newGraph,accel2EdgeMap,arena);
      view.CalculateGraphData(arena);
      IsGraphValid(view);
      OutputGraphDotFile(versat,view,true,"debug/%.*s/Merged2View.dot",UNPACK_SS(name));
   }

   MergeGraphResult res = {};
   res.accel1EdgeMap = accel1EdgeMap;
   res.accel2EdgeMap = accel2EdgeMap;
   res.newGraph = newGraph;

   return res;
}

FUDeclaration* MergeAccelerators(Versat* versat,FUDeclaration* accel1,FUDeclaration* accel2,String name,
                                 int flatteningOrder,MergingStrategy strategy,SpecificMerge* specifics,int nSpecifics){
   Arena* arena = &versat->temp;
   ArenaMarker marker(arena);

   #if 0
   #if 0
   Accelerator* flatten1 = Flatten(versat,accel1->baseCircuit,1);
   Accelerator* flatten2 = Flatten(versat,accel2->baseCircuit,1);
   #else
   Accelerator* flatten1 = Flatten(versat,accel1->baseCircuit,flatteningOrder);
   Accelerator* flatten2 = Flatten(versat,accel2->baseCircuit,flatteningOrder);
   #endif

   SpecificMergeNodes* specificNodes = PushArray<SpecificMergeNodes>(arena,nSpecifics).data;

   for(int i = 0; i < nSpecifics; i++){
      specificNodes[i].instA = (ComplexFUInstance*) GetInstanceByName(flatten1,specifics[i].instA);
      specificNodes[i].instB = (ComplexFUInstance*) GetInstanceByName(flatten2,specifics[i].instB);
   }

   {
      ArenaMarker marker(arena);
      AcceleratorView view = CreateAcceleratorView(flatten1,arena);
      view.CalculateGraphData(arena);
      OutputGraphDotFile(versat,view,true,"debug/%.*s/flatten1.dot",UNPACK_SS(name));
   }
   {
      ArenaMarker marker(arena);
      AcceleratorView view = CreateAcceleratorView(flatten2,arena);
      view.CalculateGraphData(arena);
      OutputGraphDotFile(versat,view,true,"debug/%.*s/flatten2.dot",UNPACK_SS(name));
   }

   GraphMapping auxiliaryMapping = {};
   GraphMapping graphMapping = MergeAccelerator(versat,flatten1,flatten2,specificNodes,nSpecifics,strategy,name,auxiliaryMapping);

   MergeGraphResult result = MergeGraph(versat,flatten1,flatten2,graphMapping,name);
   #endif

   MergeGraphResult result = HierarchicalMergeAccelerators(versat,accel1->baseCircuit,accel2->baseCircuit,name);

   FUDeclaration* decl = RegisterSubUnit(versat,name,result.newGraph);

   {
      ArenaMarker marker(arena);
      AcceleratorView view = CreateAcceleratorView(decl->fixedDelayCircuit,arena);
      view.CalculateGraphData(arena);
      OutputGraphDotFile(versat,view,true,"debug/%.*s/FixedDelay.dot",UNPACK_SS(name));
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

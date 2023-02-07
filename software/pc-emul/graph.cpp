#include "graph.hpp"
#include "memory.hpp"

#include "textualRepresentation.hpp"

static Arena nodeArenaInst;
static Arena* nodeArena = &nodeArenaInst;

bool ContainsNode(EdgeNode* edge,Node* node){
   bool res = (edge->in.inst == node || edge->out.inst == node);
   return res;
}

void RemoveNodeAndEdges(Graph* graph,Node* node){
   if(graph->instances == node){ // Special case
      graph->instances = node->next;
   } else {
      Node* previous = nullptr;
      FOREACH_LIST(ptr,graph->instances){
         if(ptr == node){
            previous->next = ptr->next;
            break;
         }
         previous = ptr;
      }
   }

   while(ContainsNode(graph->edges,node)){ // Special case
      EdgeNode* old = graph->edges;
      graph->edges = graph->edges->next;
   }

   EdgeNode* previous = nullptr;
   for(EdgeNode* ptr = graph->edges; ptr;){
      if(ContainsNode(ptr,node)){
         previous->next = ptr->next;

         ptr = previous->next;
      } else {
         previous = ptr;
         ptr = ptr->next;
      }
   }
}

void SortEdgesByVertices(Graph* graph){
   EdgeNode* start = nullptr;
   EdgeNode* ptr = nullptr;
   FOREACH_LIST(outPtr,graph->instances){
      FOREACH_LIST(inPtr,graph->instances){
         EdgeNode* previous = nullptr;
         for(EdgeNode* edge = graph->edges; edge;){
            EdgeNode* next = edge->next;

            if(edge->out.inst == outPtr && edge->in.inst == inPtr){
               // Remove edge from edge list
               if(previous){
                  previous->next = next;
               } else if(graph->edges){
                  graph->edges = next;
               }

               // Add it to new list
               if(ptr){
                  ptr->next = edge;
               } else {
                  start = edge;
               }

               edge->next = nullptr;
               ptr = edge;
            } else {
               previous = edge;
            }

            edge = next;
         }
      }
   }

   graph->edges = start;
}

void ReorganizeAccelerator(Graph* graph,Arena* temp);

FlattenResult FlattenNode(Graph* graph,Node* node,Arena* arena){
   FUDeclaration* decl = node->declaration;

   if(decl->type != FUDeclaration::COMPOSITE){
      return (FlattenResult){};
   }

   Accelerator* circuit = decl->baseCircuit;
   ReorganizeAccelerator(circuit,arena);

   ComplexFUInstance* inputPortToNode[99] = {};
   ComplexFUInstance* outputInstance = nullptr;

   Hashmap<ComplexFUInstance*,Node*> map = {};
   map.Init(arena,circuit->numberInstances);

   int inserted = 0;

   Node* ptr = node;
   FOREACH_LIST(inst,circuit->instances){
      if(inst->declaration == BasicDeclaration::input){
         int port = inst->portIndex;
         continue;
      }
      if(inst->declaration == BasicDeclaration::output){
         outputInstance = inst;
         continue;
      }

      String newName = PushString(&graph->versat->permanent,"%.*s.%.*s",UNPACK_SS(node->name),UNPACK_SS(inst->name));
      ComplexFUInstance* res = CopyInstance(graph,inst,newName,true,ptr);

      map.Insert(inst,res);

      inserted += 1;
      ptr = res;
   }
   Node* flatUnitStart = node->next;
   Node* flatUnitEnd = ptr->next;

   EdgeNode* listPos = nullptr; // Tries to keep edges of similar sorted nodes close together
   FOREACH_LIST(ptr,graph->edges){
      if(ptr->in.inst == node){
         listPos = ptr;
         break;
      }
   }

   // Input edges
   EdgeNode* flatEdgeStart = nullptr;
   int flattenedEdges = 0;
   FOREACH_LIST(edge,circuit->edges){
      if(edge->units[0].inst->declaration != BasicDeclaration::input ||
         edge->units[1].inst->declaration == BasicDeclaration::output){ // We don't deal with input to output edges here
         continue;
      }

      Assert(edge->units[0].port == 0); // Must be connected to port 0

      int outsidePort = edge->units[0].inst->portIndex; // The input port is stored in the id of the input unit
      ComplexFUInstance* instInside = edge->units[1].inst;
      Node* nodeInside = map.GetOrFail(instInside);
      int portInside = edge->units[1].port;

      for(EdgeNode* edgeNode = graph->edges; edgeNode; edgeNode = edgeNode->next){
         if(!(edgeNode->in.inst == node && edgeNode->in.port == outsidePort)){
            continue;
         }

         #if 0
         if(listPos == nullptr){
            listPos = edgeNode;
            flatEdgeStart = edgeNode;
         }
         #endif

         listPos = ConnectUnits(edgeNode->out.inst,edgeNode->out.port,nodeInside,portInside,0,listPos); // AddEdge(graph,edgeNode->out.inst,edgeNode->out.port,nodeInside,portInside,listPos);
         flattenedEdges += 1;
      }
   }

   #if 1
   // Input to output directly
   FOREACH_LIST(edge,circuit->edges){
      if(!(edge->units[0].inst->declaration == BasicDeclaration::input &&
           edge->units[1].inst->declaration == BasicDeclaration::output)){
         continue;
      }

      int outsideInputPort = edge->units[0].inst->portIndex; // b
      int outsideOutputPort = edge->units[1].port; // c

      // Search out node
      for(EdgeNode* n1 = graph->edges; n1; n1 = n1->next){
         if(!(n1->in.inst == node && n1->in.port == outsideInputPort)){
            continue;
         }

         PortNode out = n1->out;

         // Search in node
         for(EdgeNode* n2 = graph->edges; n2; n2 = n2->next){
            if(!(n2->out.inst == node && n2->out.port == outsideOutputPort)){
               continue;
            }

            PortNode in = n2->in;

            listPos = ConnectUnits(out.inst,out.port,in.inst,in.port,0,listPos);  // AddEdge(graph,out.node,out.port,in.node,in.port,listPos);
            flattenedEdges += 1;
         }
      }
   }
   #endif

   FOREACH_LIST(ptr,graph->edges){
      if(ptr->out.inst == node){
         listPos = ptr;
         break;
      }
   }

   #if 1
   int circuitEdgesStart = flattenedEdges;
   // Circuit edges
   FOREACH_LIST(edge,circuit->edges){
      if(edge->units[0].inst->declaration == BasicDeclaration::input ||
         edge->units[1].inst->declaration == BasicDeclaration::output){
         continue;
      }

      ComplexFUInstance* outInside = edge->units[0].inst;
      ComplexFUInstance* inInside = edge->units[1].inst;
      int outPort = edge->units[0].port;
      int inPort = edge->units[1].port;

      Node* out = map.GetOrFail(outInside);
      Node* in = map.GetOrFail(inInside);

      listPos = ConnectUnits(out,outPort,in,inPort,0,listPos); // AddEdge(graph,out,outPort,in,inPort,listPos);
      flattenedEdges += 1;
   }
   int circuitEdgesEnd = flattenedEdges;
   #endif

   // Output edges
   FOREACH_LIST(edge,circuit->edges){
      if(edge->units[1].inst->declaration != BasicDeclaration::output ||
         edge->units[0].inst->declaration == BasicDeclaration::input){ // We don't deal with input to output edges here
         continue;
      }

      ComplexFUInstance* instInside = edge->units[0].inst;
      Node* nodeInside = map.GetOrFail(instInside);
      int portInside = edge->units[0].port;
      int outsidePort = edge->units[1].port;

      for(EdgeNode* edgeNode = graph->edges; edgeNode; edgeNode = edgeNode->next){
         if(!(edgeNode->out.inst == node && edgeNode->out.port == outsidePort)){
            continue;
         }

         listPos = ConnectUnits(nodeInside,portInside,edgeNode->in.inst,edgeNode->in.port,0,listPos); // AddEdge(graph,nodeInside,portInside,edgeNode->in.node,edgeNode->in.port,listPos);
         flattenedEdges += 1;
      }
   }

   RemoveNodeAndEdges(graph,node);

   SortEdgesByVertices(graph);

   FlattenResult res = {};
   res.flatUnitStart = flatUnitStart;
   res.flatUnitEnd = flatUnitEnd;
   res.flatEdgeStart = flatEdgeStart;
   res.flatEdgeEnd = listPos;
   res.flattenedUnits = inserted;
   res.flattenedEdges = flattenedEdges;
   res.circuitEdgesStart = circuitEdgesStart;
   res.circuitEdgesEnd = circuitEdgesEnd;

   return res;
}

String OutputDotGraph(Subgraph graph,Arena* output){
   Byte* mark = MarkArena(output);

   PushString(output,"digraph accel {\n\tnode [fontcolor=white,style=filled,color=\"160,60,176\"];\n");
   FOREACH_SUBLIST(ptr,graph.nodes){
      ArenaMarker marker(nodeArena); // Use node arena as temporary storage for repr

      String res = ptr->name;
      //String res = Repr(ptr->inst,GRAPH_DOT_FORMAT_NAME,nodeArena);
      PushString(output,"    \"%.*s\";\n",UNPACK_SS(res));
   }
   FOREACH_SUBLIST(ptr,graph.edges){
      ArenaMarker marker(nodeArena); // Use node arena as temporary storage for repr

      String out = ptr->out.inst->name;
      String in = ptr->in.inst->name;

      #if 0
      String out = Repr(ptr->out.node->inst,GRAPH_DOT_FORMAT_NAME,nodeArena);
      String in = Repr(ptr->in.node->inst,GRAPH_DOT_FORMAT_NAME,nodeArena);
      #endif

      PushString(output,"    \"%.*s\" -> \"%.*s\";\n",UNPACK_SS(out),UNPACK_SS(in));
   }
   PushString(output,"}\n");

   String res = PointArena(output,mark);
   return res;
}

String OutputDotGraph(Graph* graph,Arena* output){
   Byte* mark = MarkArena(output);

   PushString(output,"digraph accel {\n\tnode [fontcolor=white,style=filled,color=\"160,60,176\"];\n");
   FOREACH_LIST(ptr,graph->instances){
      ArenaMarker marker(nodeArena); // Use node arena as temporary storage for repr

      String res = ptr->name;
      //String res = Repr(ptr->inst,GRAPH_DOT_FORMAT_NAME,nodeArena);
      PushString(output,"    \"%.*s\";\n",UNPACK_SS(res));
   }
   FOREACH_LIST(ptr,graph->edges){
      ArenaMarker marker(nodeArena); // Use node arena as temporary storage for repr

      String out = ptr->out.inst->name;
      String in = ptr->in.inst->name;

      #if 0
      String out = Repr(ptr->out.node->inst,GRAPH_DOT_FORMAT_NAME,nodeArena);
      String in = Repr(ptr->in.node->inst,GRAPH_DOT_FORMAT_NAME,nodeArena);
      #endif

      PushString(output,"    \"%.*s\" -> \"%.*s\";\n",UNPACK_SS(out),UNPACK_SS(in));
   }
   PushString(output,"}\n");

   String res = PointArena(output,mark);
   return res;
}

#include "merge.hpp"

Node* GetOutputInstance(Subgraph sub){
   FOREACH_SUBLIST(inst,sub.nodes){
      if(inst->declaration == BasicDeclaration::output){
         return inst;
      }
   }
   return nullptr;
}

ConsolidationResult GenerateConsolidationGraph(Versat* versat,Arena* arena,Subgraph accel1,Subgraph accel2,ConsolidationGraphOptions options){
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
      FOREACH_SUBLIST(edge2,accel2.edges){
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


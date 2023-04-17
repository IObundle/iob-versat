#include "graph.hpp"
#include "memory.hpp"

#include "textualRepresentation.hpp"
#include "debug.hpp"
#include "debugGUI.hpp"

#if 0
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
            graph->numberInstances -= 1;
            break;
         }
         previous = ptr;
      }
   }

   while(ContainsNode(graph->edges,node)){ // Special case
      graph->edges = graph->edges->next;
   }

   EdgeNode* previous = nullptr;
   for(EdgeNode* ptr = graph->edges; ptr;){
      if(ContainsNode(ptr,node)){
         previous->next = ptr->next;
         ptr = previous->next;
         graph->numberEdges -= 1;
      } else {
         previous = ptr;
         ptr = ptr->next;
      }
   }
}

Array<int> OffsetGivenCounts(Array<int> counts,Arena* out){
   int size = counts.size;
   Array<int> offsets = PushArray<int>(out,size);

   offsets[0] = 0;
   for(int i = 1; i < size; i++){
      offsets[i] = offsets[i-1] + counts[i-1];
   }
   return offsets;
}

void SortEdgesByVertices(Graph* graph){
   struct EdgeWithOrder{
      EdgeNode* node;
      int outputIndex;
      int inputIndex;
   };

   Arena tempInst = InitArena(Megabyte(64));
   Arena* temp = &tempInst;

   #if 1
   // Fast
   region(temp){
      int instances = Size(graph->instances);
      int edges = Size(graph->edges);

      Hashmap<ComplexFUInstance*,int> instToIndex = {};
      instToIndex.Init(temp,instances);

      int index = 0;
      FOREACH_LIST_INDEXED(ptr,graph->instances,index){
         Assert(ptr);
         instToIndex.Insert(ptr,index);
      }
      int nodeCount = index;

      Array<EdgeWithOrder> orderedEdges = PushArray<EdgeWithOrder>(temp,edges);
      Array<int> outCounts = PushArray<int>(temp,nodeCount);
      Array<int> inCounts = PushArray<int>(temp,nodeCount);

      Memset(outCounts,0);
      Memset(inCounts,0);

      index = 0;
      FOREACH_LIST_INDEXED(edge,graph->edges,index){
         int outIndex = instToIndex.GetOrFail(edge->out.inst);
         int inIndex = instToIndex.GetOrFail(edge->in.inst);

         orderedEdges[index].node = edge;
         orderedEdges[index].outputIndex = outIndex;
         orderedEdges[index].inputIndex = inIndex;

         outCounts[outIndex] += 1;
         inCounts[inIndex] += 1;
      }
      int edgeCount = index;
      Assert(edges == edgeCount);

      Array<int> inOffsets = OffsetGivenCounts(inCounts,temp);
      Array<EdgeWithOrder> orderedByIn = PushArray<EdgeWithOrder>(temp,edgeCount);

      for(EdgeWithOrder& edge : orderedEdges){
         Assert(edge.node);
         int inIndex = edge.inputIndex;
         int offset = inOffsets[inIndex]++;
         //DEBUG_BREAK_IF(offset == 0);
         orderedByIn[offset] = edge;
      }

      Array<int> outOffsets = OffsetGivenCounts(outCounts,temp);

      for(EdgeWithOrder& edge : orderedByIn){
         Assert(edge.node);
         int outIndex = edge.outputIndex;
         int offset = outOffsets[outIndex]++;
         //DEBUG_BREAK_IF(offset == 0);
         orderedEdges[offset] = edge;
      }

      for(int i = 0; i < orderedEdges.size - 1; i++){
         orderedEdges[i].node->next = orderedEdges[i+1].node;
      }
      orderedEdges[orderedEdges.size - 1].node->next = nullptr;
      graph->edges = orderedEdges[0].node;
   };
   #else
   // Slow
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
      //AssertNoLoop(graph->edges,&temp);
   }
   graph->edges = start;
   #endif

   Free(temp);
}

void ReorganizeAccelerator(Graph* graph,Arena* temp);

FlattenResult FlattenNode(Graph* graph,Node* node,Arena* arena){
   FUDeclaration* decl = node->declaration;

   if(decl->type != FUDeclaration::COMPOSITE){
      return (FlattenResult){};
   }

   Accelerator* circuit = decl->baseCircuit;
   ReorganizeAccelerator(circuit,arena);

   Hashmap<ComplexFUInstance*,Node*> map = {};
   map.Init(arena,circuit->numberInstances);

   int inserted = 0;

   Node* ptr = node;
   FOREACH_LIST(inst,circuit->instances){
      if(inst->declaration == BasicDeclaration::input){
         continue;
      }
      if(inst->declaration == BasicDeclaration::output){
         continue;
      }

      String newName = PushString(&graph->versat->permanent,"%.*s.%.*s",UNPACK_SS(node->name),UNPACK_SS(inst->name));
      ComplexFUInstance* res = CopyInstance(graph,inst,newName,ptr);

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

   //SortEdgesByVertices(graph);

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

#if 0
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
         if(!(EqualPortMapping(edge1->units[0],edge2->units[0]) &&
            EqualPortMapping(edge1->units[1],edge2->units[1]))){
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

#endif
#endif

InstanceNode* GetInstanceNode(Accelerator* accel,ComplexFUInstance* inst){
   FOREACH_LIST(ptr,accel->allocated){
      if(ptr->inst == inst){
         return ptr;
      }
   }
   Assert(false); // For now, this should not fail
   return nullptr;
}

void CalculateNodeType(InstanceNode* node){
   node->type = InstanceNode::TAG_UNCONNECTED;

   bool hasInput = (node->allInputs != nullptr);
   bool hasOutput = (node->allOutputs != nullptr);

   // If the unit is both capable of acting as a sink or as a source of data
   if(hasInput && hasOutput){
      if(CHECK_DELAY(node->inst,DELAY_TYPE_SINK_DELAY) || CHECK_DELAY(node->inst,DELAY_TYPE_SOURCE_DELAY)){
         node->type = InstanceNode::TAG_SOURCE_AND_SINK;
      }  else {
         node->type = InstanceNode::TAG_COMPUTE;
      }
   } else if(hasInput){
      node->type = InstanceNode::TAG_SINK;
   } else if(hasOutput){
      node->type = InstanceNode::TAG_SOURCE;
   } else {
      // Unconnected
   }
}

void FixInputs(InstanceNode* node){
   Memset(node->inputs,{});
   node->multipleSamePortInputs = false;

   FOREACH_LIST(ptr,node->allInputs){
      int port = ptr->port;

      if(node->inputs[port].node){
         node->multipleSamePortInputs = true;
      }

      node->inputs[port] = ptr->instConnectedTo;
   }
}

Edge* ConnectUnitsGetEdge(PortNode out,PortNode in,int delay){
   FUDeclaration* inDecl = in.node->inst->declaration;
   FUDeclaration* outDecl = out.node->inst->declaration;

   Assert(out.node->inst->accel == in.node->inst->accel);
   Assert(in.port < inDecl->inputDelays.size);
   Assert(out.port < outDecl->outputLatencies.size);

   Accelerator* accel = out.node->inst->accel;

   Edge* edge = PushStruct<Edge>(accel->accelMemory);
   edge->next = accel->edges;
   accel->edges = edge;

   edge->units[0].inst = out.node->inst;
   edge->units[0].port = out.port;
   edge->units[1].inst = in.node->inst;
   edge->units[1].port = in.port;
   edge->delay = delay;

   // Update graph data.
   InstanceNode* inputNode = in.node;
   InstanceNode* outputNode = out.node;

   // Add info to outputNode
   // Update all outputs
   {
   ConnectionNode* con = PushStruct<ConnectionNode>(accel->accelMemory);
   con->edgeDelay = delay;
   con->port = out.port;
   con->instConnectedTo.node = inputNode;
   con->instConnectedTo.port = in.port;

   outputNode->allOutputs = ListInsert(outputNode->allOutputs,con);
   outputNode->outputs += 1;
   }

   // Add info to inputNode
   {
   ConnectionNode* con = PushStruct<ConnectionNode>(accel->accelMemory);
   con->edgeDelay = delay;
   con->port = in.port;
   con->instConnectedTo.node = outputNode;
   con->instConnectedTo.port = out.port;

   inputNode->allInputs = ListInsert(inputNode->allInputs,con);

   if(inputNode->inputs[in.port].node){
      inputNode->multipleSamePortInputs = true;
   }

   inputNode->inputs[in.port].node = outputNode;
   inputNode->inputs[in.port].port = out.port;
   }

   CalculateNodeType(inputNode);
   CalculateNodeType(outputNode);

   return edge;
}

// Connects out -> in
Edge* ConnectUnitsGetEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay,Edge* previous){
   FUDeclaration* inDecl = in->declaration;
   FUDeclaration* outDecl = out->declaration;

   Assert(out->accel == in->accel);
   Assert(inIndex < inDecl->inputDelays.size);
   Assert(outIndex < outDecl->outputLatencies.size);

   Accelerator* accel = out->accel;

   Edge* edge = PushStruct<Edge>(accel->accelMemory);

   if(previous){
      edge->next = previous->next;
      previous->next = edge;
   } else {
      edge->next = accel->edges;
      accel->edges = edge;
   }

   edge->units[0].inst = (ComplexFUInstance*) out;
   edge->units[0].port = outIndex;
   edge->units[1].inst = (ComplexFUInstance*) in;
   edge->units[1].port = inIndex;
   edge->delay = delay;

   // Update graph data.
   InstanceNode* inputNode = GetInstanceNode(accel,(ComplexFUInstance*) in);
   InstanceNode* outputNode = GetInstanceNode(accel,(ComplexFUInstance*) out);

   // Add info to outputNode
   // Update all outputs
   {
   ConnectionNode* con = PushStruct<ConnectionNode>(accel->accelMemory);
   con->edgeDelay = delay;
   con->port = outIndex;
   con->instConnectedTo.node = inputNode;
   con->instConnectedTo.port = inIndex;

   outputNode->allOutputs = ListInsert(outputNode->allOutputs,con);
   outputNode->outputs += 1;
   }

   // Add info to inputNode
   {
   ConnectionNode* con = PushStruct<ConnectionNode>(accel->accelMemory);
   con->edgeDelay = delay;
   con->port = inIndex;
   con->instConnectedTo.node = outputNode;
   con->instConnectedTo.port = outIndex;

   inputNode->allInputs = ListInsert(inputNode->allInputs,con);

   if(inputNode->inputs[inIndex].node){
      inputNode->multipleSamePortInputs = true;
   }

   inputNode->inputs[inIndex].node = outputNode;
   inputNode->inputs[inIndex].port = outIndex;
   }

   CalculateNodeType(inputNode);
   CalculateNodeType(outputNode);

   return edge;
}

void RemoveConnection(Accelerator* accel,PortNode out,PortNode in){
   RemoveConnection(accel,out.node,out.port,in.node,in.port);
}

void RemoveConnection(Accelerator* accel,InstanceNode* out,int outPort,InstanceNode* in,int inPort){
   accel->edges = ListRemoveAll(accel->edges,[&](Edge* edge){
      bool res = (edge->out.inst == out->inst && edge->out.port == outPort && edge->in.inst == in->inst && edge->in.port == inPort);
      return res;
   });

   out->allOutputs = ListRemoveAll(out->allOutputs,[&](ConnectionNode* n){
      bool res = (n->port == outPort && n->instConnectedTo.node == in && n->instConnectedTo.port == inPort);
      return res;
   });
   out->outputs = Size(out->allOutputs);

   in->allInputs = ListRemoveAll(in->allInputs,[&](ConnectionNode* n){
      bool res = (n->port == inPort && n->instConnectedTo.node == out && n->instConnectedTo.port == outPort);
      return res;
   });
   FixInputs(in);
}

void RemoveAllDirectedConnections(InstanceNode* out,InstanceNode* in){
   out->allOutputs = ListRemoveAll(out->allOutputs,[&](ConnectionNode* n){
      return (n->instConnectedTo.node == in);
   });
   out->outputs = Size(out->allOutputs);

   in->allInputs = ListRemoveAll(in->allInputs,[&](ConnectionNode* n){
      return (n->instConnectedTo.node == out);
   });

   FixInputs(in);
}

void RemoveAllConnections(InstanceNode* n1,InstanceNode* n2){
   RemoveAllDirectedConnections(n1,n2);
   RemoveAllDirectedConnections(n2,n1);
}

InstanceNode* RemoveUnit(InstanceNode* nodes,InstanceNode* unit){
   STACK_ARENA(temp,Kilobyte(32));

   Hashmap<InstanceNode*,int>* toRemove = PushHashmap<InstanceNode*,int>(&temp,100);

   for(auto* ptr = unit->allInputs; ptr;){
      auto* next = ptr->next;

      toRemove->Insert(ptr->instConnectedTo.node,0);

      ptr = next;
   }

   for(auto* ptr = unit->allOutputs; ptr;){
      auto* next = ptr->next;

      toRemove->Insert(ptr->instConnectedTo.node,0);

      ptr = next;
   }

   for(auto& pair : toRemove){
      RemoveAllConnections(unit,pair.first);
   }

   // Remove instance
   int oldSize = Size(nodes);
   auto* res = ListRemove(nodes,unit);
   int newSize = Size(res);
   Assert(oldSize == newSize + 1);

   return res;
}

void InsertUnit(Accelerator* accel,PortNode before,PortNode after,PortNode newUnit){
   RemoveConnection(accel,before.node,before.port,after.node,after.port);
   ConnectUnitsGetEdge(newUnit,after,0);
   ConnectUnitsGetEdge(before,newUnit,0);
}

#if 0
// Fixes edges such that unit before connected to after, are reconnected to new unit
void InsertUnit(Accelerator* accel, PortInstance before, PortInstance after, PortInstance newUnit){
   //UNHANDLED_ERROR; // Do not think any code uses this function. Check if needed and remove otherwise. Seems to be old.
   FOREACH_LIST(edge,accel->edges){
      if(edge->units[0] == before && edge->units[1] == after){
         RemoveConnection(GetInstanceNode(accel,edge->units[0].inst),edge->units[0].port,GetInstanceNode(accel,edge->units[1].inst),edge->units[1].port);

         Edge* newEdge = ConnectUnits(newUnit,after);
         ConnectUnits(edge->units[0],newUnit);

         return;
      }
   }

   Assert(false);
}
#endif

void AssertGraphValid(InstanceNode* nodes,Arena* arena){
   BLOCK_REGION(arena);

   int size = Size(nodes);
   Hashmap<InstanceNode*,int>* seen = PushHashmap<InstanceNode*,int>(arena,size);

   FOREACH_LIST(ptr,nodes){
      seen->Insert(ptr,0);
   }

   FOREACH_LIST(ptr,nodes){
      FOREACH_LIST(con,ptr->allInputs){
         seen->GetOrFail(con->instConnectedTo.node);
      }

      FOREACH_LIST(con,ptr->allOutputs){
         seen->GetOrFail(con->instConnectedTo.node);
      }

      for(PortNode& n : ptr->inputs){
         if(n.node){
            seen->GetOrFail(n.node);
         }
      }
   }
}

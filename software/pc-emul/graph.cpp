#include "graph.hpp"
#include "memory.hpp"

#include "textualRepresentation.hpp"

static Arena nodeArenaInst;
static Arena* nodeArena = &nodeArenaInst;

static EdgeNode* freeEdgeNodes = nullptr;
static Node* freeNodes = nullptr;

static Node* AllocateNode(){
   Node* res = nullptr;
   if(freeNodes){
      res = freeNodes;
      freeNodes = res->next;
   } else {
      res = PushStruct<Node>(nodeArena);
   }
   return res;
}

static EdgeNode* AllocateEdgeNode(){
   EdgeNode* res = nullptr;
   if(freeEdgeNodes){
      res = freeEdgeNodes;
      freeEdgeNodes = res->next;
   } else {
      res = PushStruct<EdgeNode>(nodeArena);
   }
   return res;
}

Graph* PushGraph(Arena* arena){
   static bool init = false;
   if(!init){
      InitArena(nodeArena,Gigabyte(1));
      init = true;
   }

   Graph* graph = PushStruct<Graph>(arena);
   *graph = {};

   return graph;
}

Node* AddNode(Graph* graph,ComplexFUInstance* inst,Node* previous){
   Node* node = AllocateNode();
   node->inst = inst;
   node->name = inst->name;

   if(previous){
      node->next = previous->next;
      previous->next = node;
   } else {
      node->next = graph->nodes;
      graph->nodes = node;
   }

   return node;
}

EdgeNode* AddEdge(Graph* graph,Node* out,int outPort,Node* in,int inPort,EdgeNode* previous){
   EdgeNode* node = AllocateEdgeNode();
   node->out.node = out;
   node->out.port = outPort;
   node->in.node = in;
   node->in.port = inPort;

   if(previous){
      node->next = previous->next;
      previous->next = node;
   } else {
      node->next = graph->edges;
      graph->edges = node;
   }

   return node;
}

bool ContainsNode(EdgeNode* edge,Node* node){
   bool res = (edge->in.node == node || edge->out.node == node);
   return res;
}

void RemoveNodeAndEdges(Graph* graph,Node* node){
   if(graph->nodes == node){ // Special case
      graph->nodes = node->next;
   } else {
      Node* previous = nullptr;
      for(Node* ptr = graph->nodes; ptr; ptr = ptr->next){
         if(ptr == node){
            previous->next = ptr->next;
            break;
         }
         previous = ptr;
      }
   }
   // Add to free list
   node->next = freeNodes;
   freeNodes = node;

   while(ContainsNode(graph->edges,node)){ // Special case
      EdgeNode* old = graph->edges;
      graph->edges = graph->edges->next;

      old->next = freeEdgeNodes;
      freeEdgeNodes = old;
   }

   EdgeNode* previous = nullptr;
   for(EdgeNode* ptr = graph->edges; ptr;){
      if(ContainsNode(ptr,node)){
         previous->next = ptr->next;

         ptr->next = freeEdgeNodes;
         freeEdgeNodes = ptr;

         ptr = previous->next;
      } else {
         previous = ptr;
         ptr = ptr->next;
      }
   }
}

void ConvertGraph(Graph* graph,Accelerator* accel,Arena* arena){
   Hashmap<ComplexFUInstance*,Node*> map = {};
   map.Init(arena,accel->instances.Size());

   Assert(graph->nodes == nullptr); // Only allow conversion in an empty graph

   {
   Node* ptr = nullptr;
   for(ComplexFUInstance* inst : accel->instances){
      ptr = AddNode(graph,inst,ptr);
      map.Insert(inst,ptr);
   }
   }

   {
   EdgeNode* ptr = nullptr;
   for(ComplexFUInstance* inst : accel->instances){
      Node* out = map.GetOrFail(inst);

      for(Edge* edge : accel->edges){
         if(edge->units[0].inst != inst){
            continue;
         }

         int outPort = edge->units[0].port;
         Node* in = map.GetOrFail(edge->units[1].inst);
         int inPort = edge->units[1].port;

         ptr = AddEdge(graph,out,outPort,in,inPort,ptr);
      }
   }
   }
}

void SortEdgesByVertices(Graph* graph){
   EdgeNode* start = nullptr;
   EdgeNode* ptr = nullptr;
   FOREACH_LIST(nodePtr,graph->nodes){
      EdgeNode* previous = nullptr;
      for(EdgeNode* edge = graph->edges; edge;){
         EdgeNode* next = edge->next;

         if(edge->out.node == nodePtr){
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

   graph->edges = start;
}

FlattenResult FlattenNode(Graph* graph,Node* node,Arena* arena){
   FUDeclaration* decl = node->inst->declaration;

   if(decl->type != FUDeclaration::COMPOSITE){
      return (FlattenResult){};
   }

   Accelerator* circuit = decl->baseCircuit;

   ComplexFUInstance* inputPortToNode[99] = {};
   ComplexFUInstance* outputInstance = nullptr;

   Hashmap<ComplexFUInstance*,Node*> map = {};
   map.Init(arena,circuit->instances.Size());

   int inserted = 0;

   Node* ptr = node;
   bool first = true;
   for(ComplexFUInstance* inst : circuit->instances){
      Node* instNode = nullptr;

      if(inst->declaration == BasicDeclaration::input){
         int port = inst->id;
         continue;
      }
      if(inst->declaration == BasicDeclaration::output){
         outputInstance = inst;
         continue;
      }

      instNode = AllocateNode();

      String newName = PushString(arena,"%.*s.%.*s",UNPACK_SS(node->name),UNPACK_SS(inst->name));

      instNode->inst = inst;
      instNode->next = ptr->next;
      instNode->name = newName;

      map.Insert(inst,instNode);

      ptr->next = instNode;
      ptr = instNode;

      inserted += 1;
   }
   Node* flatUnitStart = node->next;
   Node* flatUnitEnd = ptr->next;

   // Input edges
   EdgeNode* listPos = nullptr; // Tries to keep edges of similar sorted nodes close together
   FOREACH_LIST(ptr,graph->edges){
      if(ptr->in.node == node){
         listPos = ptr;
         break;
      }
   }

   EdgeNode* flatEdgeStart = nullptr;
   int flattenedEdges = 0;
   for(Edge* edge : circuit->edges){
      if(edge->units[0].inst->declaration != BasicDeclaration::input ||
         edge->units[1].inst->declaration == BasicDeclaration::output){ // We don't deal with input to output edges here
         continue;
      }

      Assert(edge->units[0].port == 0); // Must be connected to port 0

      int outsidePort = edge->units[0].inst->id; // The input port is stored in the id of the input unit
      ComplexFUInstance* instInside = edge->units[1].inst;
      Node* nodeInside = map.GetOrFail(instInside);
      int portInside = edge->units[1].port;

      for(EdgeNode* edgeNode = graph->edges; edgeNode; edgeNode = edgeNode->next){
         if(!(edgeNode->in.node == node && edgeNode->in.port == outsidePort)){
            continue;
         }

         #if 0
         if(listPos == nullptr){
            listPos = edgeNode;
            flatEdgeStart = edgeNode;
         }
         #endif

         listPos = AddEdge(graph,edgeNode->out.node,edgeNode->out.port,nodeInside,portInside,listPos);
         flattenedEdges += 1;
      }
   }

   #if 1
   // Input to output directly
   for(Edge* edge : circuit->edges){
      if(!(edge->units[0].inst->declaration == BasicDeclaration::input &&
           edge->units[1].inst->declaration == BasicDeclaration::output)){
         continue;
      }

      int outsideInputPort = edge->units[0].inst->id; // b
      int outsideOutputPort = edge->units[1].port; // c

      // Search out node
      for(EdgeNode* n1 = graph->edges; n1; n1 = n1->next){
         if(!(n1->in.node == node && n1->in.port == outsideInputPort)){
            continue;
         }

         PortNode out = n1->out;

         // Search in node
         for(EdgeNode* n2 = graph->edges; n2; n2 = n2->next){
            if(!(n2->out.node == node && n2->out.port == outsideOutputPort)){
               continue;
            }

            PortNode in = n2->in;

            listPos = AddEdge(graph,out.node,out.port,in.node,in.port,listPos);
            flattenedEdges += 1;
         }
      }
   }
   #endif

   FOREACH_LIST(ptr,graph->edges){
      if(ptr->out.node == node){
         listPos = ptr;
         break;
      }
   }

   #if 1
   int circuitEdgesStart = flattenedEdges;
   // Circuit edges
   for(Edge* edge : circuit->edges){
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

      listPos = AddEdge(graph,out,outPort,in,inPort,listPos);
      flattenedEdges += 1;
   }
   int circuitEdgesEnd = flattenedEdges;
   #endif

   // Output edges
   for(Edge* edge : circuit->edges){
      if(edge->units[1].inst->declaration != BasicDeclaration::output ||
         edge->units[0].inst->declaration == BasicDeclaration::input){ // We don't deal with input to output edges here
         continue;
      }

      ComplexFUInstance* instInside = edge->units[0].inst;
      Node* nodeInside = map.GetOrFail(instInside);
      int portInside = edge->units[0].port;
      int outsidePort = edge->units[1].port;

      for(EdgeNode* edgeNode = graph->edges; edgeNode; edgeNode = edgeNode->next){
         if(!(edgeNode->out.node == node && edgeNode->out.port == outsidePort)){
            continue;
         }

         listPos = AddEdge(graph,nodeInside,portInside,edgeNode->in.node,edgeNode->in.port,listPos);
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

String OutputDotGraph(Graph* graph,Arena* output){
   Byte* mark = MarkArena(output);

   PushString(output,"digraph accel {\n\tnode [fontcolor=white,style=filled,color=\"160,60,176\"];\n");
   for(Node* ptr = graph->nodes; ptr; ptr = ptr->next){
      ArenaMarker marker(nodeArena); // Use node arena as temporary storage for repr

      String res = ptr->name;
      //String res = Repr(ptr->inst,GRAPH_DOT_FORMAT_NAME,nodeArena);
      PushString(output,"    \"%.*s\";\n",UNPACK_SS(res));
   }
   for(EdgeNode* ptr = graph->edges; ptr; ptr = ptr->next){
      ArenaMarker marker(nodeArena); // Use node arena as temporary storage for repr

      String out = ptr->out.node->name;
      String in = ptr->in.node->name;

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


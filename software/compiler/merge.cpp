#include "merge.hpp"
#include "configurations.hpp"
#include "declaration.hpp"
#include "utilsCore.hpp"

#include "debug.hpp"
#include "debugVersat.hpp"
#include "textualRepresentation.hpp"
#include <ctime>
#include <cstdio>
#include <cstdarg>
#include <unordered_map>

bool NodeMappingConflict(PortEdge edge1,PortEdge edge2){
  PortInstance p00 = edge1.units[0];
  PortInstance p01 = edge1.units[1];
  PortInstance p10 = edge2.units[0];
  PortInstance p11 = edge2.units[1];

  /*
    if(!(edge1.units[0].port == edge2.units[0].port && edge1.units[1].port == edge2.units[1].port)){
    return false;
    }
   */

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
    PortEdge nodeMapping00 = {map1.edges[0].units[0],map1.edges[1].units[0]};
    PortEdge nodeMapping01 = {map1.edges[0].units[1],map1.edges[1].units[1]};
    PortEdge nodeMapping10 = {map2.edges[0].units[0],map2.edges[1].units[0]};
    PortEdge nodeMapping11 = {map2.edges[0].units[1],map2.edges[1].units[1]};

    bool res = NodeMappingConflict(nodeMapping00,nodeMapping10) ||
               NodeMappingConflict(nodeMapping01,nodeMapping10) ||
               NodeMappingConflict(nodeMapping00,nodeMapping11) ||
               NodeMappingConflict(nodeMapping01,nodeMapping11);

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

ConsolidationGraph Copy(ConsolidationGraph graph,Arena* out){
  ConsolidationGraph res = {};

  res = graph;
  res.validNodes.Init(out,graph.nodes.size);
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
bool EqualPortMapping(PortInstance p1,PortInstance p2){
  FUDeclaration* d1 = p1.inst->declaration;
  FUDeclaration* d2 = p2.inst->declaration;

  if(d1 == BasicDeclaration::input && d2 == BasicDeclaration::input){ // Check the special case for inputs
      if(GetInputPortNumber(p1.inst) == GetInputPortNumber(p2.inst)){
        return true;
      } else {
        return false;
      }
  }

  bool res = (d1 == d2);

  return res;
}

inline bool EdgeOrder(Edge* edge,ConsolidationGraphOptions options){
#if 0
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
#endif

  return true;
}

#if 0
int NodeDepth(MappingNode node){
#if 0
  switch(node.type){
  case MappingNode::NODE:{
    FUInstance* inst0 = node.nodes.instances[0];
    FUInstance* inst1 = node.nodes.instances[1];

    return Abs(inst1->graphData->order - inst0->graphData->order);
  }break;
  case MappingNode::EDGE:{
    FUInstance* inst00 = node.edges[0].units[0].inst;
    FUInstance* inst01 = node.edges[0].units[1].inst;
    FUInstance* inst10 = node.edges[1].units[0].inst;
    FUInstance* inst11 = node.edges[1].units[1].inst;

    return Abs(Abs(inst01->graphData->order - inst00->graphData->order) - Abs(inst11->graphData->order - inst10->graphData->order));
  }break;
  default: Assert(false); break;
  }
#endif

  return 0;
}
#endif

ConsolidationResult GenerateConsolidationGraph(Versat* versat,Arena* out,Accelerator* accel1,Accelerator* accel2,ConsolidationGraphOptions options){
  ConsolidationGraph graph = {};

  // Should be temp memory instead of using memory intended for the graph, but since the graph is expected to use a lot of memory and we are technically saving memory using this mapping, no major problem
  Hashmap<FUInstance*,MergeEdge>* specificsMapping = PushHashmap<FUInstance*,MergeEdge>(out,1000);
  Pool<MappingNode> specificsAdded = {};

  for(SpecificMergeNodes specific : options.specifics){
    MergeEdge node = {};
    node.instances[0] = specific.instA;
    node.instances[1] = specific.instB;

    specificsMapping->Insert(specific.instA,node);
    specificsMapping->Insert(specific.instB,node);

    MappingNode* n = specificsAdded.Alloc();
    n->nodes = node;
    n->type = MappingNode::NODE;
  }

  FUInstance* accel1Output = (FUInstance*) GetOutputInstance(accel1->allocated);
  FUInstance* accel2Output = (FUInstance*) GetOutputInstance(accel2->allocated);
#if 1
  // Map outputs
  if(accel1Output && accel2Output){
    MergeEdge node = {};
    node.instances[0] = accel1Output;
    node.instances[1] = accel2Output;

    specificsMapping->Insert(accel1Output,node);
    specificsMapping->Insert(accel2Output,node);

    MappingNode* n = specificsAdded.Alloc();
    n->nodes = node;
    n->type = MappingNode::NODE;
  }

  // Map inputs
  FOREACH_LIST(InstanceNode*,ptr1,accel1->allocated){
    FUInstance* instA = ptr1->inst;
    if(instA->declaration != BasicDeclaration::input){
      continue;
    }

    FOREACH_LIST(InstanceNode*,ptr2,accel2->allocated){
      FUInstance* instB = ptr2->inst;
      if(instB->declaration != BasicDeclaration::input){
        continue;
      }

      if(instA->id != instB->id){
        continue;
      }

      MergeEdge node = {};
      node.instances[0] = instA;
      node.instances[1] = instB;

      specificsMapping->Insert(instA,node);
      specificsMapping->Insert(instB,node);

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

    MappingNode* node = PushStruct<MappingNode>(out);

    node->type = MappingNode::NODE;
    node->nodes.instances[0] = specific.instA;
    node->nodes.instances[1] = specific.instB;

    graph.nodes.size += 1;
  }
#endif

  DynamicArray<MappingNode> nodes = StartArray<MappingNode>(out);
#if 1
  // Check possible edge mapping
  FOREACH_LIST(Edge*,edge1,accel1->edges){
    if(!EdgeOrder(edge1,options)){
      continue;
    }

    FOREACH_LIST(Edge*,edge2,accel2->edges){
      // TODO: some nodes do not care about which port is connected (think the inputs for common operations, like multiplication, adders and the likes)
      // Can augment the algorithm further to find more mappings
      if(!EdgeOrder(edge2,options)){
        continue;
      }

      if(!(EqualPortMapping(edge1->units[0],edge2->units[0]) &&
           EqualPortMapping(edge1->units[1],edge2->units[1]))){
        continue;
      }

      MappingNode node = {};
      node.type = MappingNode::EDGE;
      node.edges[0].units[0] = edge1->units[0];
      node.edges[0].units[1] = edge1->units[1];
      node.edges[1].units[0] = edge2->units[0];
      node.edges[1].units[1] = edge2->units[1];

      // Checks to see if we are in conflict with any specific node.
      MergeEdge* possibleSpecificConflict = specificsMapping->Get(node.edges[0].units[0].inst);
      if(!possibleSpecificConflict) possibleSpecificConflict = specificsMapping->Get(node.edges[0].units[1].inst);
      if(!possibleSpecificConflict) possibleSpecificConflict = specificsMapping->Get(node.edges[1].units[0].inst);
      if(!possibleSpecificConflict) possibleSpecificConflict = specificsMapping->Get(node.edges[1].units[1].inst);

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

      MappingNode* space = nodes.PushElem();

      *space = node;
      graph.nodes.size += 1;
    }
  }
  graph.nodes = EndArray(nodes);
#endif

  // Check node mapping
#if 01
  if(1 /*options.mapNodes */){
    for(FUInstance* instA : accel1->instances){
      PortInstance portA = {};
      portA.inst = instA;

#if 0
      int deltaA = instA->graphData->order - options.order;

      if(options.type == ConsolidationGraphOptions::EXACT_ORDER && deltaA >= 0 && deltaA <= options.difference){
        continue;
      }
#endif
        
      if(instA == accel1Output || instA->declaration == BasicDeclaration::input){
        continue;
      }

      for(FUInstance* instB : accel2->instances){
        PortInstance portB = {};
        portB.inst = instB;

#if 0
        int deltaB = instB->graphData->order - options.order;

        if(options.type == ConsolidationGraphOptions::SAME_ORDER && instA->graphData->order != instB->graphData->order){
          continue;
        }
        if(options.type == ConsolidationGraphOptions::EXACT_ORDER && deltaB >= 0 && deltaB <= options.difference){
          continue;
        }
#endif
        if(instB == accel2Output || instB->declaration == BasicDeclaration::input){
          continue;
        }

        if(!EqualPortMapping(portA,portB)){
          continue;
        }

        MappingNode node = {};
        node.type = MappingNode::NODE;
        node.nodes.instances[0] = instA;
        node.nodes.instances[1] = instB;

        MergeEdge* possibleSpecificConflict = specificsMapping->Get(node.nodes.instances[0]);
        if(!possibleSpecificConflict) possibleSpecificConflict = specificsMapping->Get(node.nodes.instances[1]);

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

        MappingNode* space = PushStruct<MappingNode>(out);

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
  region(out){
    AcceleratorView view1 = CreateAcceleratorView(accel1,out);
    AcceleratorView view2 = CreateAcceleratorView(accel2,out);
    view1.CalculateDAGOrdering(out);
    view2.CalculateDAGOrdering(out);

    Array<int> count = PushArray<int>(out,graph.nodes.size);
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
    Array<int> offsets = PushArray<int>(out,max + 1);
    Memset(offsets,0);
    for(int i = 1; i < offsets.size; i++){
      offsets[i] = offsets[i-1] + count[i-1];
    }
    Array<MappingNode> sorted = PushArray<MappingNode>(out,graph.nodes.size);
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
  }
#endif

  int upperBound = std::min(Size(accel1->edges),Size(accel2->edges));
  Array<BitArray> neighbors = PushArray<BitArray>(out,graph.nodes.size);
  int times = 0;
  for(int i = 0; i < graph.nodes.size; i++){
    neighbors[i].Init(out,graph.nodes.size);
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
  region(out){
    Array<int> degree = PushArray<int>(out,graph.nodes.size);
    Memset(degree,0);
    for(int i = 0; i < graph.nodes.size; i++){
      degree[i] = graph.edges[i].GetNumberBitsSet();
    }
  }
#endif

  graph.validNodes.Init(out,graph.nodes.size);
  graph.validNodes.Fill(1);

  ConsolidationResult res = {};
  res.graph = graph;
  res.upperBound = upperBound;
  res.specificsAdded = specificsAdded;

  return res;
}

CliqueState* InitMaxClique(ConsolidationGraph graph,int upperBound,Arena* out){
  CliqueState* state = PushStruct<CliqueState>(out);
  *state = {};

  state->table = PushArray<int>(out,graph.nodes.size);
  state->clique = Copy(graph,out); // Preserve nodes and edges, but allocates different valid nodes
  state->upperBound = upperBound;

  return state;
}

void Clique(CliqueState* state,ConsolidationGraph graphArg,int index,IndexRecord* record,int size,Arena* temp,Time MAX_CLIQUE_TIME){
  state->iterations += 1;

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
  Time elapsed = end - state->start;
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
    if(size + num <= state->table[index]){
      return;
    }
#endif

    int i = graph.validNodes.FirstBitSetIndex(lastI);

    if(size + state->table[i] <= state->max){
      return;
    }

    graph.validNodes.Set(i,0);

    auto mark = MarkArena(temp);
    ConsolidationGraph tempGraph = Copy(graph,temp);

    tempGraph.validNodes &= graph.edges[i];

    IndexRecord newRecord = {};
    newRecord.index = i;
    newRecord.next = record;

    Clique(state,tempGraph,i,&newRecord,size + 1,temp,MAX_CLIQUE_TIME);

    PopMark(mark);

    if(state->found == true){
      return;
    }

    lastI = i;
  } while((num = graph.validNodes.GetNumberBitsSet()) != 0);
}

void RunMaxClique(CliqueState* state,Arena* arena,Time MAX_CLIQUE_TIME){
  ConsolidationGraph graph = Copy(state->clique,arena);
  graph.validNodes.Fill(0);

  UNHANDLED_ERROR("Implement if gonna use functin again"); // Get time changed, check this later
#if 0
  state->start = GetTime();
#endif

  int startI = state->startI ? state->startI : graph.nodes.size - 1;

  for(int i = startI; i >= 0; i--){
    auto mark = MarkArena(arena);

    state->found = false;
    for(int j = i; j < graph.nodes.size; j++){
      graph.validNodes.Set(j,1);
    }

    graph.validNodes &= graph.edges[i];

    IndexRecord record = {};
    record.index = i;

    printf("%d / %d\r",i,graph.nodes.size);
    Clique(state,graph,i,&record,1,arena,MAX_CLIQUE_TIME);
    state->table[i] = state->max;

    PopMark(mark);

    if(state->max == state->upperBound){
      printf("Upperbound finish, index: %d (from %d)\n",i,graph.nodes.size);
      break;
    }

    UNHANDLED_ERROR("Implement if needed"); // Get time changed, check this later
#if 0
    auto end = GetTime();
    float elapsed = end - state->start;
    if(elapsed > MAX_CLIQUE_TIME){
      printf("Timeout, index: %d (from %d)\n",i,graph.nodes.size);
      break;
    }
#endif
  }
  printf("\nFinished in: %d\n",state->iterations);

  Assert(IsClique(state->clique).result);
}

CliqueState MaxClique(ConsolidationGraph graph,int upperBound,Arena* arena,Time MAX_CLIQUE_TIME){
  CliqueState state = {};
  state.table = PushArray<int>(arena,graph.nodes.size);
  state.clique = Copy(graph,arena); // Preserve nodes and edges, but allocates different valid nodes
  //state.start = std::chrono::steady_clock::now();

  graph.validNodes.Fill(0);

  //printf("Upper:%d\n",upperBound);

  state.start = GetTime();
  for(int i = graph.nodes.size - 1; i >= 0; i--){
    auto mark = MarkArena(arena);

    state.found = false;
    for(int j = i; j < graph.nodes.size; j++){
      graph.validNodes.Set(j,1);
    }

    graph.validNodes &= graph.edges[i];

    IndexRecord record = {};
    record.index = i;

    Clique(&state,graph,i,&record,1,arena,MAX_CLIQUE_TIME);
    state.table[i] = state.max;

    PopMark(mark);

    if(state.max == upperBound){
      //printf("Upperbound finish, index: %d (from %d)\n",i,graph.nodes.size);
      break;
    }

    auto end = GetTime();
    Time elapsed = end - state.start;
    if(elapsed > MAX_CLIQUE_TIME){
      //printf("Timeout, index: %d (from %d)\n",i,graph.nodes.size);
      break;
    }
  }

  Assert(IsClique(state.clique).result);

  return state;
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

    fprintf(outputFile,"\t\"%.*s\";\n",UNPACK_SS(Repr(node,memory)));
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
      String str1 = Repr(node1,memory);
      String str2 = Repr(node2,memory);

      fprintf(outputFile,"\t\"%.*s\" -- \"%.*s\";\n",UNPACK_SS(str1),UNPACK_SS(str2));
    }
  }
#endif

  fprintf(outputFile,"}\n");
  fclose(outputFile);
}

void DoInsertMapping(InstanceMap& map,FUInstance* inst1,FUInstance* inst0){
  auto iter = map.find(inst1);

  if(iter != map.end()){
    Assert(iter->second == inst0);
    return;
  }

  map.insert({inst1,inst0});
}

void InsertMapping(GraphMapping& map,FUInstance* inst1,FUInstance* inst0){
  DoInsertMapping(map.instanceMap,inst1,inst0);
  DoInsertMapping(map.reverseInstanceMap,inst0,inst1);
}

void InsertMapping(GraphMapping& map,PortEdge& edge0,PortEdge& edge1){
  map.edgeMap.insert({edge1,edge0});

  InsertMapping(map,edge1.units[0].inst,edge0.units[0].inst);
  InsertMapping(map,edge1.units[1].inst,edge0.units[1].inst);
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

GraphMapping ConsolidationGraphMapping(Versat* versat,Accelerator* accel1,Accelerator* accel2,ConsolidationGraphOptions options,Arena* arena,String name){
  ArenaMarker marker(arena);

  ConsolidationResult result = GenerateConsolidationGraph(versat,arena,accel1,accel2,options);
  ConsolidationGraph graph = result.graph;

#if 0
  printf("%d\n",graph.validNodes.bitSize);
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

  if(graph.validNodes.bitSize == 0){
    return res;
  }

  if(versat->debug.outputGraphs){
    OutputConsolidationGraph(graph,arena,true,"debug/%.*s/ConsolidationGraph.dot",UNPACK_SS(name));
  }
    
  int upperBound = result.upperBound;

#if 0
  ConsolidationGraph clique = ParallelMaxClique(graph,upperBound,arena,Seconds(10));
#else
  ConsolidationGraph clique = MaxClique(graph,upperBound,arena,Seconds(10)).clique;
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

  if(versat->debug.outputGraphs){
    OutputConsolidationGraph(clique,arena,true,"debug/%.*s/Clique1.dot",UNPACK_SS(name));
  }
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

#if 0
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
#endif

  NOT_IMPLEMENTED("Not fully implemented. Missing edge mappings");
  return res;
}

#include <algorithm>

#if 0
struct {
  bool operator()(FUInstance* f1, FUInstance* f2) const {
    bool res = false;
    if(f1->graphData->order == f2->graphData->order){
      res = (f1->declaration < f2->declaration);
    } else {
      res = (f1->graphData->order < f2->graphData->order);
    }
    return res;
  }
} compare;
#endif

void OrderedMatch(std::vector<FUInstance*>& order1,std::vector<FUInstance*>& order2,InstanceMap& map,int orderOffset){
  UNHANDLED_ERROR("Implement if needed");
#if 0
  auto iter1 = order1.begin();
  auto iter2 = order2.begin();

  // Match same order
  for(; iter1 != order1.end() && iter2 != order2.end();){
    FUInstance* inst1 = *iter1;
    FUInstance* inst2 = *iter2;

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
#endif
}

struct OverheadCount{
  int muxes;
  int buffers;
};

OverheadCount CountOverheadUnits(Versat* versat,Accelerator* accel){
  OverheadCount res = {};

  UNHANDLED_ERROR("Implement if needed");
#if 0
  FOREACH_LIST(inst,accel->instances){
  FUDeclaration* decl = inst->declaration;

  if(decl == BasicDeclaration::multiplexer || decl == BasicDeclaration::combMultiplexer){
    res.muxes += 1;
  }

  if(decl == BasicDeclaration::buffer || decl == BasicDeclaration::fixedBuffer){
    res.buffers += 1;
  }
}
#endif

  return res;
}

void PrintAcceleratorStats(Versat* versat,Accelerator* accel){
  OverheadCount count = CountOverheadUnits(versat,accel);
  printf("Instances:%d Edges:%d Muxes:%d Buffer:%d",Size(accel->allocated),Size(accel->edges),count.muxes,count.buffers);
}

#if 0
static int Weight(Versat* versat,Accelerator* accel,bool countBuffer = false){
  int weight = 0;
  UNHANDLED_ERROR;
#if 0
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
#endif
  return weight;
}
#endif

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
  UNHANDLED_ERROR("Implement if needed");
#if 0
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
#endif
}

static GraphMapping MergeAccelerator(Versat* versat,Accelerator* accel1,Accelerator* accel2,Array<SpecificMergeNodes> specificNodes,MergingStrategy strategy,String name){
  Arena* temp = &versat->temp;
  BLOCK_REGION(temp);
  
  GraphMapping graphMapping = {};

  String debugAccel1 = PushString(temp,"debug/%.*s/accel1.dot",UNPACK_SS(name));
  String debugAccel2 = PushString(temp,"debug/%.*s/accel2.dot",UNPACK_SS(name));
  OutputGraphDotFile(versat,accel1,true,debugAccel1,temp);
  OutputGraphDotFile(versat,accel2,true,debugAccel2,temp);
   
  switch(strategy){
  case MergingStrategy::SIMPLE_COMBINATION:{
    // Do nothing, no mapping leads to simple combination
  }break;
  case MergingStrategy::CONSOLIDATION_GRAPH:{
    ConsolidationGraphOptions options = {};
    options.mapNodes = false;
    options.specifics = specificNodes;

    graphMapping = ConsolidationGraphMapping(versat,accel1,accel2,options,temp,name);
  }break;
  case MergingStrategy::FIRST_FIT:{
    graphMapping = FirstFitGraphMapping(versat,accel1,accel2,temp);
  }break;
  }

  return graphMapping;
}

MergeGraphResult MergeGraph(Versat* versat,Accelerator* flatten1,Accelerator* flatten2,GraphMapping& graphMapping,String name,Arena* out){
  int size1 = Size(flatten1->allocated);
  int size2 = Size(flatten2->allocated);

  // This maps are returned
  Hashmap<InstanceNode*,InstanceNode*>* map1 = PushHashmap<InstanceNode*,InstanceNode*>(out,size1);
  Hashmap<InstanceNode*,InstanceNode*>* map2 = PushHashmap<InstanceNode*,InstanceNode*>(out,size2);

  BLOCK_REGION(out);

  Hashmap<FUInstance*,FUInstance*>* map = PushHashmap<FUInstance*,FUInstance*>(out,size1 + size2);

  Accelerator* newGraph = CreateAccelerator(versat,name);

  // Create base instances from accel 1
  FOREACH_LIST(InstanceNode*,ptr,flatten1->allocated){
    FUInstance* inst = ptr->inst;

    InstanceNode* newNode = CopyInstance(newGraph,ptr,inst->name);

    map->Insert(inst,newNode->inst); // Maps from old instance in flatten1 to new inst in newGraph
    map1->Insert(newNode,ptr);
  }

  // Create base instances from accel 2, unless they are mapped to nodes from accel1
  FOREACH_LIST(InstanceNode*,ptr,flatten2->allocated){
    FUInstance* inst = ptr->inst;

    auto mapping = graphMapping.instanceMap.find(inst); // Returns mapping from accel2 to accel1

    FUInstance* mappedNode = nullptr;
    if(mapping != graphMapping.instanceMap.end()){
      mappedNode = map->GetOrFail(mapping->second);

      // For now, change name even if they have the same name
      //if(!CompareString(inst->name,mappedNode->name)){
      String newName = PushString(&versat->permanent,"%.*s_%.*s",UNPACK_SS(mappedNode->name),UNPACK_SS(inst->name));

      mappedNode->name = newName;
      //}
    } else {
      mappedNode = CopyInstance(newGraph,inst,inst->name);
      //mappedNode = (FUInstance*) CreateFUInstance(newGraph,inst->declaration,inst->name);
    }
    InstanceNode* newNode = GetInstanceNode(newGraph,mappedNode);
    map->Insert(inst,mappedNode);
    map2->Insert(newNode,ptr);
  }

  // Create base edges from accel 1
  FOREACH_LIST(Edge*,edge,flatten1->edges){
    FUInstance* mappedInst = map->GetOrFail(edge->units[0].inst);
    FUInstance* mappedOther = map->GetOrFail(edge->units[1].inst);

    ConnectUnitsGetEdge(mappedInst,edge->units[0].port,mappedOther,edge->units[1].port,edge->delay);
  }

  // Create base edges from accel 2, unless they are mapped to edges from accel 1
  FOREACH_LIST(Edge*,edge,flatten2->edges){
    PortEdge searchEdge = {};
    searchEdge.units[0] = edge->units[0];
    searchEdge.units[1] = edge->units[1];

    auto iter = graphMapping.edgeMap.find(searchEdge);

    if(iter != graphMapping.edgeMap.end()){
      FUInstance* mappedInst = map->GetOrFail(edge->units[0].inst);
      FUInstance* mappedOther = map->GetOrFail(edge->units[1].inst);

      Edge* oldEdge = FindEdge(mappedInst,edge->units[0].port,mappedOther,edge->units[1].port,edge->delay);

      if(oldEdge){ // Edge between port instances might exist but different delay means we need to create another one
            continue;
      }
    }

    FUInstance* mappedInst = map->GetOrFail(edge->units[0].inst);
    FUInstance* mappedOther = map->GetOrFail(edge->units[1].inst);
    ConnectUnitsIfNotConnectedGetEdge(mappedInst,edge->units[0].port,mappedOther,edge->units[1].port,edge->delay);
  }

  MergeGraphResult res = {};
  res.newGraph = newGraph;
  res.map1 = map1;
  res.map2 = map2;
  res.accel1 = flatten1;
  res.accel2 = flatten2;

  return res;
}

MergeGraphResultExisting MergeGraphToExisting(Versat* versat,Accelerator* existing,Accelerator* flatten2,GraphMapping& graphMapping,String name,Arena* out){
  int size1 = Size(existing->allocated);
  int size2 = Size(flatten2->allocated);

  // This maps are returned
  Hashmap<InstanceNode*,InstanceNode*>* map2 = PushHashmap<InstanceNode*,InstanceNode*>(out,size2);

  BLOCK_REGION(out);

  Hashmap<FUInstance*,FUInstance*>* map = PushHashmap<FUInstance*,FUInstance*>(out,size1 + size2);

  FOREACH_LIST(InstanceNode*,ptr,existing->allocated){
    map->Insert(ptr->inst,ptr->inst);
  }

  // Create base instances from accel 2, unless they are mapped to nodes from accel1
  FOREACH_LIST(InstanceNode*,ptr,flatten2->allocated){
    FUInstance* inst = ptr->inst;

    auto mapping = graphMapping.instanceMap.find(inst); // Returns mapping from accel2 to accel1

    FUInstance* mappedNode = nullptr;
    if(mapping != graphMapping.instanceMap.end()){
      mappedNode = map->GetOrFail(mapping->second);

      // For now, change name even if they have the same name
      //if(!CompareString(inst->name,mappedNode->name)){
      String newName = PushString(&versat->permanent,"%.*s_%.*s",UNPACK_SS(mappedNode->name),UNPACK_SS(inst->name));

      mappedNode->name = newName;
      //}
    } else {
      mappedNode = CopyInstance(existing,inst,inst->name);
      //mappedNode = (FUInstance*) CreateFUInstance(existing,inst->declaration,inst->name);
    }
    InstanceNode* newNode = GetInstanceNode(existing,mappedNode);
    map->Insert(inst,mappedNode);
    map2->Insert(ptr,newNode);
  }

  // Create base edges from accel 2, unless they are mapped to edges from accel 1
  FOREACH_LIST(Edge*,edge,flatten2->edges){
    PortEdge searchEdge = {};
    searchEdge.units[0] = edge->units[0];
    searchEdge.units[1] = edge->units[1];

    auto iter = graphMapping.edgeMap.find(searchEdge);

    if(iter != graphMapping.edgeMap.end()){
      FUInstance* mappedInst = map->GetOrFail(edge->units[0].inst);
      FUInstance* mappedOther = map->GetOrFail(edge->units[1].inst);

      Edge* oldEdge = FindEdge(mappedInst,edge->units[0].port,mappedOther,edge->units[1].port,edge->delay);

      if(oldEdge){ // Edge between port instances might exist but different delay means we need to create another one
            continue;
      }
    }

    FUInstance* mappedInst = map->GetOrFail(edge->units[0].inst);
    FUInstance* mappedOther = map->GetOrFail(edge->units[1].inst);
    ConnectUnitsIfNotConnectedGetEdge(mappedInst,edge->units[0].port,mappedOther,edge->units[1].port,edge->delay);
  }

  MergeGraphResultExisting res = {};
  res.result = existing;
  res.map2 = map2;
  res.accel2 = flatten2;

  return res;
}

FUDeclaration* MergeAccelerators(Versat* versat,FUDeclaration* accel1,FUDeclaration* accel2,String name,
                                 int flatteningOrder,MergingStrategy strategy,SpecificMerge* specifics,int nSpecifics){
  Arena* arena = &versat->temp;
  ArenaMarker marker(arena);

  Accelerator* flatten1 = Flatten(versat,accel1->baseCircuit,99);
  Accelerator* flatten2 = Flatten(versat,accel2->baseCircuit,99);

  Array<SpecificMergeNodes> specificNodes = PushArray<SpecificMergeNodes>(arena,nSpecifics);
#if 0
  for(int i = 0; i < nSpecifics; i++){
    specificNodes[i].instA = (FUInstance*) GetInstanceByName(flatten1,specifics[i].instA);
    specificNodes[i].instB = (FUInstance*) GetInstanceByName(flatten2,specifics[i].instB);
  }
#endif

  region(arena){
    String filepath = PushString(arena,"debug/%.*s/flatten1.dot",UNPACK_SS(name));
    OutputGraphDotFile(versat,flatten1,true,filepath,arena);
  }
  region(arena){
    String filepath = PushString(arena,"debug/%.*s/flatten2.dot",UNPACK_SS(name));
    OutputGraphDotFile(versat,flatten2,true,filepath,arena);
  }

  GraphMapping graphMapping = MergeAccelerator(versat,flatten1,flatten2,specificNodes,strategy,name);

  MergeGraphResult result = MergeGraph(versat,flatten1,flatten2,graphMapping,name,arena);
  Accelerator* newGraph = result.newGraph;

  region(arena){
    newGraph->name = name;
  }

  FUDeclaration* decl = RegisterSubUnit(versat,newGraph);
  
  return decl;
}

Array<int> GetPortConnections(InstanceNode* node,Arena* arena){
  int maxPorts = 0;
  FOREACH_LIST(ConnectionNode*,con,node->allInputs){
    maxPorts = std::max(maxPorts,con->port);
  }

  Array<int> res = PushArray<int>(arena,maxPorts + 1);
  Memset(res,0);

  FOREACH_LIST(ConnectionNode*,con,node->allInputs){
    res[con->port] += 1;
  }

  return res;
}

Optional<int> GetConfigurationIndexFromInstanceNode(FUDeclaration* type,InstanceNode* node){
  // While configuration array is fully defined, no need to do this check beforehand.
  int index = 0;
  FOREACH_LIST_INDEXED(InstanceNode*,iter,type->fixedDelayCircuit->allocated,index){
    if(iter == node){
      return type->configInfo.configOffsets.offsets[index];
    }
  }

  Assert(false);
  return Optional<int>{};
}

FUDeclaration* Merge(Versat* versat,Array<FUDeclaration*> types,String name,MergingStrategy strat){
  Assert(types.size >= 2);

  strat = MergingStrategy::CONSOLIDATION_GRAPH;
  
  Arena* temp = &versat->temp;
  Arena* perm = &versat->permanent;
  BLOCK_REGION(temp);

  int size = types.size;
  Array<Accelerator*> flatten = PushArray<Accelerator*>(temp,size);
  Array<InstanceNodeMap*> view = PushArray<InstanceNodeMap*>(temp,size);
  Array<InstanceNodeMap*> reverseView = PushArray<InstanceNodeMap*>(temp,size);
  Array<GraphMapping> mappings = PushArray<GraphMapping>(temp,size - 1);
  
  Array<SpecificMergeNodes> specificNodes = {}; // For now, ignore specifics.

  for(int i = 0; i < size; i++){
    flatten[i] = Flatten(versat,types[i]->baseCircuit,99);
  }
  
  for(int i = 0; i < size - 1; i++){
    new (&mappings[i].instanceMap) InstanceMap; // Maps from accel2 to accel1
    new (&mappings[i].reverseInstanceMap) InstanceMap; // Maps from accel1 to accel2
    new (&mappings[i].edgeMap) InstanceMap;
  }
  
  // We copy accel 1.
  Accelerator* result = CopyAccelerator(versat,flatten[0],nullptr);

  InstanceNodeMap* initialMap = PushHashmap<InstanceNode*,InstanceNode*>(temp,Size(result->allocated));

  InstanceNode* ptr1 = result->allocated;
  InstanceNode* ptr2 = flatten[0]->allocated;

  for(; ptr1 != nullptr && ptr2 != nullptr; ptr1 = ptr1->next,ptr2 = ptr2->next){
    initialMap->Insert(ptr2,ptr1);
  }
  Assert(ptr1 == nullptr && ptr2 == nullptr);
  view[0] = initialMap; // Merges from flattened 0 into copied accelerator. 
  
  for(int i = 1; i < size; i++){
    String tempName = STRING(StaticFormat("Merge%d",i));
    mappings[i-1] = MergeAccelerator(versat,result,flatten[i],specificNodes,strat,tempName);
    MergeGraphResultExisting res = MergeGraphToExisting(versat,result,flatten[i],mappings[i-1],tempName,temp);
    
    view[i] = res.map2;
    result = res.result;
  }

  // TODO: This might indicate the need to implement a bidirectional map.
  for(int i = 0; i < size; i++){
    InstanceNodeMap* map = view[i];

    reverseView[i] = PushHashmap<InstanceNode*,InstanceNode*>(temp,map->nodesUsed);
    for(Pair<InstanceNode*,InstanceNode*> p : map){
      reverseView[i]->Insert(p.second,p.first);
    }
  }
  

  // Need to map from final to each input graph
  int numberOfMultipleInputs = 0;
  FOREACH_LIST(InstanceNode*,ptr,result->allocated){
    if(ptr->multipleSamePortInputs){
      numberOfMultipleInputs += 1;
    }
  }

  Array<InstanceNodeMap*> mapMultipleInputsToSubgraphs = PushArray<InstanceNodeMap*>(temp,size);

  for(int i = 0; i < size; i++){
    mapMultipleInputsToSubgraphs[i] = PushHashmap<InstanceNode*,InstanceNode*>(temp,numberOfMultipleInputs);

    for(Pair<InstanceNode*,InstanceNode*> pair : view[i]){
      if(pair.second->multipleSamePortInputs){
        mapMultipleInputsToSubgraphs[i]->Insert(pair.second,pair.first);
      }
    }
  }

  FUDeclaration* muxType = GetTypeByName(versat,STRING("CombMux4"));
  int multiplexersAdded = 0;
  FOREACH_LIST(InstanceNode*,ptr,result->allocated){
    if(ptr->multipleSamePortInputs){
      // Need to figure out which port, (which might be multiple), has the same inputs
      Array<int> portConnections = GetPortConnections(ptr,temp);

      for(int port = 0; port < portConnections.size; port++){
        if(portConnections[port] < 2){
          continue;
        }

        int numberOfMultiple = portConnections[port];

        //printf("%d %d:",port,numberOfMultiple);

        // Collect the problematic edges for this port
        Array<EdgeNode> problematicEdgesInFinalGraph = PushArray<EdgeNode>(temp,numberOfMultiple);
        int index = 0;

        FOREACH_LIST(ConnectionNode*,con,ptr->allInputs){
          if(con->port == port){
            EdgeNode node = {};
            node.node0 = con->instConnectedTo;
            node.node1 = {ptr,con->port};
            problematicEdgesInFinalGraph[index++] = node;
          }
        }

        // Need to map edgeNode into the integer that indicates which input graph it belongs to.
        Array<int> inputNumberToEdgeNodeIndex = PushArray<int>(temp,size);
        Memset(inputNumberToEdgeNodeIndex,0);

        for(int i = 0; i < problematicEdgesInFinalGraph.size; i++){
          EdgeNode edgeNode = problematicEdgesInFinalGraph[i];

          for(int ii = 0; ii < size; ii++){
            InstanceNode** possibleNode = mapMultipleInputsToSubgraphs[ii]->Get(edgeNode.node1.node);

            if(possibleNode){
              InstanceNode* node = *possibleNode;

              // We find the other node that matches what we expected. If we add a general map from finalResult to input graph
              // Kinda like we have with the mapMultipleInputsToSubgraphs, except that we can map all the nodes instead of only the multiple inputs
              // Then we could use the map here, instead of searching the all inputs of the node.
              // TODO: The correct approach would be to use a Bimap to perform the mapping from final graph to inputs, and to not use the mapMultipleInputsToSubgraphs (because this is just an incomplete map, if we add a complete map we could do the same thing)
              FOREACH_LIST(ConnectionNode*,con,node->allInputs){
                InstanceNode* other = view[ii]->GetOrFail(con->instConnectedTo.node);

                // In here, we could save edge information for the view graphs, so that we could add a multiplexer to the view graph later
                // Do not know yet if needed.
                if(con->port == port && other == edgeNode.node0.node && con->instConnectedTo.port == edgeNode.node0.port){
                  inputNumberToEdgeNodeIndex[ii] = i;
                  break;
                }
              }
            }
          }
        }

        // At this point, we know the edges that we need to remove and to replace with a multiplexer and the
        // port to which they should connect.
        const char* format = "versat_merge_mux";

        String str = PushString(&versat->permanent,format,multiplexersAdded++);
        PushNullByte(&versat->permanent);
        InstanceNode* multiplexer = CreateFlatFUInstance(result,muxType,str);
        SetStatic(result,multiplexer->inst);
        
        for(int i = 0; i < problematicEdgesInFinalGraph.size; i++){
          EdgeNode node = problematicEdgesInFinalGraph[i];

          for(int ii = 0; ii < inputNumberToEdgeNodeIndex.size; ii++){
            int edgeIndex = inputNumberToEdgeNodeIndex[ii];
            if(edgeIndex != i){
              continue;
            }

            int graphIndex = ii;

            RemoveConnection(result,node.node0.node,node.node0.port,node.node1.node,node.node1.port);
            ConnectUnitsGetEdge(node.node0,{multiplexer,graphIndex},0);
          }
        }

        ConnectUnitsGetEdge({multiplexer,0},{ptr,port},0);
      }
    }
  }

  if(versat->debug.outputGraphs){
    for(int i = 0; i < size; i++){
      BLOCK_REGION(temp);
      Set<FUInstance*>* firstGraph = PushSet<FUInstance*>(temp,Size(flatten[i]->allocated));
      FOREACH_LIST(InstanceNode*,ptr,flatten[i]->allocated){
        InstanceNode* finalNode = view[i]->GetOrFail(ptr);

        firstGraph->Insert(finalNode->inst);
      }

      String filepath = PushString(temp,"debug/%.*s/finalMerged_%d.dot",UNPACK_SS(name),i);
      OutputGraphDotFile(versat,result,false,firstGraph,filepath,temp);
    }
  }
  
  // I'm registing the entire sub unit so what I probably want is to simplify the entire registerSubUnit process and only then tackle this next step.
  String permanentName = PushString(&versat->permanent,name);
  result->name = permanentName;
  FUDeclaration* decl = RegisterSubUnit(versat,result);
  
  int mergedConfigSize = decl->configInfo.configOffsets.offsets.size;
  decl->mergeInfo = PushArray<MergeInfo>(&versat->permanent,size);
  for(int i = 0; i < size; i++){
    InstanceNodeMap* map = reverseView[i];

    decl->mergeInfo[i].baseType = types[i];
    decl->mergeInfo[i].name = types[i]->name;
    decl->mergeInfo[i].config.configOffsets.offsets = PushArray<int>(&versat->permanent,mergedConfigSize);
    decl->mergeInfo[i].config.configOffsets.max = decl->configInfo.configOffsets.max;
    decl->mergeInfo[i].baseName = PushArray<String>(&versat->permanent,mergedConfigSize);
    
    int configIndex = 0;
    FOREACH_LIST_INDEXED(InstanceNode*,ptr,result->allocated,configIndex){
      if(map->Exists(ptr)){
        int configOffset = decl->configInfo.configOffsets.offsets[configIndex];
        decl->mergeInfo[i].config.configOffsets.offsets[configIndex] = configOffset;

        InstanceNode* originalNode = map->GetOrFail(ptr); // TODO: This is the way of getting the actual name that we want to put into the generated structures.
        decl->mergeInfo[i].baseName[configIndex] = originalNode->inst->name;
      } else {
        decl->mergeInfo[i].baseName[configIndex] = STRING("");
        decl->mergeInfo[i].config.configOffsets.offsets[configIndex] = -1;
      }
    }
  }
  
  return decl;
}


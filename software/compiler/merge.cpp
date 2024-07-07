#include "merge.hpp"
#include "accelerator.hpp"
#include "configurations.hpp"
#include "declaration.hpp"
#include "dotGraphPrinting.hpp"
#include "globals.hpp"
#include "logger.hpp"
#include "memory.hpp"
#include "utils.hpp"
#include "utilsCore.hpp"

#include "debug.hpp"
#include "debugVersat.hpp"
#include "textualRepresentation.hpp"
#include "versat.hpp"
#include <ctime>
#include <cstdio>
#include <cstdarg>
#include <unordered_map>

bool NodeMappingConflict(Edge edge1,Edge edge2){
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
    Edge nodeMapping00 = {map1.edges[0].units[0],map1.edges[1].units[0]};
    Edge nodeMapping01 = {map1.edges[0].units[1],map1.edges[1].units[1]};
    Edge nodeMapping10 = {map2.edges[0].units[0],map2.edges[1].units[0]};
    Edge nodeMapping11 = {map2.edges[0].units[1],map2.edges[1].units[1]};
    
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

int EdgeEqual(Edge edge1,Edge edge2){
  int res = (memcmp(&edge1,&edge2,sizeof(Edge)) == 0);

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
  }
#endif

  return 0;
}
#endif

ConsolidationResult GenerateConsolidationGraph(Accelerator* accel0,Accelerator* accel1,ConsolidationGraphOptions options,Arena* out,Arena* temp){
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

  FUInstance* accel0Output = (FUInstance*) GetOutputInstance(accel0->allocated);
  FUInstance* accel1Output = (FUInstance*) GetOutputInstance(accel1->allocated);
#if 1
  // Map outputs
  if(accel0Output && accel1Output){
    MergeEdge node = {};
    node.instances[0] = accel0Output;
    node.instances[1] = accel1Output;

    specificsMapping->Insert(accel0Output,node);
    specificsMapping->Insert(accel1Output,node);

    MappingNode* n = specificsAdded.Alloc();
    n->nodes = node;
    n->type = MappingNode::NODE;
  }

  // Map inputs
  FOREACH_LIST(FUInstance*,ptr1,accel0->allocated){
    FUInstance* instA = ptr1;
    if(instA->declaration != BasicDeclaration::input){
      continue;
    }

    FOREACH_LIST(FUInstance*,ptr2,accel1->allocated){
      FUInstance* instB = ptr2;
      if(instB->declaration != BasicDeclaration::input){
        continue;
      }

      if(instA->portIndex != instB->portIndex){
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

  Array<Edge> accel0Edges = GetAllEdges(accel0,temp);
  Array<Edge> accel1Edges = GetAllEdges(accel1,temp);

  for(Edge& edge0 : accel0Edges){
    for(Edge& edge1 : accel1Edges){
  // TODO: some nodes do not care about which port is connected (think the inputs for common operations, like multiplication, adders and the likes)
      // Can augment the algorithm further to find more mappings
      if(!(EqualPortMapping(edge0.units[0],edge1.units[0]) &&
           EqualPortMapping(edge0.units[1],edge1.units[1]))){
        continue;
      }

      if(edge0.delay != edge1.delay){
        continue;
      }
      
      MappingNode node = {};
      node.type = MappingNode::EDGE;
      node.edges[0].units[0] = edge0.units[0];
      node.edges[0].units[1] = edge0.units[1];
      node.edges[1].units[0] = edge1.units[0];
      node.edges[1].units[1] = edge1.units[1];
      
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

      MappingNode* space = nodes.PushElem();

      *space = node;
      graph.nodes.size += 1;
    }
  }
  graph.nodes = EndArray(nodes);
#endif

  // Check node mapping
#if 1
  if(1 /*options.mapNodes */){
    for(FUInstance* instA : accel0->instances){
      PortInstance portA = {};
      portA.inst = instA;

      if(instA == accel0Output || instA->declaration == BasicDeclaration::input){
        continue;
      }

      for(FUInstance* instB : accel1->instances){
        PortInstance portB = {};
        portB.inst = instB;

        if(instB == accel1Output || instB->declaration == BasicDeclaration::input){
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
    AcceleratorView view1 = CreateAcceleratorView(accel0,out);
    AcceleratorView view2 = CreateAcceleratorView(accel1,out);
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

  int upperBound = std::min(accel0Edges.size,accel1Edges.size);
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

void OutputConsolidationGraph(ConsolidationGraph graph,bool onlyOutputValid,String moduleName,String fileName,Arena* temp){
  BLOCK_REGION(temp);
  String filePath = PushDebugPath(temp,moduleName,fileName);
  
  FILE* outputFile = OpenFileAndCreateDirectories(StaticFormat("%.*s",UNPACK_SS(filePath)),"w");
  DEFER_CLOSE_FILE(outputFile);

  fprintf(outputFile,"graph GraphName {\n\tnode [fontcolor=white,style=filled,color=\"160,60,176\"];\n");
  for(int i = 0; i < graph.nodes.size; i++){
    MappingNode* node = &graph.nodes[i];

    if(onlyOutputValid && !graph.validNodes.Get(i)){
      continue;
    }

    BLOCK_REGION(temp);

    fprintf(outputFile,"\t\"%.*s\";\n",UNPACK_SS(Repr(node,temp)));
  }

  for(int i = 0; i < graph.nodes.size; i++){
    if(!graph.validNodes.Get(i)){
      continue;
    }

    MappingNode* node1 = &graph.nodes[i];
    for(int ii = i + 1; ii < graph.edges.size; ii++){
      if(!graph.validNodes.Get(ii)){
        continue;
      }
      if(!graph.edges[i].Get(ii)){
        continue;
      }

      MappingNode* node2 = &graph.nodes[ii];

      ArenaMarker marker(temp);
      String str1 = Repr(node1,temp);
      String str2 = Repr(node2,temp);

      fprintf(outputFile,"\t\"%.*s\" -- \"%.*s\";\n",UNPACK_SS(str1),UNPACK_SS(str2));
    }
  }

  fprintf(outputFile,"}\n");
}

void DoInsertMapping(InstanceMap* map,FUInstance* inst1,FUInstance* inst0){
  if(map->Exists(inst1)){
    Assert(map->GetOrFail(inst1) == inst0);
  } else {
    map->Insert(inst1,inst0);
  }
}

void InsertMapping(GraphMapping& map,FUInstance* inst1,FUInstance* inst0){
  DoInsertMapping(map.instanceMap,inst1,inst0);
  DoInsertMapping(map.reverseInstanceMap,inst0,inst1);
}

void InsertMapping(GraphMapping& map,Edge& edge0,Edge& edge1){
  map.edgeMap->Insert(edge1,edge0);

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
      Edge& edge0 = node.edges[0]; // Edge in graph 1
      Edge& edge1 = node.edges[1]; // Edge in graph 2

      InsertMapping(res,edge1.units[0].inst,edge0.units[0].inst);
      InsertMapping(res,edge1.units[1].inst,edge0.units[1].inst);

      res.edgeMap->Insert(edge1,edge0);
    }
  }
}

GraphMapping InitGraphMapping(Arena* out){
  GraphMapping mapping = {};
  mapping.instanceMap = PushHashmap<FUInstance*,FUInstance*>(out,999);
  mapping.reverseInstanceMap = PushHashmap<FUInstance*,FUInstance*>(out,999);
  mapping.edgeMap = PushHashmap<Edge,Edge>(out,999);
  
  return mapping;
}

int ValidNodes(ConsolidationGraph graph){
  int count = 0;
  for(int b : graph.validNodes){
    count += 1;
  }
  return count;
}

GraphMapping ConsolidationGraphMapping(Accelerator* accel1,Accelerator* accel2,ConsolidationGraphOptions options,String name,Arena* temp,Arena* out){
  BLOCK_REGION(temp);

  ConsolidationResult result = {};
  region(out){
    result = GenerateConsolidationGraph(accel1,accel2,options,temp,out);
  }
  ConsolidationGraph graph = result.graph;

  if(globalDebug.outputGraphs){
    OutputConsolidationGraph(graph,true,name,STRING("ConsolidationGraph.dot"),temp);
  }

  GraphMapping res = InitGraphMapping(out);

  for(MappingNode* n : result.specificsAdded){
    MappingNode node = *n;

    if(node.type == MappingNode::NODE){
      MergeEdge& nodes = node.nodes;

      InsertMapping(res,nodes.instances[1],nodes.instances[0]);
    } else { // Edge mapping
         Edge& edge0 = node.edges[0]; // Edge in graph 1
         Edge& edge1 = node.edges[1]; // Edge in graph 2

         InsertMapping(res,edge1.units[0].inst,edge0.units[0].inst);
         InsertMapping(res,edge1.units[1].inst,edge0.units[1].inst);

         res.edgeMap->Insert(edge1,edge0);
    }
  }

  if(graph.validNodes.bitSize == 0){
    return res;
  }
    
  int upperBound = result.upperBound;
  upperBound = 9999999; // TODO: Upperbound not working correctly.
  ConsolidationGraph clique = MaxClique(graph,upperBound,temp,Seconds(1)).clique;

#if 1
  printf("Clique: %d\n",ValidNodes(clique));
  for(int i = 0; i < clique.nodes.size; i++){
    if(clique.validNodes.Get(i)){
      //String repr = Repr(result.graph.nodes[i],temp);
      //printf("%.*s\n",UNPACK_SS(repr));
    }
  }
#endif

  if(globalDebug.outputGraphs){
    OutputConsolidationGraph(clique,true,name,STRING("Clique1.dot"),temp);
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

static GraphMapping FirstFitGraphMapping(Accelerator* accel1,Accelerator* accel2,Arena* arena){
  GraphMapping res = InitGraphMapping(arena);

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

OverheadCount CountOverheadUnits(Accelerator* accel){
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

void PrintMergePossibility(Accelerator* accel1,Accelerator* accel2){
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

static GraphMapping MergeAccelerator(Accelerator* accel1,Accelerator* accel2,Array<SpecificMergeNodes> specificNodes,MergingStrategy strategy,String name,Arena* temp,Arena* out){
  BLOCK_REGION(temp);
  
  GraphMapping graphMapping = InitGraphMapping(out);

#if 0
  OutputDebugDotGraph(accel1,STRING("accel1.dot"),temp);
  OutputDebugDotGraph(accel2,STRING("accel2.dot"),temp);
#endif
   
  switch(strategy){
  case MergingStrategy::SIMPLE_COMBINATION:{
    // Only map inputs and outputs
    FUInstance* accel1Output = GetOutputInstance(accel1->allocated);
    FUInstance* accel2Output = GetOutputInstance(accel2->allocated);

    if(accel1Output && accel2Output){
      InsertMapping(graphMapping,accel2Output,accel1Output);
    }
    
    // Map inputs
    FOREACH_LIST(FUInstance*,ptr1,accel1->allocated){
      FUInstance* instA = ptr1;
      if(instA->declaration != BasicDeclaration::input){
        continue;
      }

      FOREACH_LIST(FUInstance*,ptr2,accel2->allocated){
        FUInstance* instB = ptr2;
        if(instB->declaration != BasicDeclaration::input){
          continue;
        }

        if(instA->portIndex != instB->portIndex){
          continue;
        }

        InsertMapping(graphMapping,instB,instA);
      }
    }
  }break;
  case MergingStrategy::CONSOLIDATION_GRAPH:{
    ConsolidationGraphOptions options = {};
    options.mapNodes = false;
    options.specifics = specificNodes;

    graphMapping = ConsolidationGraphMapping(accel1,accel2,options,name,temp,out);
  }break;
  case MergingStrategy::FIRST_FIT:{
    graphMapping = FirstFitGraphMapping(accel1,accel2,out);
  }break;
  }

  return graphMapping;
}

MergeGraphResult MergeGraph(Accelerator* flatten1,Accelerator* flatten2,GraphMapping& graphMapping,String name,Arena* out){
  int size1 = Size(flatten1->allocated);
  int size2 = Size(flatten2->allocated);

  Arena* perm = globalPermanent;
  
  // This maps are returned
  Hashmap<FUInstance*,FUInstance*>* map1 = PushHashmap<FUInstance*,FUInstance*>(out,size1);
  Hashmap<FUInstance*,FUInstance*>* map2 = PushHashmap<FUInstance*,FUInstance*>(out,size2);

  BLOCK_REGION(out);

  Hashmap<FUInstance*,FUInstance*>* map = PushHashmap<FUInstance*,FUInstance*>(out,size1 + size2);

  Accelerator* newGraph = CreateAccelerator(name);

  // Create base instances from accel 1
  FOREACH_LIST(FUInstance*,ptr,flatten1->allocated){
    FUInstance* inst = ptr;

    FUInstance* newNode = CopyInstance(newGraph,ptr,inst->name);

    map->Insert(inst,newNode); // Maps from old instance in flatten1 to new inst in newGraph
    map1->Insert(newNode,ptr);
  }

  // Create base instances from accel 2, unless they are mapped to nodes from accel1
  FOREACH_LIST(FUInstance*,ptr,flatten2->allocated){
    FUInstance* inst = ptr;
    
    FUInstance** mapping = graphMapping.instanceMap->Get(inst); // Returns mapping from accel2 to accel1

    FUInstance* mappedNode = nullptr;
    if(mapping){
      mappedNode = map->GetOrFail(*mapping);

      // For now, change name even if they have the same name
      //if(!CompareString(inst->name,mappedNode->name)){
      String newName = PushString(perm,"%.*s_%.*s",UNPACK_SS(mappedNode->name),UNPACK_SS(inst->name));

      mappedNode->name = newName;
      //}
    } else {
      mappedNode = CopyInstance(newGraph,inst,inst->name);
      //mappedNode = (FUInstance*) CreateFUInstance(newGraph,inst->declaration,inst->name);
    }
    FUInstance* newNode = mappedNode;
    map->Insert(inst,mappedNode);
    map2->Insert(newNode,ptr);
  }

  // Create base edges from accel 1
  //FOREACH_LIST(Edge*,edge,flatten1->edges){
  EdgeIterator iter = IterateEdges(flatten1);
  while(iter.HasNext()){
    Edge edgeInst = iter.Next();
    Edge* edge = &edgeInst;

    FUInstance* mappedInst = map->GetOrFail(edge->units[0].inst);
    FUInstance* mappedOther = map->GetOrFail(edge->units[1].inst);

    ConnectUnitsGetEdge(mappedInst,edge->units[0].port,mappedOther,edge->units[1].port,edge->delay);
  }

  // Create base edges from accel 2, unless they are mapped to edges from accel 1
  //FOREACH_LIST(Edge*,edge,flatten2->edges){
  iter = IterateEdges(flatten2);
  while(iter.HasNext()){
    Edge edgeInst = iter.Next();
    Edge* edge = &edgeInst;

    Edge searchEdge = {};
    searchEdge.units[0] = edge->units[0];
    searchEdge.units[1] = edge->units[1];

    auto hasEdge = graphMapping.edgeMap->Get(searchEdge);

    if(hasEdge){
      FUInstance* mappedInst = map->GetOrFail(edge->units[0].inst);
      FUInstance* mappedOther = map->GetOrFail(edge->units[1].inst);

      Opt<Edge> oldEdge = FindEdge(mappedInst,edge->units[0].port,mappedOther,edge->units[1].port,edge->delay);

      if(oldEdge.has_value()){ // Edge between port instances might exist but different delay means we need to create another one
        continue;
      }
    }

    FUInstance* mappedInst = map->GetOrFail(edge->units[0].inst);
    FUInstance* mappedOther = map->GetOrFail(edge->units[1].inst);
    ConnectUnitsIfNotConnected(mappedInst,edge->units[0].port,mappedOther,edge->units[1].port,edge->delay);
  }

  MergeGraphResult res = {};
  res.newGraph = newGraph;
  res.map1 = map1;
  res.map2 = map2;
  res.accel1 = flatten1;
  res.accel2 = flatten2;

  return res;
}

MergeGraphResultExisting MergeGraphToExisting(Accelerator* existing,Accelerator* flatten2,GraphMapping& graphMapping,String name,Arena* out){
  Arena* perm = globalPermanent;

  int size1 = Size(existing->allocated);
  int size2 = Size(flatten2->allocated);

  // These maps are returned
  Hashmap<FUInstance*,FUInstance*>* map2 = PushHashmap<FUInstance*,FUInstance*>(out,size2);

  BLOCK_REGION(out);

  Hashmap<FUInstance*,FUInstance*>* map = PushHashmap<FUInstance*,FUInstance*>(out,size1 + size2);

  FOREACH_LIST(FUInstance*,ptr,existing->allocated){
    map->Insert(ptr,ptr);
  }

  // Create base instances from accel 2, unless they are mapped to nodes from accel1
  FOREACH_LIST(FUInstance*,ptr,flatten2->allocated){
    FUInstance* inst = ptr;

    FUInstance** mapping = graphMapping.instanceMap->Get(inst); // Returns mapping from accel2 to accel1

    FUInstance* mappedNode = nullptr;
    if(mapping){
      mappedNode = map->GetOrFail(*mapping);

      // For now, change name even if they have the same name
      // if(!CompareString(inst->name,mappedNode->name)){
      String newName = PushString(perm,"%.*s_%.*s",UNPACK_SS(mappedNode->name),UNPACK_SS(inst->name));

      mappedNode->name = newName;
      //}
    } else {
      mappedNode = CopyInstance(existing,inst,inst->name);
   }
    FUInstance* newNode = mappedNode;
    map->Insert(inst,mappedNode);
    map2->Insert(ptr,newNode);
  }

  EdgeIterator iter = IterateEdges(flatten2);
  while(iter.HasNext()){
    Edge edgeInst = iter.Next();
    Edge* edge = &edgeInst;
    Edge searchEdge = {};
    searchEdge.units[0] = edge->units[0];
    searchEdge.units[1] = edge->units[1];

    auto hasEdge = graphMapping.edgeMap->Get(searchEdge);

    if(hasEdge){
      FUInstance* mappedInst = map->GetOrFail(edge->units[0].inst);
      FUInstance* mappedOther = map->GetOrFail(edge->units[1].inst);

      Opt<Edge> oldEdge = FindEdge(mappedInst,edge->units[0].port,mappedOther,edge->units[1].port,edge->delay);

      if(oldEdge.has_value()){ // Edge between port instances might exist but different delay means we need to create another one
        continue;
      }
    }

    FUInstance* mappedInst = map->GetOrFail(edge->units[0].inst);
    FUInstance* mappedOther = map->GetOrFail(edge->units[1].inst);
    ConnectUnitsIfNotConnected(mappedInst,edge->units[0].port,mappedOther,edge->units[1].port,edge->delay);
  }

  MergeGraphResultExisting res = {};
  res.result = existing;
  res.map2 = map2;
  res.accel2 = flatten2;

  return res;
}

Array<int> GetPortConnections(FUInstance* node,Arena* arena){
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

Opt<int> GetConfigurationIndexFromInstanceNode(FUDeclaration* type,FUInstance* node){
  // While configuration array is fully defined, no need to do this check beforehand.
  int index = 0;
  FOREACH_LIST_INDEXED(FUInstance*,iter,type->fixedDelayCircuit->allocated,index){
    if(iter == node){
      return type->baseConfig.configOffsets.offsets[index];
    }
  }

  Assert(false);
  return Opt<int>{};
}

FUInstance* GetNodeByName(Accelerator* accel,String name){
  FOREACH_LIST(FUInstance*,ptr,accel->allocated){
    if(CompareString(ptr->name,name)){
      return ptr;
    }
  }

  return nullptr;
}

FUDeclaration* muxTypeGlobal = nullptr;
bool GetPathRecursive(PortInstance searching,Set<PortInstance>* mergedMultiplexers,PortInstance nodeToCheck,PortInstance parentNode,DynamicArray<Edge>& array){
  FUInstance* n = nodeToCheck.inst;

  if(nodeToCheck == searching){
    return true;
  } else if(nodeToCheck.inst->declaration != BasicDeclaration::fixedBuffer &&
            nodeToCheck.inst->declaration != BasicDeclaration::buffer &&
            nodeToCheck.inst->declaration != muxTypeGlobal){
    return false;
  } else {
    for(int port = 0; port < n->inputs.size; port++){
      PortInstance out = n->inputs[port];
      if(out.inst == nullptr) continue;

      if(n->declaration == muxTypeGlobal){
        PortInstance muxSide = {.inst = n,.port = port};
      
        if(!mergedMultiplexers->Exists(muxSide)){
          continue;
        }
      }
      
      if(GetPathRecursive(searching,mergedMultiplexers,out,nodeToCheck,array)){
        Edge edge = {};
        edge.in = {.inst = nodeToCheck.inst,.port = port};
        edge.out = out;
        *array.PushElem() = edge;
        return true;
      }
    }
    return false;
  }
}

typedef Array<Edge> Path;

void PrintPath(Path p){
  for(Edge e : p){
    printf("%.*s:%d->%.*s:%d\n",UNPACK_SS(e.out.inst->name),e.out.port,UNPACK_SS(e.in.inst->name),e.in.port);
  }
}

Array<Path> GetAllPaths(Accelerator* accel,PortInstance start,PortInstance end,Arena* out,Arena* temp){
  BLOCK_REGION(temp);

  Assert(!(start == end));
  
  if(end.inst->type == NodeType_SOURCE_AND_SINK){
    return {};
  }

  ArenaList<Path>* allPaths = PushArenaList<Path>(temp);
  PortInstance outInst = end.inst->inputs[end.port];
  if(!outInst.inst){
    return {};
  }
    
  Edge edge = {.out = outInst,.in = end};
    
  if(outInst == start){
    Array<Edge> edges = PushArray<Edge>(out,1);
    edges[0] = edge;
    Array<Path> path = PushArray<Path>(out,1);
    path[0] = edges;
    return path;
  }
    
  for(int outPort = 0; outPort < outInst.inst->inputs.size; outPort++){
    PortInstance outInputSide = {.inst = outInst.inst,.port = outPort};
      
    Array<Path> subPaths = GetAllPaths(accel,start,outInputSide,temp,out);

    for(Path p : subPaths){
      Array<Edge> edges = PushArray<Edge>(out,p.size + 1);

      for(int i = 0; i < p.size; i++){
        edges[i] = p[i];
      }
      edges[p.size] = edge;

      *PushListElement(allPaths) = edges;
    }
  }

  return PushArrayFromList(out,allPaths);
}

Array<Edge> GetPath(Accelerator* accel,Set<PortInstance>* mergedMultiplexers,PortInstance start,PortInstance end,Arena* out){
  Assert(!(start == end));

  muxTypeGlobal = GetTypeByName(STRING("CombMux8")); // TODO: Kind of an hack

  DynamicArray<Edge> arr = StartArray<Edge>(out);
  
  for(int port = 0; port < end.inst->inputs.size; port++){
    PortInstance out = end.inst->inputs[port];

    if(end.inst->declaration == muxTypeGlobal){
      PortInstance muxSide = {.inst = end.inst,.port = port};
      
      if(!mergedMultiplexers->Exists(muxSide)){
        continue;
      }
    }
    
    if(GetPathRecursive(start,mergedMultiplexers,out,end,arr)){
      Edge edge = {};
      edge.in = end;
      edge.out = out;
      *arr.PushElem() = edge;
      break;
    }
  }

  Array<Edge> result = EndArray(arr);

  // Fix delays
  for(Edge& node : result){
    Opt<Edge> edge = FindEdge(node.out,node.in);

    if(edge.has_value()){
      node.delay = edge.value().delay;
   }
  }

  return result;
}

void MappingCheck(InstanceMap* map){
  int idToCheckFirst = -1;
  int idToCheckSecond = -1;
  for(Pair<FUInstance*,FUInstance**> p : map){
    FUInstance* f = p.first;
    FUInstance* s = *p.second;

    if(idToCheckFirst < 0){
      idToCheckFirst = f->accel->id;
      idToCheckSecond = s->accel->id;
    } else {
      Assert(idToCheckFirst == f->accel->id);
      Assert(idToCheckSecond == s->accel->id);
    }
  }
}

Pair<int,int> MappingGetId(InstanceMap* map){
  MappingCheck(map);

  for(Pair<FUInstance*,FUInstance**> p : map){
    FUInstance* f = p.first;
    FUInstance* s = *p.second;

    return {f->accel->id,s->accel->id};
  }

  NOT_POSSIBLE("For now do not consider empty mappings");
  return {-1,-1};
}

InstanceMap* MappingReverse(InstanceMap* toReverse,Arena* out){
  MappingCheck(toReverse);

  InstanceMap* result = PushInstanceMap(out,toReverse->nodesUsed);

  for(Pair<FUInstance*,FUInstance**> p : toReverse){
    result->Insert(*p.second,p.first);
  }

  return result;
}

InstanceMap* MappingCombine(InstanceMap* first,InstanceMap* second,Arena* out){
  MappingCheck(first);
  MappingCheck(second);

  InstanceMap* result = PushInstanceMap(out,first->nodesUsed);

  for(Pair<FUInstance*,FUInstance**> p : first){
    FUInstance* start = p.first;
    FUInstance** end = second->Get(*p.second);

    if(end){
      result->Insert(start,*end);
    }
  }

  return result;
}

struct ReconstituteResult{
  Accelerator* accel;
  InstanceMap* accelToRecon;
  InstanceMap* reconToAccel;
};

ReconstituteResult ReconstituteGraph(Accelerator* merged,Set<PortInstance>* mergedMultiplexers,Accelerator* base,String name,InstanceMap* baseToMerged,Arena* out,Arena* temp){
  BLOCK_REGION(temp);
  Accelerator* accel = CreateAccelerator(name);

  MappingCheck(baseToMerged);
  InstanceMap* mergedToBase = MappingReverse(baseToMerged,temp);

  for(PortInstance portInst : mergedMultiplexers){
    LOCATION();
    Assert(portInst.inst->accel->id == merged->id);
    break;
  }
  
  auto mappingId = MappingGetId(mergedToBase);
  Assert(mappingId.first == merged->id);
  Assert(mappingId.second == base->id);
  
  int size = Size(merged->allocated);
  InstanceMap* accelToRecon = PushHashmap<FUInstance*,FUInstance*>(out,size);
  InstanceMap* reconToAccel = PushHashmap<FUInstance*,FUInstance*>(out,size);

  Hashmap<FUInstance*,FUInstance*>* instancesAdded = PushHashmap<FUInstance*,FUInstance*>(temp,9999);

  FUDeclaration* muxType = GetTypeByName(STRING("CombMux8")); // TODO: Kind of an hack

  static int index = 0;
  EdgeIterator iter = IterateEdges(base);
  while(iter.HasNext()){
    Edge edgeInst = iter.Next();
    Edge* edge = &edgeInst;

    FUInstance* start = baseToMerged->GetOrFail(edge->units[0].inst);
    FUInstance* end = baseToMerged->GetOrFail(edge->units[1].inst);

    PortInstance startPort = {.inst = start,.port = edge->units[0].port};
    PortInstance endPort = {.inst = end,.port = edge->units[1].port};

    Array<Path> allPaths = GetAllPaths(merged,startPort,endPort,temp,out);
#if 0
    printf("All paths: %d\n",allPaths.size);
    for(Path p : allPaths){
      PrintPath(p);

      for(Edge edge : p){
        if(mergedMultiplexers->Exists(edge.in)){
          printf("%.*s %d %d IS\n",UNPACK_SS(edge.in.inst->name),edge.in.inst->id,edge.in.inst->accel->id);
        } else {
          printf("%.*s %d %d NOT\n",UNPACK_SS(edge.in.inst->name),edge.in.inst->id,edge.in.inst->accel->id);
        }
      }
      
      printf("\n");
    }
#endif
    
    Array<Edge>* pathSelected = nullptr;

    for(Path p : allPaths){
      bool valid = true;
      for(Edge edge : p){
        if(edge.in.inst->declaration == muxType){
          if(!mergedMultiplexers->Exists(edge.in)){
            valid = false;
            break;
          }
        }
      }

      if(valid){
        Assert(pathSelected == nullptr);
        pathSelected = &p;
      }
    }
    
#if 0
    Array<Edge> pathOnMerged = GetPath(merged,mergedMultiplexers,startPort,endPort,temp);
    Assert(pathOnMerged.size > 0);

    printf("Path selected:\n");
    PrintPath(pathOnMerged);
    for(Edge edge : pathOnMerged){
      if(mergedMultiplexers->Exists(edge.in)){
        printf("%.*s %d %d IS\n",UNPACK_SS(edge.in.inst->name),edge.in.inst->id,edge.in.inst->accel->id);
      } else {
        printf("%.*s %d %d NOT\n",UNPACK_SS(edge.in.inst->name),edge.in.inst->id,edge.in.inst->accel->id);
      }
    }
#endif
    
    // Replicate exact same path on new accelerator.
    for(Edge& edgeOnMerged : *pathSelected){
      FUInstance* node0 = edgeOnMerged.units[0].inst;
      FUInstance* node1 = edgeOnMerged.units[1].inst;
      GetOrAllocateResult<FUInstance*> res0 = instancesAdded->GetOrAllocate(node0);
      GetOrAllocateResult<FUInstance*> res1 = instancesAdded->GetOrAllocate(node1);
      
      if(!res0.alreadyExisted){
        FUInstance** optOriginal = mergedToBase->Get(node0);

        String name = node0->name;
        if(optOriginal){
          name = (*optOriginal)->name;
        }
        
        *res0.data = CopyInstance(accel,node0,name);
        (*res0.data)->disabledMergeMultiplexer = node0->disabledMergeMultiplexer;
        
        accelToRecon->Insert(node0,*res0.data);
        reconToAccel->Insert(*res0.data,node0);
      }
      
      if(!res1.alreadyExisted){
        FUInstance** optOriginal = mergedToBase->Get(node1);

        String name = node1->name;
        if(optOriginal){
          name = (*optOriginal)->name;
        }

        *res1.data = CopyInstance(accel,node1,name);
        (*res1.data)->disabledMergeMultiplexer = node1->disabledMergeMultiplexer;

        accelToRecon->Insert(node1,*res1.data);
        reconToAccel->Insert(*res1.data,node1);
      }

      //printf("%.*s %.*s\n",UNPACK_SS((*res0.data)->name),UNPACK_SS((*res1.data)->name));
      
      ConnectUnits((PortInstance){*res0.data,edgeOnMerged.units[0].port},{*res1.data,edgeOnMerged.units[1].port},edgeOnMerged.delay);
    }
  }

  ReconstituteResult result = {};
  result.accel = accel;
  result.accelToRecon = accelToRecon;
  result.reconToAccel = reconToAccel;
  
  return result;
}

FUInstance* GetInstanceById(Accelerator* accel,int id){
  FOREACH_LIST(FUInstance*,inst,accel->allocated){
    if(inst->id == id){
      return inst;
    }
  }

  return nullptr;
}

ReconstituteResult ReconstituteGraphFromAnother(Accelerator* accel,Accelerator* base,Arena* out,Arena* temp){
  int baseNodes = Size(base->allocated);
  InstanceMap* baseNodeToAccel = PushInstanceMap(temp,baseNodes);
  InstanceMap* accelNodeToBase = PushInstanceMap(temp,baseNodes);
                                                                  
  FOREACH_LIST(FUInstance*,inst,base->allocated){
    int id = inst->id;

    FUInstance* other = GetInstanceById(accel,id);
    Assert(other);

    baseNodeToAccel->Insert(inst,other);
    accelNodeToBase->Insert(other,inst);
  }

#if 0  
  region(temp){
    String filepath = PushDebugPath(temp,accel->name,STRING("reconTest.dot"));
    OutputGraphDotFile(res.result,false,filepath,temp);
  }
#endif

  return {};
}

struct GraphAndMapping{
  FUDeclaration* decl;
  InstanceMap* map;
  Set<PortInstance>* mergeMultiplexers;
};

template<> class std::hash<GraphAndMapping>{
public:
   std::size_t operator()(GraphAndMapping const& s) const noexcept{
     std::size_t res = (std::size_t) s.decl;
     return res;
   }
};

bool operator==(const GraphAndMapping& g0,const GraphAndMapping& g1){
  return (g0.decl == g1.decl);
}

// Recurses over composite but does not recurse lower than merged units.
Array<GraphAndMapping> GetAllBaseMergeTypes(FUDeclaration* decl,Arena* out,Arena* temp){
  BLOCK_REGION(temp);
  Set<GraphAndMapping>* seen = PushSet<GraphAndMapping>(temp,10);

  if(decl->type == FUDeclarationType_MERGED){
    for(ConfigurationInfo info : decl->configInfo){
      seen->Insert({.decl = info.baseType,.map = info.mapping,.mergeMultiplexers = info.mergeMultiplexers});
    }
  } else {
    FOREACH_LIST(FUInstance*,inst,decl->fixedDelayCircuit->allocated){
      FUDeclaration* subDecl = inst->declaration;

      if(subDecl->type == FUDeclarationType_MERGED){
        for(ConfigurationInfo info : subDecl->configInfo){
          seen->Insert({.decl = info.baseType,.map = info.mapping,.mergeMultiplexers = info.mergeMultiplexers});
        }
      } else if(subDecl->type == FUDeclarationType_COMPOSITE ||
                subDecl->type == FUDeclarationType_ITERATIVE){
        Array<GraphAndMapping> subTypes = GetAllBaseMergeTypes(subDecl,temp,out);
        for(GraphAndMapping result : subTypes){
          seen->Insert(result);
        }
      }
    }
  }

  return PushArrayFromSet(out,seen);
}

FUDeclaration* Merge(Array<FUDeclaration*> types,
                     String name,Array<SpecificMergeNode> specifics,
                     Arena* temp,Arena* temp2,MergingStrategy strat){
  Assert(types.size >= 2);

  strat = MergingStrategy::CONSOLIDATION_GRAPH;
  
  Arena* perm = globalPermanent;
  BLOCK_REGION(temp);
  BLOCK_REGION(temp2);
  
  int size = types.size;
  Array<Accelerator*> flatten = PushArray<Accelerator*>(temp,size);
  Array<InstanceMap*> flattenToMerge = PushArray<InstanceMap*>(temp,size);
  Array<InstanceMap*> mergeToFlatten = PushArray<InstanceMap*>(temp,size); 
  Array<GraphMapping> mappings = PushArray<GraphMapping>(temp,size - 1);
  
  int mergedAmount = 1;
  for(FUDeclaration* decl : types){
    mergedAmount *= decl->configInfo.size;
  }
  mergedAmount = std::max(mergedAmount,size);

#if 0
  for(int i = 0; i < size; i++){
    flatten[i] = Flatten(types[i]->baseCircuit,99,temp);
  }
#endif
  for(int i = 0; i < size; i++){
    flatten[i] = types[i]->flattenedBaseCircuit;
  }
  
  for(int i = 0; i < size - 1; i++){
    mappings[i] = InitGraphMapping(temp);
  }
    
  // We copy accel 1.
  Accelerator* result = CopyAccelerator(flatten[0],nullptr);
  
  Array<Accelerator*> orderedAccelerators = [&](){
    DynamicArray<Accelerator*> arr = StartArray<Accelerator*>(temp);
    bool first = true;
    for(Accelerator* accel : flatten){
      if(first){
        *arr.PushElem() = result;
      } else {
        *arr.PushElem() = accel;
      }
      first = false;
    }
    return EndArray(arr);
  }();
                                            
  Array<SpecificMergeNodes> specificNodes = [&](){
    auto arr = StartArray<SpecificMergeNodes>(temp); // For now, only use specifics for two graphs.
    for(SpecificMergeNode node : specifics){
      // For now this only works for two graphs
      FUInstance* left = GetNodeByName(orderedAccelerators[node.firstIndex],node.firstName);
      FUInstance* right = GetNodeByName(orderedAccelerators[node.secondIndex],node.secondName);
      *arr.PushElem() = {left,right};
    }
    return EndArray(arr);
  }();

  InstanceMap* initialMap = PushHashmap<FUInstance*,FUInstance*>(temp,Size(result->allocated));

  FUInstance* ptr1 = result->allocated;
  FUInstance* ptr2 = flatten[0]->allocated;

  for(; ptr1 != nullptr && ptr2 != nullptr; ptr1 = ptr1->next,ptr2 = ptr2->next){
    initialMap->Insert(ptr2,ptr1);
  }
  Assert(ptr1 == nullptr && ptr2 == nullptr);
  flattenToMerge[0] = initialMap; // Maps from flattened 0 into copied accelerator. 

  for(int i = 1; i < size; i++){
    String folderName = PushString(temp,"%.*s/Merge%d",UNPACK_SS(name),i);

    mappings[i-1] = MergeAccelerator(result,flatten[i],specificNodes,strat,folderName,temp,temp2);
    MergeGraphResultExisting res = MergeGraphToExisting(result,flatten[i],mappings[i-1],folderName,temp);

    OutputDebugDotGraph(res.result,STRING(StaticFormat("result%d.dot",i)),temp);

    flattenToMerge[i] = res.map2;
    result = res.result;
  }
  
  // TODO: This might indicate the need to implement a bidirectional map.
  for(int i = 0; i < size; i++){
    mergeToFlatten[i] = MappingReverse(flattenToMerge[i],temp);
  }

  // Need to map from final to each input graph
  int numberOfMultipleInputs = 0;
  FOREACH_LIST(FUInstance*,ptr,result->allocated){
    if(ptr->multipleSamePortInputs){
      numberOfMultipleInputs += 1;
    }
  }

  Array<InstanceMap*> mapMultipleInputsToSubgraphs = PushArray<InstanceMap*>(temp,size);

  for(int i = 0; i < size; i++){
    mapMultipleInputsToSubgraphs[i] = PushHashmap<FUInstance*,FUInstance*>(temp,numberOfMultipleInputs);

    for(Pair<FUInstance*,FUInstance**> pair : flattenToMerge[i]){
      if((*pair.second)->multipleSamePortInputs){
        mapMultipleInputsToSubgraphs[i]->Insert(*pair.second,pair.first);
      }
    }
  }

  // Do not consider these for purposes of finding path
  for(int i = 0; i < size; i++){
    FOREACH_LIST(FUInstance*,node,result->allocated){
      node->disabledMergeMultiplexer = true;
    }
  }

  Array<Set<PortInstance>*> mergeMultiplexers = PushArray<Set<PortInstance>*>(perm,size);

  for(Set<PortInstance>*& s : mergeMultiplexers){
    s = PushSet<PortInstance>(temp,99); // TODO: Correct size or change to TrieSet. NOTE: Temp because later on the function we replace this maps from fixedCircuit to flattened circuit
  }

  FUDeclaration* muxType = GetTypeByName(STRING("CombMux8"));

  EdgeIterator iter = IterateEdges(result);
  while(iter.HasNext()){
    Edge edge = iter.Next();

    if(edge.in.inst->declaration == muxType){
      for(Set<PortInstance>* s : mergeMultiplexers){
        s->Insert(edge.in);
      }
    }
  }
  
  int multiplexersAdded = 0;
  FOREACH_LIST(FUInstance*,ptr,result->allocated){
    if(ptr->multipleSamePortInputs){
      // Need to figure out which port, (which might be multiple), has the same inputs
      Array<int> portConnections = GetPortConnections(ptr,temp);

      for(int port = 0; port < portConnections.size; port++){
        if(portConnections[port] < 2){
          continue;
        }

        int numberOfMultiple = portConnections[port];

        // Collect the problematic edges for this port
        Array<Edge> problematicEdgesInFinalGraph = PushArray<Edge>(temp,numberOfMultiple);
        int index = 0;

        FOREACH_LIST(ConnectionNode*,con,ptr->allInputs){
          if(con->port == port){
            Edge node = {};
            node.units[0] = con->instConnectedTo;
            node.units[1] = {ptr,con->port};
            node.delay = con->edgeDelay;
            problematicEdgesInFinalGraph[index++] = node;
          }
        }

        // Need to map edgeNode into the integer that indicates which input graph it belongs to.
        Array<int> inputNumberToEdgeIndex = PushArray<int>(temp,size);
        Memset(inputNumberToEdgeIndex,0);

        for(int i = 0; i < problematicEdgesInFinalGraph.size; i++){
          Edge edgeNode = problematicEdgesInFinalGraph[i];

          for(int ii = 0; ii < size; ii++){
            FUInstance** possibleNode = mapMultipleInputsToSubgraphs[ii]->Get(edgeNode.units[1].inst);

            if(possibleNode){
              FUInstance* node = *possibleNode;

              // We find the other node that matches what we expected. If we add a general map from finalResult to input graph
              // Kinda like we have with the mapMultipleInputsToSubgraphs, except that we can map all the nodes instead of only the multiple inputs
              // Then we could use the map here, instead of searching the all inputs of the node.
              // TODO: The correct approach would be to use a Bimap to perform the mapping from final graph to inputs, and to not use the mapMultipleInputsToSubgraphs (because this is just an incomplete map, if we add a complete map we could do the same thing)
              FOREACH_LIST(ConnectionNode*,con,node->allInputs){
                FUInstance* other = flattenToMerge[ii]->GetOrFail(con->instConnectedTo.inst);

                // In here, we could save edge information for the view graphs, so that we could add a multiplexer to the view graph later
                // Do not know yet if needed.
                if(con->port == port && other == edgeNode.units[0].inst && con->instConnectedTo.port == edgeNode.units[0].port && con->edgeDelay == edgeNode.delay){
                  inputNumberToEdgeIndex[ii] = i;
                  break;
                }
              }
            }
          }
        }

        // At this point, we know the edges that we need to remove and to replace with a multiplexer and the
        // port to which they should connect.
        // Create multiplexer instance
        const char* format = "versat_merge_mux_%d";
        String str = PushString(perm,format,multiplexersAdded++);
        PushNullByte(perm);
        FUInstance* multiplexer = CreateFUInstance(result,muxType,str);
        multiplexer->isMergeMultiplexer = true;
        ShareInstanceConfig(multiplexer,99); // TODO: Realistic share index. Maybe add a function that allocates a free share index and we keep the next free shared index inside the accelerator.

        for(int i = 0; i < problematicEdgesInFinalGraph.size; i++){
          Edge node = problematicEdgesInFinalGraph[i];

          for(int ii = 0; ii < inputNumberToEdgeIndex.size; ii++){
            int edgeIndex = inputNumberToEdgeIndex[ii];
            if(edgeIndex != i){
              continue;
            }

            int graphIndex = ii;

            PortInstance portInst = {.inst = multiplexer,.port = graphIndex};
            mergeMultiplexers[graphIndex]->Insert(portInst);
            
            RemoveConnection(result,node.units[0].inst,node.units[0].port,node.units[1].inst,node.units[1].port);
            ConnectUnits(node.units[0],{multiplexer,graphIndex},node.delay);
          }
        }

        ConnectUnits((PortInstance){multiplexer,0},{ptr,port},0);
      }
    }
  }

  if(globalDebug.outputGraphs){
    for(int i = 0; i < size; i++){
      BLOCK_REGION(temp);
      Set<FUInstance*>* firstGraph = PushSet<FUInstance*>(temp,Size(flatten[i]->allocated));
      FOREACH_LIST(FUInstance*,ptr,flatten[i]->allocated){
        FUInstance* finalNode = flattenToMerge[i]->GetOrFail(ptr);

        firstGraph->Insert(finalNode);
      }

      OutputDebugDotGraph(result,STRING(StaticFormat("finalMerged_%d.dot",i)),firstGraph,temp);
    }
  }
  
  String permanentName = PushString(perm,name);
  result->name = permanentName;

  FUDeclaration declInst = {};
  declInst.name = name;

  Arena* permanent = perm;
  Accelerator* circuit = result;

  InstanceMap* circuitToCopy = PushInstanceMap(perm,Size(circuit->allocated));
  
  declInst.baseCircuit = CopyAccelerator(circuit,circuitToCopy);
  declInst.flattenedBaseCircuit = declInst.baseCircuit; // Should already be flattened.
  
  // Output full circuit
  OutputDebugDotGraph(declInst.baseCircuit,STRING("FullCircuit.dot"),temp);
#if 0
  region(temp){

    GraphPrintingContent res = GenerateDefaultPrintingContent(declInst.baseCircuit,temp,temp2);

    String content = GenerateDotGraph(circuit,res,temp,temp2);

    String filepath = PushDebugPath(temp,name,STRING("FullCircuit.dot"));
    OutputContentToFile(filepath,content);
    }
#endif

  int actualMergedAmount = mergedAmount;
  mergedAmount = size; // TODO: FOR NOW

  int buffersInserted = 0;
  Array<Accelerator*> recon = PushArray<Accelerator*>(temp,mergedAmount);
  Array<InstanceMap*> reconToAccel = PushArray<InstanceMap*>(temp,mergedAmount);
  Array<InstanceMap*> accelToRecon = PushArray<InstanceMap*>(temp,mergedAmount);
  Array<DAGOrderNodes> reconOrder = PushArray<DAGOrderNodes>(temp,mergedAmount);
  Array<CalculateDelayResult> reconDelay = PushArray<CalculateDelayResult>(temp,mergedAmount);

  Array<Array<int>> bufferValues = PushArray<Array<int>>(perm,size);
  for(Array<int>& b : bufferValues){
    b = PushArray<int>(perm,999);
    Memset(b,0);
  }

  auto repr = [](FUInstance* node,Arena* out) -> GraphInfo{
    GraphInfo info = defaultNodeContent(node,out);
    if(node->disabledMergeMultiplexer){
      return {info.content,Color_RED};
    } else {
      return defaultNodeContent(node,out);
    }
  };

  // "Merged circuit"
  // Multiplexers set on the merged circuit base
  // Base circuit
  // Name
  // Base to merged circuit map.
  // Arenas.

  // TODO: Because we are also keeping the middle graphs in memory, we do not use the actualMergedAmount, but instead a value that is slightly higher (but should not be more than half).
  actualMergedAmount = 99;
  {
    Array<Accelerator*> mergedAccelerators = PushArray<Accelerator*>(temp,actualMergedAmount);
    Array<Set<PortInstance>*> mergedMultiplexers = PushArray<Set<PortInstance>*>(temp,actualMergedAmount);
    Array<Accelerator*> baseCircuits = PushArray<Accelerator*>(temp,actualMergedAmount);
    Array<FUDeclaration*> baseCircuitType = PushArray<FUDeclaration*>(temp,actualMergedAmount);
    Array<InstanceMap*> maps = PushArray<InstanceMap*>(temp,actualMergedAmount);
    Array<bool> lowestLevel = PushArray<bool>(temp,actualMergedAmount);
    
    Memset(lowestLevel,false);
    // Init first iteration
    for(int i = 0; i < size; i++){
      mergedAccelerators[i] = circuit;
      mergedMultiplexers[i] = mergeMultiplexers[i];
      baseCircuits[i] = flatten[i];
      baseCircuitType[i] = types[i];
      maps[i] = flattenToMerge[i];
    }

    bool didRecurse = false;
    int start = 0;
    int toProcess = size;
    int toWrite = size;

    while(start != toProcess){
      for(int i = start; i < toProcess; i++){
        ReconstituteResult result = ReconstituteGraph(mergedAccelerators[i],mergedMultiplexers[i],baseCircuits[i],permanentName,maps[i],temp,temp2);

        Array<GraphAndMapping> subMerged = GetAllBaseMergeTypes(baseCircuitType[i],temp,temp2);

        if(subMerged.size == 0){
          lowestLevel[i] = true;
        }

        // TODO: Left here. If everything is working correctly, we should have the lower recons working fine. Still probably need to add an array to save recon to circuit mappings because we do need them later on.
        
        printf("%d %d %d %d\n",start,i,toProcess,lowestLevel[i] ? 1 : 0);
        
        InstanceMap* flattenToRecon = MappingCombine(maps[i],result.accelToRecon,temp);
        
        for(int ii = 0; ii < subMerged.size; ii++){
          didRecurse = true;
          GraphAndMapping subDecl = subMerged[ii];
          Accelerator* mergedAccel = subDecl.decl->flattenedBaseCircuit;

          InstanceMap* baseToMergedAccel = subDecl.map;
          InstanceMap* mergedAccelToBase = MappingReverse(baseToMergedAccel,temp);

          InstanceMap* combined = MappingCombine(baseToMergedAccel,flattenToRecon,temp);
          InstanceMap* reversed = MappingReverse(combined,temp);

          Set<PortInstance>* trueMergeMultiplexers = PushSet<PortInstance>(temp,99);

          for(PortInstance p : subDecl.mergeMultiplexers){
            trueMergeMultiplexers->Insert({flattenToRecon->GetOrFail(p.inst),p.port});
          }

          MappingCheck(flattenToRecon);
        
          for(PortInstance p : mergeMultiplexers[i]){
            trueMergeMultiplexers->Insert({result.accelToRecon->GetOrFail(p.inst),p.port});
          }
          
          mergedAccelerators[toWrite] = result.accel;
          mergedMultiplexers[toWrite] = trueMergeMultiplexers;
          baseCircuits[toWrite] = mergedAccel;
          baseCircuitType[toWrite] = subDecl.decl;
          maps[toWrite] = combined;
          
          toWrite += 1;
        }
      }

      start = toProcess;
      toProcess = toWrite;
    }

    if(didRecurse){
      exit(0);
    }
  }
    
#if 0
  ReconstituteResult result = ReconstituteGraph(circuit,mergeMultiplexers[i],flatten[i],permanentName,flattenToMerge[i],temp,temp2);

  ReconstituteResult subResult = ReconstituteGraph(result.accel,trueMergeMultiplexers,mergedAccel,STRING("Test"),combined,temp,temp2);
#endif
  
  int globalIndex = 0;
  int outerIndex = 0;
  bool didRecurse = false;
  while(true){
    int index = 0;
    for(int i = 0; i < size; i++){
      ReconstituteResult result = ReconstituteGraph(circuit,mergeMultiplexers[i],flatten[i],permanentName,flattenToMerge[i],temp,temp2);
      
#if 0
      printf("A:%d %d\n",circuit->id,flatten[i]->id);
    
      {
        String fileName = PushString(temp,"AccelRecon%d.dot",i);
        String path = PushDebugPath(temp,permanentName,fileName);
      
        GraphPrintingContent res = GeneratePrintingContent(result.accel,repr,defaultEdgeContent,temp,temp2);
        String content = GenerateDotGraph(result.accel,res,temp,temp2);
        OutputContentToFile(path,content);
      }
#endif
      
#if 1
      Array<GraphAndMapping> subMerged = GetAllBaseMergeTypes(types[i],temp,temp2);

#if 0
      NEWLINE();
      printf("Circuit: %d\n",circuit->id);
      printf("Flatten: %d\n",flatten[i]->id);
      printf("Result: %d\n",result.accel->id);
      
      auto a = MappingGetId(result.accelToRecon);
      printf("a:%d->%d\n",a.first,a.second);

      auto b = MappingGetId(flattenToMerge[i]);
      printf("b:%d->%d\n",b.first,b.second);
#endif      

      InstanceMap* flattenToRecon = MappingCombine(flattenToMerge[i],result.accelToRecon,temp);
      
      for(int ii = 0; ii < subMerged.size; ii++){
        GraphAndMapping subDecl = subMerged[ii];
        Accelerator* mergedAccel = subDecl.decl->flattenedBaseCircuit;

        InstanceMap* baseToMergedAccel = subDecl.map;
        InstanceMap* mergedAccelToBase = MappingReverse(baseToMergedAccel,temp);

#if 0
        auto f = MappingGetId(baseToMergedAccel);
        printf("SubMerged: %d\n",mergedAccel->id);
        printf("f:%d->%d\n",f.first,f.second);
#endif
        
        InstanceMap* combined = MappingCombine(baseToMergedAccel,flattenToRecon,temp);
        InstanceMap* reversed = MappingReverse(combined,temp);

        Set<PortInstance>* trueMergeMultiplexers = PushSet<PortInstance>(temp,99);

        for(PortInstance p : subDecl.mergeMultiplexers){
          trueMergeMultiplexers->Insert({flattenToRecon->GetOrFail(p.inst),p.port});
        }

        MappingCheck(flattenToRecon);
        
        for(PortInstance p : mergeMultiplexers[i]){
          trueMergeMultiplexers->Insert({result.accelToRecon->GetOrFail(p.inst),p.port});
        }

        for(PortInstance p : trueMergeMultiplexers){
          printf("%.*s:%d %d %d\n",UNPACK_SS(p.inst->name),p.port,p.inst->id,p.inst->accel->id);
        }

#if 1
        didRecurse = true;
        NEWLINE();
        printf("Gonna recon: %.*s\n",UNPACK_SS(mergedAccel->name));
        ReconstituteResult subResult = ReconstituteGraph(result.accel,trueMergeMultiplexers,mergedAccel,STRING("Test"),combined,temp,temp2);

        printf("Gonna print graph: %d\n",globalIndex);
        {
          String fileName = PushString(temp,"Test%d.dot",globalIndex++);
          String path = PushDebugPath(temp,permanentName,fileName);
      
          GraphPrintingContent res = GeneratePrintingContent(subResult.accel,repr,defaultEdgeContent,temp,temp2);
          String content = GenerateDotGraph(subResult.accel,res,temp,temp2);
          OutputContentToFile(path,content);
        }
        NEWLINE();
#endif
      }
#endif
      
      recon[index] = result.accel;
      accelToRecon[index] = result.accelToRecon;
      reconToAccel[index] = result.reconToAccel;

#if 1
      String fileName = PushString(temp,"accel%dRecon.dot",i);
      String path = PushDebugPath(temp,permanentName,fileName);
      
      GraphPrintingContent res = GeneratePrintingContent(recon[index],repr,defaultEdgeContent,temp,temp2);
      String content = GenerateDotGraph(recon[index],res,temp,temp2);
      OutputContentToFile(path,content);
#endif
      
      index += 1;
    }

    outerIndex += 1;
    
    Hashmap<String,NodeType>* nodeTypes = PushHashmap<String,NodeType>(temp,999);

    bool delayInserted = false;
    for(int i = 0; i < recon.size; i++){
      Accelerator* accel = recon[i];
      reconOrder[i] = CalculateDAGOrder(accel->allocated,temp);
      reconDelay[i] = CalculateDelay(accel,reconOrder[i],temp);

      Array<DelayToAdd> delaysToAdd = GenerateFixDelays(accel,reconDelay[i].edgesDelay,perm,temp);
      
      for(DelayToAdd toAdd : delaysToAdd){
        Edge reconEdge = toAdd.edge;

        FUInstance* n0 = reconToAccel[i]->GetOrFail(reconEdge.units[0].inst);
        FUInstance* n1 = reconToAccel[i]->GetOrFail(reconEdge.units[1].inst);

        String uniqueName = PushString(perm,"%.*s_%d_%d",UNPACK_SS(toAdd.bufferName),i,outerIndex);
        FUInstance* buffer = CreateFUInstance(circuit,BasicDeclaration::buffer,uniqueName);
        SetStatic(circuit,buffer);
        buffersInserted += 1;
       
        InsertUnit(circuit,PortInstance{n0,reconEdge.units[0].port},PortInstance{n1,reconEdge.units[1].port},PortInstance{buffer,0});
      }

      if(delaysToAdd.size > 0){
        delayInserted = true;
        break;
      }
    }

    if(!delayInserted){
      break;
    }
  }

  if(didRecurse){
    exit(0);
  }
  
  // Corrects mapping to start on the flattened circuit instead of fixed 
  for(Set<PortInstance>*& s : mergeMultiplexers){
    Set<PortInstance>* flattenedSet = PushSet<PortInstance>(perm,s->map->nodesUsed);

    for(PortInstance p : s){
      flattenedSet->Insert({circuitToCopy->GetOrFail(p.inst),p.port});
    }

    s = flattenedSet;
  }
  
  for(int i = 0; i < size; i++){
    BLOCK_REGION(temp);
    String fileName = PushString(temp,"finalRecon_%d.dot",i);
    String filePath = PushDebugPath(temp,permanentName,fileName);

    GraphPrintingContent content = GenerateDelayDotGraph(recon[i],reconDelay[i],temp,debugArena);
    String result = GenerateDotGraph(recon[i],content,temp,debugArena);
    OutputContentToFile(filePath,result);
  }

  OutputDebugDotGraph(circuit,STRING("FullCircuitFinal.dot"),temp);
#if 0
  region(temp){
    GraphPrintingContent res = GenerateDefaultPrintingContent(circuit,temp,temp2);

    String content = GenerateDotGraph(circuit,res,temp,temp2);

    String filepath = PushDebugPath(temp,name,STRING("FullCircuitFinal.dot"));
    OutputContentToFile(filepath,content);
  }
#endif
  
  declInst.fixedDelayCircuit = circuit;

  FillDeclarationWithAcceleratorValuesNoDelay(&declInst,circuit,temp,temp2);
  FillDeclarationWithDelayType(&declInst);

  // TODO: Kinda ugly
  int numberInputs = 0;
  for(int i = 0; i < size; i++){
    BLOCK_REGION(temp);
    auto res = ExtractInputDelays(recon[i],reconDelay[i],0,temp,temp2);
    numberInputs = std::max(res.size,numberInputs);
  }

  // TODO: HACK. Because type inputs and outputs depend on size of baseConfig input/output, put first graph here. 
  declInst.baseConfig.inputDelays = ExtractInputDelays(recon[0],reconDelay[0],numberInputs,perm,temp);
  declInst.baseConfig.outputLatencies = ExtractOutputLatencies(recon[0],reconDelay[0],perm,temp);

  FUDeclaration* decl = RegisterFU(declInst);
  decl->type = FUDeclarationType_MERGED;

  decl->staticUnits = CollectStaticUnits(decl->fixedDelayCircuit,decl,perm);
  
  int mergedUnitsAmount = Size(result->allocated);

  Array<Hashmap<FUInstance*,int>*> reconToOrder = PushArray<Hashmap<FUInstance*,int>*>(temp,size);
  for(int i = 0; i < reconToOrder.size; i++){
    DAGOrderNodes order = reconOrder[i];
    reconToOrder[i] = PushHashmap<FUInstance*,int>(temp,order.instances.size);

    for(int ii = 0; ii < order.instances.size; ii++){
      reconToOrder[i]->Insert(order.instances[ii],order.order[ii]);
    }
  }

  decl->configInfo = PushArray<ConfigurationInfo>(perm,size);
  Memset(decl->configInfo,{});

  for(int i = 0; i < size; i++){
    decl->configInfo[i].name = types[i]->name;
    decl->configInfo[i].baseType = types[i];

    decl->configInfo[i].baseName = PushArray<String>(perm,mergedUnitsAmount);
    Memset(decl->configInfo[i].baseName,{});

    decl->configInfo[i].unitBelongs = PushArray<bool>(perm,mergedUnitsAmount);
  }
  
  for(int i = 0; i < size; i++){
    InstanceMap* map = accelToRecon[i];
    
    // Copy everything else for now. Only config and names are being handled for now
    decl->configInfo[i].configs = decl->baseConfig.configs;
    decl->configInfo[i].states = decl->baseConfig.states;
    decl->configInfo[i].stateOffsets = decl->baseConfig.stateOffsets;
    decl->configInfo[i].delayOffsets = decl->baseConfig.delayOffsets;

    decl->configInfo[i].configOffsets.offsets = PushArray<int>(perm,mergedUnitsAmount);
    decl->configInfo[i].configOffsets.max = decl->baseConfig.configOffsets.max;

    decl->configInfo[i].inputDelays = ExtractInputDelays(recon[i],reconDelay[i],numberInputs,perm,temp);
    decl->configInfo[i].outputLatencies = ExtractOutputLatencies(recon[i],reconDelay[i],perm,temp);

    decl->configInfo[i].calculatedDelays = PushArray<int>(perm,mergedUnitsAmount);
    decl->configInfo[i].order = PushArray<int>(perm,mergedUnitsAmount);

    int index = 0;
    int orderIndex = 0;
    FOREACH_LIST_INDEXED(FUInstance*,ptr,result->allocated,index){
      FUInstance** optReconNode = map->Get(ptr);
      bool mapExists = optReconNode != nullptr;
      
      if(optReconNode){
        decl->configInfo[i].calculatedDelays[orderIndex] = reconDelay[i].nodeDelay->GetOrFail(*optReconNode);
        decl->configInfo[i].order[orderIndex] = reconToOrder[i]->GetOrFail(*optReconNode);
      } else {
        decl->configInfo[i].calculatedDelays[orderIndex] = 0;
        //decl->configInfo[i].calculatedDelays[orderIndex] = -1;
        decl->configInfo[i].order[orderIndex] = -1;
      }

      if(ptr->isMergeMultiplexer || ptr->declaration == BasicDeclaration::fixedBuffer){
        int val = decl->baseConfig.configOffsets.offsets[index];
        decl->configInfo[i].configOffsets.offsets[index] = val;

        decl->configInfo[i].baseName[index] = ptr->name;
        decl->configInfo[i].unitBelongs[index] = true;
      } else if(mapExists){
        int val = decl->baseConfig.configOffsets.offsets[index];
        decl->configInfo[i].configOffsets.offsets[index] = val;

        FUInstance* originalNode = map->GetOrFail(ptr);
        decl->configInfo[i].baseName[index] = originalNode->name;
        decl->configInfo[i].unitBelongs[index] = true;
      } else {
        decl->configInfo[i].baseName[index] = ptr->name;
        decl->configInfo[i].configOffsets.offsets[index] = -1;
        decl->configInfo[i].unitBelongs[index] = false;
      }
      
#if 1
      if(mapExists){
        orderIndex += 1; // TODO: Why does this exist? Is the problem caused by the commented line above because we are trying to iterate differently?
      }
#endif
    }

    Assert(orderIndex == reconDelay[i].nodeDelay->nodesUsed);
  }

  for(int i = 0; i < size; i++){
    BLOCK_REGION(temp);

    auto firstMapping = MappingReverse(circuitToCopy,temp);
    auto f = MappingGetId(firstMapping);
    auto s = MappingGetId(mergeToFlatten[i]);
#if 0
    printf("S: %d\n",flatten[i]->id);
    printf("E: %d\n",declInst.baseCircuit->id);
    printf("CircuitToCopy:%d->%d\n",f.first,f.second);
    printf("ReverseView  :%d->%d\n",s.first,s.second);
#endif

    InstanceMap* copyToFlatten = MappingCombine(firstMapping,mergeToFlatten[i],temp);

    printf("HERE: %.*s\n",UNPACK_SS(name));
    // Need to map from flattened base type to copy of merged circuit (decl.flattenedBaseCircuit).
    decl->configInfo[i].mapping = MappingReverse(copyToFlatten,perm);
    decl->configInfo[i].mergeMultiplexers = mergeMultiplexers[i];
  }
  
  return decl;
}

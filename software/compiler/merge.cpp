#include "merge.hpp"
#include "accelerator.hpp"
#include "configurations.hpp"
#include "declaration.hpp"
#include "delayCalculation.hpp"
#include "dotGraphPrinting.hpp"
#include "filesystem.hpp"
#include "globals.hpp"
#include "logger.hpp"
#include "memory.hpp"
#include "utils.hpp"
#include "utilsCore.hpp"

#include "textualRepresentation.hpp"
#include "versat.hpp"
#include <bits/getopt_core.h>
#include <unordered_map>

bool NodeConflict(FUInstance* inst){
  // For now, do not even try to map nodes that contain any config modifiers.
  if(inst->isStatic){
    return true;
  }

  if(inst->sharedEnable){
    return true;
  }

  if(inst->isMergeMultiplexer){
    return true;
  }
  
  return false;
}

bool NodeMappingConflict(Edge edge1,Edge edge2){
  PortInstance p00 = edge1.units[0];
  PortInstance p01 = edge1.units[1];
  PortInstance p10 = edge2.units[0];
  PortInstance p11 = edge2.units[1];

  if(NodeConflict(p00.inst) || NodeConflict(p01.inst) || 
     NodeConflict(p10.inst) || NodeConflict(p11.inst)){
    return true;
  }
     
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

    if(NodeConflict(p00) || NodeConflict(p01) || 
       NodeConflict(p10) || NodeConflict(p11)){
      return true;
    }
    
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

    if(NodeConflict( p0) ||  NodeConflict(p1) || 
       NodeConflict(e00) || NodeConflict(e01) || 
       NodeConflict(e10) || NodeConflict(e11)){
      return true;
    }

#define XOR(A,B,C,D) ((A == B && !(C == D)) || (!(A == B) && C == D))

    if(XOR(p0,e00,p1,e10)){
      return true;
    }
    if(XOR(p0,e01,p1,e11)){
      return true;
    }
#undef XOR
    
  }

  return false;
}

int EdgeEqual(Edge edge1,Edge edge2){
  int res = (memcmp(&edge1,&edge2,sizeof(Edge)) == 0);

  return res;
}

ConsolidationGraph Copy(ConsolidationGraph graph,Arena* out){
  ConsolidationGraph res = {};

  res = graph;
  res.validNodes.Init(out,graph.nodes.size);
  res.validNodes.Copy(graph.validNodes);

  return res;
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

ConsolidationResult GenerateConsolidationGraph(Accelerator* accel0,Accelerator* accel1,ConsolidationGraphOptions options,Arena* out){
  TEMP_REGION(temp,out);
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

  FUInstance* accel0Output = GetOutputInstance(&accel0->allocated);
  FUInstance* accel1Output = GetOutputInstance(&accel1->allocated);
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
  for(FUInstance* ptr1 : accel0->allocated){
    FUInstance* instA = ptr1;
    if(instA->declaration != BasicDeclaration::input){
      continue;
    }

    for(FUInstance* ptr2 : accel1->allocated){
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

  //auto nodes = StartGrowableArray<MappingNode>(out);
  
  // Insert specific mappings first (the first mapping nodes are the specifics)
#if 0
  for(int i = 0; i < options.nSpecifics; i++){
    SpecificMergeNodes specific = options.specifics[i];

    MappingNode* node = nodes.PushElem()

    node->type = MappingNode::NODE;
    node->nodes.instances[0] = specific.instA;
    node->nodes.instances[1] = specific.instB;
  }
#endif

  auto nodes = StartArray<MappingNode>(out);
  
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

      if(NodeConflict(edge0.units[0].inst) || NodeConflict(edge0.units[1].inst) || 
         NodeConflict(edge1.units[0].inst) || NodeConflict(edge1.units[1].inst)){
        continue;
      }
      
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

      *nodes.PushElem() = node;
    }
  }
#endif

  // Check node mapping
#if 1
  if(1 /*options.mapNodes */){
    for(FUInstance* instA : accel0->allocated){
      PortInstance portA = {};
      portA.inst = instA;

      if(instA == accel0Output || instA->declaration == BasicDeclaration::input){
        continue;
      }

      for(FUInstance* instB : accel1->allocated){
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
        
        *nodes.PushElem() = node;
      }
    }
  }
#endif

  graph.nodes = EndArray(nodes);

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

CliqueState MaxClique(ConsolidationGraph graph,int upperBound,Arena* out,Time MAX_CLIQUE_TIME){
  CliqueState state = {};
  state.table = PushArray<int>(out,graph.nodes.size);
  state.clique = Copy(graph,out); // Preserve nodes and edges, but allocates different valid nodes

  graph.validNodes.Fill(0);

  state.start = GetTime();
  for(int i = graph.nodes.size - 1; i >= 0; i--){
    BLOCK_REGION(out);

    state.found = false;
    for(int j = i; j < graph.nodes.size; j++){
      graph.validNodes.Set(j,1);
    }

    graph.validNodes &= graph.edges[i];

    IndexRecord record = {};
    record.index = i;

    Clique(&state,graph,i,&record,1,out,MAX_CLIQUE_TIME);
    state.table[i] = state.max;

    if(state.max == upperBound){
      break;
    }

    auto end = GetTime();
    Time elapsed = end - state.start;
    if(elapsed > MAX_CLIQUE_TIME){
      break;
    }
  }

  Assert(IsClique(state.clique).result);

  return state;
}

void OutputConsolidationGraph(ConsolidationGraph graph,bool onlyOutputValid,String moduleName,String fileName){
  TEMP_REGION(temp,nullptr);

  if(!globalOptions.debug){
    return;
  }

  BLOCK_REGION(temp);
  String filePath = PushDebugPath(temp,moduleName,fileName);
  
  FILE* outputFile = OpenFileAndCreateDirectories(filePath,"w",FilePurpose_DEBUG_INFO);
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

GraphMapping ConsolidationGraphMapping(Accelerator* accel1,Accelerator* accel2,ConsolidationGraphOptions options,String name,Arena* out){
  TEMP_REGION(temp,out);

  ConsolidationResult result = {};
  region(out){
    result = GenerateConsolidationGraph(accel1,accel2,options,temp);
  }
  ConsolidationGraph graph = result.graph;

  if(globalDebug.outputConsolidationGraphs){
    OutputConsolidationGraph(graph,true,name,STRING("ConsolidationGraph.dot"));
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
  ConsolidationGraph clique = MaxClique(graph,upperBound,temp,Seconds(10)).clique;

#if 0
  printf("Clique: %d\n",ValidNodes(clique));
#endif

  if(globalDebug.outputGraphs){
    OutputConsolidationGraph(clique,true,name,STRING("Clique1.dot"));
  }
  AddCliqueToMapping(res,clique);

  return res;
}

static GraphMapping FirstFitGraphMapping(Accelerator* accel1,Accelerator* accel2,Arena* out){
  GraphMapping res = InitGraphMapping(out);

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

static GraphMapping CalculateMergeMapping(Accelerator* accel1,Accelerator* accel2,Array<SpecificMergeNodes> specificNodes,MergingStrategy strategy,String name,Arena* out){
  TEMP_REGION(temp,out);
  
  GraphMapping graphMapping = InitGraphMapping(out);

  switch(strategy){
  case MergingStrategy::SIMPLE_COMBINATION:{
    // Only map inputs and outputs
    FUInstance* accel1Output = GetOutputInstance(&accel1->allocated);
    FUInstance* accel2Output = GetOutputInstance(&accel2->allocated);

    if(accel1Output && accel2Output){
      InsertMapping(graphMapping,accel2Output,accel1Output);
    }
    
    // Map inputs
    for(FUInstance* ptr1 : accel1->allocated){
      FUInstance* instA = ptr1;
      if(instA->declaration != BasicDeclaration::input){
        continue;
      }

      for(FUInstance* ptr2 : accel2->allocated){
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

    graphMapping = ConsolidationGraphMapping(accel1,accel2,options,name,out);
  }break;
  case MergingStrategy::FIRST_FIT:{
    graphMapping = FirstFitGraphMapping(accel1,accel2,out);
  }break;
  }

  return graphMapping;
}

String GetFreeMergeMultiplexerName(Accelerator* accel,Arena* out){
  auto mark = MarkArena(out);

  for(int i = 0; true; i++){
    String possibleName = PushString(out,"versat_merge_mux_%d",i);

    if(NameExists(accel,possibleName)){
      PopMark(mark);
      continue;
    }

    return possibleName;
  }

  NOT_POSSIBLE();
}

int GetFreeMuxGroup(Accelerator* accel){
  int muxGroup = 0;
  while(1){
    bool found = true;
    for(FUInstance* ptr : accel->allocated){
      if(ptr->isMergeMultiplexer && ptr->muxGroup == muxGroup){
        muxGroup += 1;
        found = false;
        break;
      }
    }

    if(found){
      return muxGroup;
    }
  }
}

MergeGraphResultExisting MergeGraphToExisting(Accelerator* existing,Accelerator* flatten2,GraphMapping& graphMapping,String name,Arena* out){
  TEMP_REGION(temp,out);

  int size1 = existing->allocated.Size();
  int size2 = flatten2->allocated.Size();

  // These maps are returned
  AcceleratorMapping* map2 = MappingSimple(flatten2,existing,out);
  
  Hashmap<FUInstance*,FUInstance*>* map = PushHashmap<FUInstance*,FUInstance*>(out,size1 + size2);
  TrieMap<int,int>* sharedIndexMap = PushTrieMap<int,int>(temp);
  
  for(FUInstance* ptr : existing->allocated){
    map->Insert(ptr,ptr);
  }

  // Create base instances from accel 2, unless they are mapped to nodes from accel1
  for(FUInstance* inst : flatten2->allocated){
    FUInstance** mapping = graphMapping.instanceMap->Get(inst); // Returns mapping from accel2 to accel1

    bool forceSkip = (inst->sharedEnable || inst->isStatic);
    
    FUInstance* mappedNode = nullptr;
    if(mapping && !forceSkip){
      mappedNode = map->GetOrFail(*mapping);

      // For now, change name even if they have the same name
      // if(!CompareString(inst->name,mappedNode->name)){
      String newName = PushString(globalPermanent,"%.*s_%.*s",UNPACK_SS(mappedNode->name),UNPACK_SS(inst->name));

      mappedNode->name = newName;
      //}
    } else {
      String newName = inst->name;
      if(inst->isMergeMultiplexer){
        newName = GetFreeMergeMultiplexerName(existing,globalPermanent);

        GetOrAllocateResult result = sharedIndexMap->GetOrAllocate(inst->muxGroup);
        if(!result.alreadyExisted){
          *result.data = GetFreeMuxGroup(existing);
        }

        inst->muxGroup = *result.data;
      }
      
      mappedNode = CopyInstance(existing,inst,false,newName);

      Assert(!inst->isStatic);
      if(inst->sharedEnable){
        GetOrAllocateResult result = sharedIndexMap->GetOrAllocate(inst->sharedIndex);

        if(!result.alreadyExisted){
          *result.data = GetFreeShareIndex(existing);
        }
        
        int id = *result.data;
        ShareInstanceConfig(mappedNode,id);
      }
    }
    FUInstance* newNode = mappedNode;
    map->Insert(inst,mappedNode);
    MappingInsertEqualNode(map2,inst,newNode);
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

Array<int> GetPortConnections(FUInstance* node,Arena* out){
  int maxPorts = 0;
  FOREACH_LIST(ConnectionNode*,con,node->allInputs){
    maxPorts = std::max(maxPorts,con->port);
  }

  Array<int> res = PushArray<int>(out,maxPorts + 1);
  Memset(res,0);

  FOREACH_LIST(ConnectionNode*,con,node->allInputs){
    res[con->port] += 1;
  }

  return res;
}

Array<Edge*> GetAllPaths(Accelerator* accel,PortInstance start,PortInstance end,Arena* out){
  TEMP_REGION(temp,out);

  // TODO: Maybe this happens for circuits that make loops.
  //       Something is not right, check later why does the Assert fail for the FullAESRounds
  //Assert(!(start == end));
  if(start == end){
    return {};
  }
  
  if(end.inst->type == NodeType_SOURCE_AND_SINK){
    return {};
  }

  PortInstance outInst = end.inst->inputs[end.port];
  if(!outInst.inst){
    return {};
  }
    
  Edge edge = {.out = outInst,.in = end};
    
  if(outInst == start){
    Edge* edgePtr = PushStruct<Edge>(out);
    *edgePtr = edge;
    edgePtr->next = nullptr;
    Array<Edge*> result = PushArray<Edge*>(out,1);
    result[0] = edgePtr;
    
    return result;
  }
    
  ArenaList<Edge*>* allPaths = PushArenaList<Edge*>(temp);
  for(int outPort = 0; outPort < outInst.inst->inputs.size; outPort++){
    PortInstance outInputSide = {.inst = outInst.inst,.port = outPort};
    
    Array<Edge*> subPaths = GetAllPaths(accel,start,outInputSide,out);

    // TODO: A little bit slow. This approach is a bit problematic.
    EdgeIterator iter = IterateEdges(accel);
    int edgeDelay = 0;
    while(iter.HasNext()){
      Edge edge = iter.Next();
      
      if(edge.out == start && edge.in == outInputSide){
        edgeDelay = edge.delay;
      }
    }
    
    for(Edge* subEdge : subPaths){
      Edge* newEdge = PushStruct<Edge>(out);
      *newEdge = edge;
      newEdge->delay = edgeDelay;
      newEdge->next = subEdge;
      
      *allPaths->PushElem() = newEdge;
    }
  }

  Array<Edge*> result = PushArrayFromList(out,allPaths);

  for(int i = 0; i < result.size; i++){
    result[i] = ReverseList(result[i]);
  }
  
  return result;
}

void PrintPath(Edge* head){
  FOREACH_LIST(Edge*,e,head){
    printf("%.*s:%d -[%d]> %.*s:%d\n",UNPACK_SS(e->units[0].inst->name),e->units[0].port,e->delay,UNPACK_SS(e->units[1].inst->name),e->units[1].port);
  }
}

// This entire approach is so bad.
// The only good non bug ridded approach is to keep graph reconstitutions during the entire process of expanding the graphs. 
ReconstituteResult ReconstituteGraph(Accelerator* merged,TrieSet<PortInstance>* mergedMultiplexers,Accelerator* base,String name,AcceleratorMapping* baseToMerged,Arena* out){
  TEMP_REGION(temp,out);
  Accelerator* recon = CreateAccelerator(name,AcceleratorPurpose_RECON);

  MappingCheck(baseToMerged);
  AcceleratorMapping* mergedToBase = MappingInvert(baseToMerged,temp);
  MappingCheck(mergedToBase,merged,base);
 
  for(PortInstance portInst : mergedMultiplexers){
    Assert(portInst.inst->accel->id == merged->id);
    break;
  }

  AcceleratorMapping* accelToRecon = MappingSimple(merged,recon,out);

  Hashmap<FUInstance*,FUInstance*>* instancesAdded = PushHashmap<FUInstance*,FUInstance*>(temp,9999);

  FUDeclaration* muxType = GetTypeByName(STRING("CombMux8")); // TODO: Kind of an hack

  EdgeIterator iter = IterateEdges(base);
  while(iter.HasNext()){
    Edge edgeInst = iter.Next();
    Edge* edge = &edgeInst;

    PortInstance start = MappingMapOutput(baseToMerged,edge->units[0]);
    PortInstance end = MappingMapInput(baseToMerged,edge->units[1]);

    // This might be needed because flatten map does not do a full map.
    //   Because it contains Input and Output units (and maybe some other reasons?)
    if(start.inst == nullptr || end.inst == nullptr){
      continue;
    }

    Array<Edge*> allPaths = GetAllPaths(merged,start,end,temp);

    auto allValidPathsDyn = StartArray<Edge*>(temp);
    for(Edge* p : allPaths){
      //PrintPath(p);
      bool valid = true;
      FOREACH_LIST(Edge*,edge,p){
        if(edge->in.inst->declaration == muxType){
          if(!mergedMultiplexers->Exists(edge->in)){
            valid = false;
            break;
          }
        }
      }
      
      if(valid){
        *allValidPathsDyn.PushElem() = p;
      }
    }
    Array<Edge*> allValidPaths = EndArray(allValidPathsDyn);
    
    // Replicate exact same path on new accelerator.
    for(Edge* validPath : allValidPaths){
      FOREACH_LIST(Edge*,edgeOnMerged,validPath){
        PortInstance node0 = edgeOnMerged->units[0];
        PortInstance node1 = edgeOnMerged->units[1];
        GetOrAllocateResult<FUInstance*> res0 = instancesAdded->GetOrAllocate(node0.inst);
        GetOrAllocateResult<FUInstance*> res1 = instancesAdded->GetOrAllocate(node1.inst);
        
        if(!res0.alreadyExisted){
          PortInstance optOriginal = MappingMapOutput(mergedToBase,node0);

          String name = node0.inst->name;
          if(optOriginal.inst){
            name = optOriginal.inst->name;
          }
        
          *res0.data = CopyInstance(recon,node0.inst,true,name);
        
          MappingInsertEqualNode(accelToRecon,node0.inst,*res0.data);
        }
      
        if(!res1.alreadyExisted){
          PortInstance optOriginal = MappingMapInput(mergedToBase,node1);

          String name = node1.inst->name;
          if(optOriginal.inst){
            name = optOriginal.inst->name;
          }

          *res1.data = CopyInstance(recon,node1.inst,true,name);

          MappingInsertEqualNode(accelToRecon,node1.inst,*res1.data);
        }
      
        ConnectUnitsIfNotConnected(*res0.data,edgeOnMerged->units[0].port,*res1.data,edgeOnMerged->units[1].port,edgeOnMerged->delay);
      }
    }
  }

  ReconstituteResult result = {};
  result.accel = recon;
  result.accelToRecon = accelToRecon;
  
  return result;
}

// TODO: Going by name is asking for trouble.
//       Eventually want to retire this
FUInstance* GetInstanceByName(Accelerator* accel,String name){
  for(FUInstance* inst : accel->allocated){
    if(CompareString(inst->name,name)){
      return inst;
    }
  }

  return nullptr;
}

// TODO: This is a bit forced. Very bug prone and we probably can do something better if we can preserve the mappings from the flattened operations and work from there.
//       Otherwise we need to enforce some constraints to make sure that this function does not "just work" from being lucky. Because it feels like the only reason this works is because the tests are not trying hard enough to blow this up.

// NOTE: I think that part of the reason why this exists in the first place is because we do not have flatten mapping info that goes accross module hierarchies. If A contains B that contains C, and C is a merged unit, we might have to map flatten C -> flatten B and flatten B -> flatten A in order to merge A.
//       Which means that we have to flatten and keep mapping info accross hierarchies.
//       While this function achieves the same by just looking at the names, which are mostly preserved through flattening, although this is not enforced and as such it contains a high change of bugging out for more complex workflows.
AcceleratorMapping* MapFlattenedGraphs(Accelerator* start,FUDeclaration* endDecl,Accelerator* end,Arena* out){
  AcceleratorMapping* mapping = MappingSimple(start,end,out);

  BLOCK_REGION(out);
  
  for(FUInstance* inst : endDecl->baseCircuit->allocated){
    if(inst->declaration->baseCircuit){
      for(FUInstance* sub : inst->declaration->baseCircuit->allocated){
        // What do we do about outputs ???
        if(sub->declaration == BasicDeclaration::output){
          // int port = sub->portIndex;
          
          String subName = sub->name;
          String fullName = PushString(out,"%.*s_%.*s",UNPACK_SS(inst->name),UNPACK_SS(subName)); // A bit hardcoded
          int mappedToPort = 0;
          for(PortInstance portInst : sub->inputs){
            int port = portInst.port;

            SubMappingInfo in = {};
            in.isInput = false;
            in.subPort = port;
            in.subDeclaration = inst->declaration;
            in.higherName = inst->name;
            
            PortInstance* optPort = endDecl->flattenMapping->Get(in);

            if(!optPort){
              continue;
            }
            
            PortInstance mappedTo = *optPort;
            subName = mappedTo.inst->name;
            mappedToPort = mappedTo.port;
            fullName = subName; // No inst->name added because we are at the top of inst

            FUInstance* startInst = GetInstanceByName(start,sub->name);
            FUInstance* endInst = GetInstanceByName(end,fullName);
            
            if(startInst != nullptr && endInst != nullptr){
              MappingInsertInput(mapping,{startInst,port},{endInst,mappedToPort});
            }
          }
        } else {
          String subName = sub->name;
          String fullName = PushString(out,"%.*s_%.*s",UNPACK_SS(inst->name),UNPACK_SS(subName)); // A bit hardcoded

          int mappedToPort = 0;
          if(sub->declaration == BasicDeclaration::input){
            int port = sub->portIndex;

            SubMappingInfo in = {};
            in.isInput = true;
            in.subPort = port;
            in.subDeclaration = inst->declaration;
            in.higherName = inst->name;

            PortInstance* optPort = endDecl->flattenMapping->Get(in);

            if(optPort){
              PortInstance mappedTo = *optPort;
              subName = mappedTo.inst->name;
              mappedToPort = mappedTo.port;
              fullName = subName; // No inst->name added because we are at the top of inst
            }
          }

          FUInstance* startInst = GetInstanceByName(start,sub->name);
          FUInstance* endInst = GetInstanceByName(end,fullName);
        
          if(startInst != nullptr && endInst != nullptr){
            if(startInst->declaration->type == FUDeclarationType_SPECIAL){
              // Need port of the actual units.

              MappingInsertOutput(mapping,{startInst,0},{endInst,mappedToPort});
            } else {
              Assert(endInst->declaration->type != FUDeclarationType_SPECIAL);
              MappingInsertEqualNode(mapping,startInst,endInst);
            }
          }
        }
      }
    }
  }

  return mapping;
}

// Recurses over composite but does not recurse lower than merged units.
Array<GraphAndMapping> GetAllBaseMergeTypes(FUDeclaration* decl,Arena* out){
  TEMP_REGION(temp,out);
  
  BLOCK_REGION(temp);

  Set<GraphAndMapping>* seen = PushSet<GraphAndMapping>(temp,10);

  if(decl->type == FUDeclarationType_MERGED){
    for(MergePartition info : decl->info.infos){
      seen->Insert({.decl = info.baseType,.map = info.baseTypeFlattenToMergedBaseCircuit,.mergeMultiplexers = info.mergeMultiplexers});

      // So, we have the map to the base circuit.
      
    }
  } else {
    // We check fixedDelayCircuit since composites preserves instantiation of merged and composite units 
    // But eventually we only want to perform mappings between flattened circuits.
    // For now we perform copy of the instance id, meaning that in theory we should be able to map units by searching for equal ids.
    // This works because composites preserve ids. Merging is also preserving ids when it shouldn't.
    
    for(FUInstance* inst : decl->fixedDelayCircuit->allocated){
      FUDeclaration* subDecl = inst->declaration;
      
      if(subDecl->type == FUDeclarationType_MERGED){
        // NOTE: This code is not being tested by the current tests, so it might not work well.
        
        for(MergePartition info : subDecl->info.infos){
          AcceleratorMapping* subUnitBaseTypeToSubUnitCircuit = info.baseTypeFlattenToMergedBaseCircuit;

          // we have unit A that contains unit B which can have multiple partitions B1,B2,B3. We also have the mappings B1 -> B, B2 -> B and so on
          // We create mapping B -> A.
          // Then we map combine B1 -> B with B -> A in order to get B1 -> A.
          // And do the same for B2 -> A and so on.
          AcceleratorMapping* subUnitCircuitToParentCircuit = MapFlattenedGraphs(subDecl->flattenedBaseCircuit,decl,decl->flattenedBaseCircuit,out);
          AcceleratorMapping* subUnitBaseToParent = MappingCombine(subUnitBaseTypeToSubUnitCircuit,subUnitCircuitToParentCircuit,globalPermanent);
          
          Set<PortInstance>* trueMergeMultiplexers = MappingMapInput(subUnitCircuitToParentCircuit,info.mergeMultiplexers,globalPermanent);
          
          seen->Insert({.decl = info.baseType,.map = subUnitBaseToParent,.mergeMultiplexers = trueMergeMultiplexers,.fromStruct = true});
        }
      } else if(subDecl->type == FUDeclarationType_COMPOSITE ||
                subDecl->type == FUDeclarationType_ITERATIVE){
        Array<GraphAndMapping> subTypes = GetAllBaseMergeTypes(subDecl,temp);

        for(GraphAndMapping result : subTypes){
          seen->Insert(result);
        }
      }
    }
  }

  Array<GraphAndMapping> result = PushArrayFromSet(out,seen);
  
  // TODO: Problem was in here.
  //       The problem is that we are mapping to the base circuit, but we want to map to the flattened circuit?
  for(GraphAndMapping mapping : result){
    //Assert(false);
    //Assert(mapping.map->secondId == decl->baseCircuit->id);
    Assert(mapping.map->secondId == decl->flattenedBaseCircuit->id);
  }
  
  return result;
}

/*
  Imagine if we have merge X = A | B | C;
  Imagine if we have merge Y = D | E | F;

  merge X | Y == X_A | X_B | X_C | Y_D | Y_E | Y_F;

  if we have a module Z = X x,Y y then we have

  AD_AE_AF_BD_BE_BF_CD_CE_CF = 9 options.

  Imagine X = A | B

  Module Y with X x,Const b.

  merge Y | X == Y_X_A | Y_X_B | X_A | X_B;  

  So merges add, modules multiply.
*/

ReconstituteResult ReconstituteGraphFromStruct(Accelerator* merged,TrieSet<PortInstance>* mergedMultiplexers,Accelerator* base,String name,AcceleratorMapping* baseToMerged,FUDeclaration* parentType,Arena* out);

// TODO: The one thing that I still dislike heavily is the reconstitution of the graphs from the merged graphs.
//       We are basically assuming that after having the merged graph, we can derive the reconstitutions in a fine manner. I think that this is bug/error prone.
//       The more "correct" approach is to start with the base graphs of each type and any modification made to the merge graph is propagated to the "recon" graphs (only mux and buffers are added). This guarantees that at the end, when the merged graph is correct, the recon graphs should be too, and there is no need to find paths and do all that to get the final result.
//       The only problem, of course, is that we are constantly carrying a bunch of graphs and one change in a graph must be correctly replicated in all the others. Also do not know if we do not arrive at some problems of coherence (like if we a mux to a graph, we are now changing the path of the graph forever. We then need to make sure that any further addition still makes sense. Ex: if X -> Y and we add mux M so that we have X->M->Y, and then we find out that we need to add a buffer B to X->Y, then I need to make sure that we either have X->M->B->Y or X->B->M->Y [but thinking more, if we add a mux, we add it everywhere where X and Y exist, so the buffer must be added to a graph where M also exist.] ).

//       So, basically, an api/interface where we can carry a bunch of graphs and make operations on one graph which are seamlessly integrated into the other graphs (some operations, others do nothing) is what I think is the best way of simplifying the merge algorithm.

//       Need to keep mappings between graphs, persist them as changes are made and make sure that no desyncs occur.
//       The ability to merge modules that contain merge units themselves makes this harder, I think. We basically have to move that loop in the middle of merge that handles multipe recons to the start. We start by calculating all the "recons" that we need, their initial graph. 

// TODO: Also, need to figure out if we can insert hierarchical merge in steps in here somehow (where we merge the upper layers, non flattened, and if we can find some mappings, we propagated that information when going down layers so that we can find cliques faster, using hierarchical information to speedup the clique finding (or even as a form of doing a faster heuristic to find a mapping without using the clique algorithm).

FUDeclaration* Merge(Array<FUDeclaration*> types,
                     String name,Array<SpecificMergeNode> specifics,
                     MergingStrategy strat){
  
  TEMP_REGION(temp,nullptr);
  TEMP_REGION(temp2,temp);

  Assert(types.size >= 2);

  strat = MergingStrategy::CONSOLIDATION_GRAPH;

  int size = types.size;
  Array<Accelerator*> flatten = PushArray<Accelerator*>(temp,size);
  Array<AcceleratorMapping*> flattenToMergedAccel = PushArray<AcceleratorMapping*>(temp,size);
  Array<AcceleratorMapping*> mergedAccelToFlatten = PushArray<AcceleratorMapping*>(temp,size); 
  
  int mergedAmount = 0;
  for(FUDeclaration* decl : types){
    mergedAmount += std::max(decl->MergePartitionSize(),1);
  }

  for(int i = 0; i < size; i++){
    flatten[i] = types[i]->flattenedBaseCircuit;
  }
    
  // We copy accel 1.
  Pair<Accelerator*,AcceleratorMapping*> firstCopy = CopyAcceleratorWithMapping(flatten[0],AcceleratorPurpose_MERGE,false,globalPermanent);

  Accelerator* mergedAccel = firstCopy.first;
  flattenToMergedAccel[0] = firstCopy.second;
  
  Array<Accelerator*> orderedAccelerators = [&](){
    auto arr = StartArray<Accelerator*>(temp);
    bool first = true;
    for(Accelerator* accel : flatten){
      if(first){
        *arr.PushElem() = mergedAccel;
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
      FUInstance* left = GetInstanceByName(orderedAccelerators[node.firstIndex],node.firstName);
      FUInstance* right = GetInstanceByName(orderedAccelerators[node.secondIndex],node.secondName);
      *arr.PushElem() = {left,right};
    }
    return EndArray(arr);
  }();

  for(int i = 1; i < size; i++){
    String folderName = PushString(temp,"%.*s/Merge_%d",UNPACK_SS(name),i);

    GraphMapping map = CalculateMergeMapping(mergedAccel,flatten[i],specificNodes,strat,folderName,temp2);
    MergeGraphResultExisting res = MergeGraphToExisting(mergedAccel,flatten[i],map,folderName,temp2);

    OutputDebugDotGraph(res.result,STRING(StaticFormat("result%d.dot",i)));

    flattenToMergedAccel[i] = res.map2;
    mergedAccel = res.result;
  }
  
  // TODO: This might indicate the need to implement a bidirectional map.
  for(int i = 0; i < size; i++){
    mergedAccelToFlatten[i] = MappingInvert(flattenToMergedAccel[i],temp);
  }

  // Need to map from final to each input graph
  int numberOfMultipleInputs = 0;
  for(FUInstance* ptr : mergedAccel->allocated){
    if(ptr->multipleSamePortInputs){
      numberOfMultipleInputs += 1;
    }
  }

  Array<InstanceMap*> mapMultipleInputsToSubgraphs = PushArray<InstanceMap*>(temp,size);

  for(int i = 0; i < size; i++){
    mapMultipleInputsToSubgraphs[i] = PushHashmap<FUInstance*,FUInstance*>(temp,numberOfMultipleInputs);

    for(auto pair : flattenToMergedAccel[i]->inputMap){ // NOTE: Is InputMap different than OutputMap?
      if(pair.second.inst->multipleSamePortInputs){
        mapMultipleInputsToSubgraphs[i]->Insert(pair.second.inst,pair.first.inst);
      }
    }
  }

  Array<TrieSet<PortInstance>*> mergeMultiplexers = PushArray<TrieSet<PortInstance>*>(globalPermanent,size);

  // TODO: We eventually must be able to handle generic units instantiation.
  // NOTE: We could currently reduce resource usage by, after doing the merge, replacing every mux added with the smallest version that we can. Only changing the decl of an instance is easy.
  FUDeclaration* muxType = GetTypeByName(STRING("CombMux8"));

  for(TrieSet<PortInstance>*& s : mergeMultiplexers){
    s = PushTrieSet<PortInstance>(temp); // TODO: Correct size or change to TrieSet. NOTE: Temp because later on the function we replace this maps from fixedCircuit to flattened circuit
  }

  EdgeIterator iter = IterateEdges(mergedAccel);
  while(iter.HasNext()){
    Edge edge = iter.Next();

    if(edge.in.inst->declaration == muxType){
      for(TrieSet<PortInstance>* s : mergeMultiplexers){
        s->Insert(edge.in);
      }
    }
  }

  bool insertedAMultiplexer = false;

  // Different usages because normal units can also be shared, but muxGroup is only for muxs
  // We could identify different mux groups by different shared indexes but easier to debug using mux groups
  int freeShareIndex = GetFreeShareIndex(mergedAccel);
  int muxGroup = GetFreeMuxGroup(mergedAccel);

  // After merging everything into a single graph we fix the datapaths by inserting muxs
  // At this point we do not preserve info from the lower types.
  // We do not know which type a mux input port belongs too.
  // Specially because at the end, we might have multiple types and subtypes reclaiming the same input port of a merge.
  // We solve this later by reconstituting the base types into the merged path created here
  // I kinda wonder if it is possible to preserve info instead of having to reconstitute later.
  // The problem is we first must find all the lower types and everytime we add something to the graph we also add the same to the recon graphs.
  // So we end up with the same recons, but instead of doing the work beforehand, we do some of the work first and make sure that every addition to the merge graph is also replicated in all the recon graphs as well.
  for(FUInstance* ptr : mergedAccel->allocated){
    if(ptr->multipleSamePortInputs){
      // Need to figure out which port, (which might be multiple), has the same inputs
      Array<int> portConnections = GetPortConnections(ptr,temp);

      for(int port = 0; port < portConnections.size; port++){
        if(portConnections[port] < 2){
          continue;
        }

        int numberOfMultiple = portConnections[port];

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
                PortInstance otherPort = MappingMapOutput(flattenToMergedAccel[ii],con->instConnectedTo);

                FUInstance* other = otherPort.inst;

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
        String str = GetFreeMergeMultiplexerName(mergedAccel,globalPermanent);
        insertedAMultiplexer = true;
        PushNullByte(globalPermanent);
        FUInstance* multiplexer = CreateFUInstance(mergedAccel,muxType,str);
        multiplexer->isMergeMultiplexer = true;
        ShareInstanceConfig(multiplexer,freeShareIndex);
        multiplexer->muxGroup = muxGroup;
        
        for(int i = 0; i < problematicEdgesInFinalGraph.size; i++){
          Edge node = problematicEdgesInFinalGraph[i];

          for(int ii = 0; ii < inputNumberToEdgeIndex.size; ii++){
            int edgeIndex = inputNumberToEdgeIndex[ii];
            if(edgeIndex != i){
              continue;
            }

            // Port N is used to select dataflow graph N.
            int graphIndex = ii;

            PortInstance portInst = {.inst = multiplexer,.port = graphIndex};
            mergeMultiplexers[graphIndex]->Insert(portInst);
            
            RemoveConnection(mergedAccel,node.units[0].inst,node.units[0].port,node.units[1].inst,node.units[1].port);
            ConnectUnits(node.units[0],{multiplexer,graphIndex},node.delay);
          }
        }

        ConnectUnits((PortInstance){multiplexer,0},{ptr,port},0);
      }
    }
  }
  
  String permanentName = PushString(globalPermanent,name);
  mergedAccel->name = permanentName;

  // TODO: Join the Merge and the Subunit register function common functionality into one
  FUDeclaration declInst = {};
  declInst.name = name;
  declInst.parameters = PushArray<String>(globalPermanent,6);
  declInst.parameters[0] = STRING("ADDR_W");
  declInst.parameters[1] = STRING("DATA_W");
  declInst.parameters[2] = STRING("DELAY_W");
  declInst.parameters[3] = STRING("AXI_ADDR_W");
  declInst.parameters[4] = STRING("AXI_DATA_W");
  declInst.parameters[5] = STRING("LEN_W");

  Pair<Accelerator*,AcceleratorMapping*> baseCopy = CopyAcceleratorWithMapping(mergedAccel,AcceleratorPurpose_BASE,true,globalPermanent);

  // Flattened and base should be the same.
  declInst.baseCircuit = baseCopy.first;
  declInst.flattenedBaseCircuit = declInst.baseCircuit; // Should already be flattened, I think?
  
  AcceleratorMapping* mergedAccelToBaseCircuit = baseCopy.second;
  
  // Output full circuit
  OutputDebugDotGraph(declInst.baseCircuit,STRING("FullCircuit.dot"));

  int actualMergedAmount = mergedAmount;
  mergedAmount = size; // TODO: FOR NOW

  // TODO: Correct sizes
  Array<Accelerator*> recon = PushArray<Accelerator*>(temp,99);
  Array<AcceleratorMapping*> reconToMergedAccel = PushArray<AcceleratorMapping*>(temp,99);
  Array<DAGOrderNodes> reconOrder = PushArray<DAGOrderNodes>(temp,99);
  Array<CalculateDelayResult> reconDelay = PushArray<CalculateDelayResult>(temp,99);
  
  // TODO: Because we are also keeping the middle graphs in memory, we do not use the actualMergedAmount, but instead a value that is slightly higher (but should not be more than half, I think).
  //       This value can be calculated beforehand but I might change the entire approach taken, so no point in looking into it right now.
  actualMergedAmount = 99;

  Array<FUDeclaration*> topType = PushArray<FUDeclaration*>(temp,actualMergedAmount);

  Array<Accelerator*> mergedAccelerators = PushArray<Accelerator*>(temp,actualMergedAmount);
  Array<Accelerator*> reconAccelerators = PushArray<Accelerator*>(temp,actualMergedAmount);
  Array<TrieSet<PortInstance>*> mergedMultiplexers = PushArray<TrieSet<PortInstance>*>(temp,actualMergedAmount);
  Array<Accelerator*> baseCircuits = PushArray<Accelerator*>(temp,actualMergedAmount);
  Array<FUDeclaration*> baseCircuitType = PushArray<FUDeclaration*>(temp,actualMergedAmount);
  Array<AcceleratorMapping*> maps = PushArray<AcceleratorMapping*>(temp,actualMergedAmount);
  Array<bool> lowestLevel = PushArray<bool>(temp,actualMergedAmount);
  Array<AcceleratorMapping*> resultToCircuit = PushArray<AcceleratorMapping*>(temp,actualMergedAmount);
  Array<int> parents = PushArray<int>(temp,actualMergedAmount);
  Array<bool> isFromStruct = PushArray<bool>(temp,actualMergedAmount);

  // We are reconstituting the base graph types into the merged graph.
  // We can then map between the merged graph and the base types in order to extract wathever information we need.
  int outerIndex = 0;
  while(true){
    // Init first iteration
    for(int i = 0; i < size; i++){
      mergedAccelerators[i] = mergedAccel;
      reconAccelerators[i] = nullptr;
      mergedMultiplexers[i] = mergeMultiplexers[i];
      baseCircuits[i] = flatten[i];
      baseCircuitType[i] = types[i];
      maps[i] = flattenToMergedAccel[i];
      lowestLevel[i] = false;
      resultToCircuit[i] = nullptr;
      parents[i] = -1;
      topType[i] = nullptr;
      isFromStruct[i] = false;
    }

    for(int i = 0; i < types.size; i++){
      topType[i] = types[i];
    }

    int start = 0;
    int toProcess = size;
    int toWrite = size;
    
    while(start != toProcess){
      for(int i = start; i < toProcess; i++){
        ReconstituteResult result = {};

        if(isFromStruct[i]){
          FUDeclaration* parentType = baseCircuitType[parents[i]];
          result = ReconstituteGraphFromStruct(mergedAccelerators[i],mergedMultiplexers[i],baseCircuits[i],permanentName,maps[i],parentType,temp);
        } else {
          result = ReconstituteGraph(mergedAccelerators[i],mergedMultiplexers[i],baseCircuits[i],permanentName,maps[i],temp);
        }

        Assert(result.accel->allocated.Size() > 0);
        
        Array<GraphAndMapping> subMerged = GetAllBaseMergeTypes(baseCircuitType[i],temp);
       
        if(subMerged.size == 0){
          lowestLevel[i] = true;
        }

        reconAccelerators[i] = result.accel;
        AcceleratorMapping* reconToAccel = MappingInvert(result.accelToRecon,globalPermanent);
          
        int parent = parents[i];
        if(parent == -1){
          resultToCircuit[i] = reconToAccel;
        } else {
          topType[i] = topType[parent];
          resultToCircuit[i] = MappingCombine(reconToAccel,resultToCircuit[parent],globalPermanent);
        }
          
        AcceleratorMapping* flattenToRecon = MappingCombine(maps[i],result.accelToRecon,temp);
        
        for(int ii = 0; ii < subMerged.size; ii++){
          GraphAndMapping subDecl = subMerged[ii];
          Accelerator* mergedAccel = subDecl.decl->flattenedBaseCircuit;

          //Assert(!subDecl.fromStruct);
          
          AcceleratorMapping* baseToMergedAccel = subDecl.map;

          AcceleratorMapping* combined = MappingCombine(baseToMergedAccel,flattenToRecon,temp);
 
          TrieSet<PortInstance>* trueMergeMultiplexers = PushTrieSet<PortInstance>(temp);

          for(PortInstance p : subDecl.mergeMultiplexers){
            trueMergeMultiplexers->Insert(MappingMapInput(flattenToRecon,p));
          }

          MappingCheck(flattenToRecon);
        
          for(PortInstance p : mergeMultiplexers[i]){
            trueMergeMultiplexers->Insert(MappingMapInput(result.accelToRecon,p));
          }
          
          mergedAccelerators[toWrite] = result.accel;
          mergedMultiplexers[toWrite] = trueMergeMultiplexers;
          baseCircuits[toWrite] = mergedAccel;
          baseCircuitType[toWrite] = subDecl.decl;
          maps[toWrite] = combined;
          parents[toWrite] = i;
          isFromStruct[toWrite] = subDecl.fromStruct;
          
          toWrite += 1;
        }
      }

      start = toProcess;
      toProcess = toWrite;
    }
      
    outerIndex += 1;

    recon.size = toProcess;
    reconToMergedAccel.size = toProcess;
    reconOrder.size = toProcess;
    reconDelay.size = toProcess;

    Memset(recon,(Accelerator*) nullptr);
    Memset(reconToMergedAccel,(AcceleratorMapping*) nullptr);

    int amountOfGoods = 0;
    for(int i = 0; i < toProcess; i++){
      if(lowestLevel[i]){
        recon[amountOfGoods] = reconAccelerators[i];
        reconToMergedAccel[amountOfGoods] = resultToCircuit[i];
          
        amountOfGoods += 1;
      }
    }

    recon.size = amountOfGoods;
    reconToMergedAccel.size = amountOfGoods;
    reconOrder.size = amountOfGoods;
    reconDelay.size = amountOfGoods;

    bool delayInserted = false;
    for(int i = 0; i < recon.size; i++){
      Accelerator* accel = recon[i];

      // TODO: Need to see if we actually need reconOrder here or not.
      //       After previous changes, we might get away with only calculating it on the spot. Need to check this.
      reconOrder[i] = CalculateDAGOrder(&accel->allocated,temp);
      reconDelay[i] = CalculateDelay(accel,temp);
      
      Array<DelayToAdd> delaysToAdd = GenerateFixDelays(accel,reconDelay[i].edgesDelay,globalPermanent);
      for(DelayToAdd toAdd : delaysToAdd){
        Edge reconEdge = toAdd.edge;

        PortInstance n0 = MappingMapOutput(reconToMergedAccel[i],reconEdge.units[0]);
        PortInstance n1 = MappingMapInput(reconToMergedAccel[i],reconEdge.units[1]);

        String uniqueName = PushString(globalPermanent,"%.*s_%d_%d",UNPACK_SS(toAdd.bufferName),i,outerIndex);
        FUInstance* buffer = CreateFUInstance(mergedAccel,BasicDeclaration::buffer,uniqueName);
        SetStatic(mergedAccel,buffer);
       
        // TODO: After changing mapping, do not know if we keep reconEdge port or not. 
        InsertUnit(mergedAccel,{n0.inst,reconEdge.units[0].port},PortInstance{n1.inst,reconEdge.units[1].port},PortInstance{buffer,0});
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
  
  size = recon.size;
  
  declInst.fixedDelayCircuit = mergedAccel;
  
  FUDeclaration* decl = RegisterFU(declInst);

  FillDeclarationWithAcceleratorValues(decl,mergedAccel);
  FillDeclarationWithDelayType(decl);

  decl->type = FUDeclarationType_MERGED;

  Array<Hashmap<FUInstance*,int>*> reconToOrder = PushArray<Hashmap<FUInstance*,int>*>(temp,size);
  for(int i = 0; i < reconToOrder.size; i++){
    DAGOrderNodes order = reconOrder[i];
    reconToOrder[i] = PushHashmap<FUInstance*,int>(temp,order.instances.size);

    for(int ii = 0; ii < order.instances.size; ii++){
      reconToOrder[i]->Insert(order.instances[ii],order.order[ii]);
    }
  }

  Array<int> validIndexes = PushArray<int>(temp,actualMergedAmount);

  int index = 0;
  for(int i = 0; i < actualMergedAmount; i++){
    if(lowestLevel[i]){
      validIndexes[index++] = i; 
    }
  }
  validIndexes.size = index;

  int mergeSize = size;

  if(globalOptions.debug){
    for(int i = 0; i < size; i++){
      BLOCK_REGION(temp);
      String fileName = PushString(temp,"finalRecon_%d.dot",i);
      String filePath = PushDebugPath(temp,permanentName,fileName);
      OutputDebugDotGraph(recon[i],fileName);
    }
  }
  
  // At this point we start filling AccelInfo
  decl->info.infos = PushArray<MergePartition>(globalPermanent,mergeSize);
  for(int i = 0; i < mergeSize; i++){
    BLOCK_REGION(temp);

    auto childToTopArr = StartArray<int>(temp);
    for(int j = validIndexes[i]; j != -1 ; j = parents[j]){
      *childToTopArr.PushElem() = j; 
    }
    Array<int> childToTop = EndArray(childToTopArr);

    auto builder = StartString(temp);
    for(int i = childToTop.size - 1; i >= 0 ; i--){
      int index = childToTop[i];
      FUDeclaration* decl = baseCircuitType[index];

      builder->PushString(decl->name);
      if(i != 0){
        builder->PushString("_");
      }
    }
    decl->info.infos[i].name = EndString(globalPermanent,builder);
    
    decl->info.infos[i].info = GenerateInitialInstanceInfo(decl->fixedDelayCircuit,globalPermanent,{});
    AccelInfoIterator iter = StartIteration(&decl->info);
    iter.mergeIndex = i;
    iter.accelName = decl->info.infos[i].name;
    FillInstanceInfo(iter,globalPermanent);

    AcceleratorMapping* mergedAccelToRecon = MappingInvert(reconToMergedAccel[i],globalPermanent);

    for(int index = 0; index < mergedAccel->allocated.Size(); index++){
      FUInstance* ptr = mergedAccel->allocated.Get(index);
      FUInstance* reconNode = MappingMapNode(mergedAccelToRecon,ptr);
      bool mapExists = reconNode != nullptr;
      
      if(reconNode){
        decl->info.infos[i].info[index].baseNodeDelay = reconDelay[i].nodeDelay->GetOrFail(reconNode).value;
        decl->info.infos[i].info[index].localOrder = reconToOrder[i]->GetOrFail(reconNode);
      } else {
        decl->info.infos[i].info[index].baseNodeDelay = 0; // NOTE: Even if they do not belong, this delay is directly inserted into the header file, meaning that for now it's better if we keep everything at zero.
        decl->info.infos[i].info[index].localOrder = 0;
      }
 
      decl->info.infos[i].info[index].baseName = ptr->name;
      decl->info.infos[i].info[index].belongs = true;

      if(ptr->isMergeMultiplexer || ptr->declaration == BasicDeclaration::fixedBuffer){
        // Nothing
      } else if(mapExists){
        FUInstance* originalNode = MappingMapNode(mergedAccelToRecon,ptr);
        Assert(originalNode);

        decl->info.infos[i].info[index].baseName = originalNode->name;
      } else {
        decl->info.infos[i].info[index].belongs = false;
      }
    }

    // We extract the mux configuration from the recon graphs.
    // As long as the recon graphs are correct, this guarantees that any alteration, from merging muxs or whatever we do in the future, still works.
    if(insertedAMultiplexer){
      Accelerator* rec = recon[i];

      auto builder = StartArray<int>(temp);
      for(FUInstance* inst : rec->allocated){
        if(inst->isMergeMultiplexer){
          *builder.PushElem() = inst->allInputs->port;
          Assert(inst->allInputs->next == nullptr);
        }
      }
      Array<int> muxConfigs = EndArray(builder);

      AccelInfoIterator iter = StartIteration(&decl->info);
      iter.mergeIndex = i;
    
      for(AccelInfoIterator it = iter; it.IsValid(); it = it.Next()){
        InstanceInfo* info = it.CurrentUnit();
        if(info->isMergeMultiplexer){
          info->mergePort = muxConfigs[info->muxGroup];
        }
      }
    }
    
    AcceleratorMapping* baseCircuitToMergedAccel = MappingInvert(mergedAccelToBaseCircuit,temp);
    int typeConfigIndex = i;
    int typeIndex = 0;
    while(typeConfigIndex >= types[typeIndex]->MergePartitionSize()){
      typeConfigIndex -= types[typeIndex]->MergePartitionSize();
      typeIndex += 1;
    }
    AcceleratorMapping* baseCircuitToFlatten = MappingCombine(baseCircuitToMergedAccel,mergedAccelToFlatten[typeIndex],temp);
    
    decl->info.infos[i].baseType = types[typeIndex];

    // Need to map from flattened base type to copy of merged circuit (decl.flattenedBaseCircuit).
    decl->info.infos[i].baseTypeFlattenToMergedBaseCircuit = MappingInvert(baseCircuitToFlatten,globalPermanent);

    Set<PortInstance>* flattenedSet = PushSet<PortInstance>(globalPermanent,mergeMultiplexers[typeIndex]->map->inserted);
    for(PortInstance p : mergeMultiplexers[typeIndex]){
      flattenedSet->Insert(MappingMapInput(mergedAccelToBaseCircuit,p));
    }
    decl->info.infos[i].mergeMultiplexers = flattenedSet;
    
    // NOTE: One thing. At this point, the recon graphs should be embedded inside AccelInfo, right?
    decl->info.infos[i].inputDelays = ExtractInputDelays(iter,globalPermanent);
    decl->info.infos[i].outputLatencies = ExtractOutputLatencies(iter,globalPermanent);
  }

  // Need to set the parent decl after registering stuff.
  // TODO: Need to simplify this part in here and inside the register module. Have the accelInfo calculation take parent decl.
  {
  AccelInfoIterator iter = StartIteration(&decl->info);
  for(int i = 0; i < iter.MergeSize(); i++){
    for(AccelInfoIterator it = iter; it.IsValid(); it = it.Next()){
      it.mergeIndex = i;
      it.CurrentUnit()->parent = decl;
    }
  }
  }
    
  decl->staticUnits = CollectStaticUnits(&decl->info,globalPermanent);

  FillAccelInfoFromCalculatedInstanceInfo(&decl->info,decl->fixedDelayCircuit);
  
  return decl;
}

// TODO: Do not even remember what FromStruct even means. Need to figure out and replace with a better name. Code is confusing as it is.
ReconstituteResult ReconstituteGraphFromStruct(Accelerator* merged,TrieSet<PortInstance>* mergedMultiplexers,Accelerator* base,String name,AcceleratorMapping* baseToMerged,FUDeclaration* parentType,Arena* out){
  TEMP_REGION(temp,out);
  Accelerator* recon = CreateAccelerator(name,AcceleratorPurpose_RECON);
 
  MappingCheck(baseToMerged);
  AcceleratorMapping* mergedToBase = MappingInvert(baseToMerged,temp);
  MappingCheck(mergedToBase,merged,base);
 
  for(PortInstance portInst : mergedMultiplexers){
    Assert(portInst.inst->accel->id == merged->id);
    break;
  }

  AcceleratorMapping* accelToRecon = MappingSimple(merged,recon,out);

  Hashmap<FUInstance*,FUInstance*>* instancesAdded = PushHashmap<FUInstance*,FUInstance*>(temp,9999);

  FUDeclaration* muxType = GetTypeByName(STRING("CombMux8")); // TODO: Kind of an hack

  FUInstance* mergedInstance = nullptr;
  for(FUInstance* inst : parentType->fixedDelayCircuit->allocated){
    if(inst->declaration->type == FUDeclarationType_MERGED){
      Assert(!mergedInstance);
      mergedInstance = inst;
    }
  }

  {
    EdgeIterator iter = IterateEdges(merged);

    while(iter.HasNext()){
      Edge edge = iter.Next();

      String inTempName = edge.in.inst->name;
      String outTempName = edge.out.inst->name;
      inTempName.size = mergedInstance->name.size;
      outTempName.size = mergedInstance->name.size;

      Assert(!CompareString(edge.in.inst->name,"TestMergeAfterStruct"));
      Assert(!CompareString(edge.out.inst->name,"TestMergeAfterStruct"));
      
      // Do not add any edge that connects to a  
      if(CompareString(inTempName,mergedInstance->name)){
        continue;
      }
      if(CompareString(outTempName,mergedInstance->name)){
        continue;
      }
      
      PortInstance node0 = edge.units[0];
      PortInstance node1 = edge.units[1];

      if(CompareString(node1.inst->name,"out")){
        continue;
      }
      
      GetOrAllocateResult<FUInstance*> res0 = instancesAdded->GetOrAllocate(node0.inst);
      GetOrAllocateResult<FUInstance*> res1 = instancesAdded->GetOrAllocate(node1.inst);
      
      if(!res0.alreadyExisted){
        String name = node0.inst->name;
        *res0.data = CopyInstance(recon,node0.inst,true,name);
        
        MappingInsertEqualNode(accelToRecon,node0.inst,*res0.data); // We are using equal node here. 
      }

      if(!res1.alreadyExisted){
        String name = node1.inst->name;
        *res1.data = CopyInstance(recon,node1.inst,true,name);

        MappingInsertEqualNode(accelToRecon,node1.inst,*res1.data);
      }
      
      ConnectUnitsIfNotConnected(*res0.data,edge.units[0].port,*res1.data,edge.units[1].port,edge.delay);
    }
  }

  EdgeIterator iter = IterateEdges(base);
  while(iter.HasNext()){
    Edge edgeInst = iter.Next();
    Edge* edge = &edgeInst;

    PortInstance start = MappingMapOutput(baseToMerged,edge->units[0]);
    PortInstance end = MappingMapInput(baseToMerged,edge->units[1]);

    // This might be needed because flatten map does not do a full map.
    //   Because it contains Input and Output units (and maybe some other reasons?)
    if(start.inst == nullptr || end.inst == nullptr){
      continue;
    }

    Array<Edge*> allPaths = GetAllPaths(merged,start,end,temp);

    auto allValidPathsDyn = StartArray<Edge*>(temp);
    for(Edge* p : allPaths){
      bool valid = true;
      FOREACH_LIST(Edge*,edge,p){
        if(edge->in.inst->declaration == muxType){
          if(!mergedMultiplexers->Exists(edge->in)){
            valid = false;
            break;
          }
        }
      }
      
      if(valid){
        *allValidPathsDyn.PushElem() = p;
      }
    }
    Array<Edge*> allValidPaths = EndArray(allValidPathsDyn);
    
    // Replicate exact same path on new accelerator.
    for(Edge* validPath : allValidPaths){
      FOREACH_LIST(Edge*,edgeOnMerged,validPath){
        PortInstance node0 = edgeOnMerged->units[0];
        PortInstance node1 = edgeOnMerged->units[1];
        GetOrAllocateResult<FUInstance*> res0 = instancesAdded->GetOrAllocate(node0.inst);
        GetOrAllocateResult<FUInstance*> res1 = instancesAdded->GetOrAllocate(node1.inst);

        if(!res0.alreadyExisted){
          String name = node0.inst->name;
        
          *res0.data = CopyInstance(recon,node0.inst,true,name);
        
          MappingInsertEqualNode(accelToRecon,node0.inst,*res0.data); // We are using equal node here. 
        }
      
        if(!res1.alreadyExisted){
          String name = node1.inst->name;

          *res1.data = CopyInstance(recon,node1.inst,true,name);

          MappingInsertEqualNode(accelToRecon,node1.inst,*res1.data);
        }
      
        ConnectUnitsIfNotConnected(*res0.data,edgeOnMerged->units[0].port,*res1.data,edgeOnMerged->units[1].port,edgeOnMerged->delay);
      }
    }
  }
  
  ReconstituteResult result = {};
  result.accel = recon;
  result.accelToRecon = accelToRecon;
  
  return result;
}

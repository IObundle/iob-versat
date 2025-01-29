#include "delayCalculation.hpp"

#include "accelerator.hpp"
#include "configurations.hpp"
#include "debug.hpp"
#include "declaration.hpp"
#include "dotGraphPrinting.hpp"
#include "filesystem.hpp"
#include "memory.hpp"
#include "utilsCore.hpp"
#include "versat.hpp"
#include "debugVersat.hpp"
#include "versatSpecificationParser.hpp"

static ConnectionNode* GetConnectionNode(ConnectionNode* head,int port,PortInstance other){
  FOREACH_LIST(ConnectionNode*,con,head){
    if(con->port != port){
      continue;
    }
    if(con->instConnectedTo == other){
      return con;
    }
  }

  return nullptr;
}

static void SendLatencyUpwards(FUInstance* node,Hashmap<Edge,DelayInfo>* delays,Hashmap<FUInstance*,DelayInfo>* nodeDelay,Hashmap<FUInstance*,int>* nodeToPart){
  int b = nodeDelay->GetOrFail(node).value;
  FUInstance* inst = node;
  FUDeclaration* decl = inst->declaration;
  bool multipleTypes = HasMultipleConfigs(decl);

  int nodePart = nodeToPart->GetOrFail(node);

  FOREACH_LIST(ConnectionNode*,info,node->allOutputs){
    FUInstance* other = info->instConnectedTo.inst;

    // Do not set delay for source units. Source units cannot be found in this, otherwise they wouldn't be source
    Assert(other->type != NodeType_SOURCE);

    int a = 0;

    if(multipleTypes){
      a = decl->info.infos[nodePart].outputLatencies[info->port];
    } else {
      a = inst->declaration->GetOutputLatencies()[info->port];
    }
    
    int e = info->edgeDelay;

    FOREACH_LIST(ConnectionNode*,otherInfo,other->allInputs){
      int c = 0;
      if(HasMultipleConfigs(other->declaration)){
        int otherPart = nodeToPart->GetOrFail(other);
        
        c = other->declaration->info.infos[otherPart].inputDelays[info->instConnectedTo.port];
      } else {
        c = other->declaration->GetInputDelays()[info->instConnectedTo.port];
      }

      if(info->instConnectedTo.port == otherInfo->port &&
         otherInfo->instConnectedTo.inst == inst && otherInfo->instConnectedTo.port == info->port){
        
        int delay = b + a + e - c;

        Edge edge = {};
        edge.out = {node,info->port};
        edge.in = {other,info->instConnectedTo.port};

        if(node->declaration == BasicDeclaration::buffer){
          *otherInfo->delay.value = delay;
          otherInfo->delay.isAny = true;

          delays->GetOrFail(edge).isAny = true;
        } else {
          *otherInfo->delay.value = delay;
        }
      }
    }
  }
}

struct AccelEdgeIterator{
  AccelInfoIterator iter;
  int edgeIndex;
  int totalEdgeIndex;
};

bool IsValid(AccelEdgeIterator& iter){
  return iter.iter.IsValid();
}

void Advance(AccelEdgeIterator& iter){
  InstanceInfo* info = iter.iter.CurrentUnit();
  Array<SimplePortConnection> inputs = info->inputs;

  iter.totalEdgeIndex += 1;
  
  if(iter.edgeIndex + 1 < inputs.size){
    iter.edgeIndex += 1;
    return;
  }

  for(iter.iter = iter.iter.Next(); iter.iter.IsValid(); iter.iter = iter.iter.Next()){
    InstanceInfo* info = iter.iter.CurrentUnit();
    Array<SimplePortConnection> inputs = info->inputs;
    if(inputs.size > 0){
      iter.edgeIndex = 0;
      return;
      break;
    }
  }
}

AccelEdgeIterator IterateEdges(AccelInfoIterator iter){
  AccelEdgeIterator edgeIter = {};
  edgeIter.iter = iter;
  
  InstanceInfo* initial = iter.CurrentUnit();
  if(initial->inputs.size == 0){
    Advance(edgeIter);
  }

  edgeIter.totalEdgeIndex = 0;
  
  return edgeIter;
}

SimpleEdge Get(AccelEdgeIterator iter){
  Assert(IsValid(iter));

  InstanceInfo* info = iter.iter.CurrentUnit();

  Array<SimplePortConnection> inputs = info->inputs;
  SimplePortConnection conn = inputs[iter.edgeIndex];

  int inputIndex = iter.iter.GetIndex();
  
  SimpleEdge res = {};
  res.outPort = conn.outPort;
  res.outIndex = conn.outInst;
  res.inPort = conn.inPort;
  res.inIndex = inputIndex;

  return res;
}

// TODO: We probably want to remove this 
static ConnectionNode* GetConnectionNode(SimpleEdge edge,AccelInfoIterator top){
  InstanceInfo* out = top.GetUnit(edge.outIndex);
  InstanceInfo* in = top.GetUnit(edge.inIndex);

  FOREACH_LIST(ConnectionNode*,ptr,out->inst->allOutputs){
    if(ptr->instConnectedTo.inst == in->inst && ptr->instConnectedTo.port == edge.inPort &&
       ptr->port == edge.outPort){
      return ptr;
    }
  }

  Assert(false);
  return nullptr;
}

// TODO: I should give the codebase a comb through and start normalizing this stuff. There is some confusion caused by repeated names for things that are not equal.
// Naming conventions -
// Latency - number of cycles it takes for node/edge to produce valid data.
// Delay - additional cycles of waiting added to nodes/edges to guarantee that valid data arrives at the same time.
//         delay in a edge will eventually be resolved by adding fixed buffers to the data path.
// Global vs local - Global values are values that apply to the entire graph and subgraphs, while local only applies to the current graph. It can also be used to represent the different between values that only make sense in a graph subset versues the entire graph.
// 

GraphPrintingContent GenerateLatencyDotGraph(AccelInfoIterator top,Array<int> orderToIndex,Array<DelayInfo> nodeDelay,Array<int> edgeDelay,Arena* out,Arena* temp){
  BLOCK_REGION(temp);

  int size = orderToIndex.size;
  Array<GraphPrintingNodeInfo> nodeArray = PushArray<GraphPrintingNodeInfo>(out,size);
  for(int i = 0; i < size; i++){
    InstanceInfo* info = top.GetUnit(orderToIndex[i]);
    FUInstance* node = info->inst;
    
    nodeArray[i].name = PushString(out,node->name);
    nodeArray[i].content = PushString(out,"%.*s:%d:%d",UNPACK_SS(node->name),nodeDelay[i].value,info->special);
    nodeArray[i].color = Color_BLACK;
  }

  int totalEdges = 0;
  for(AccelEdgeIterator iter = IterateEdges(top); IsValid(iter); Advance(iter)){
    totalEdges += 1;
  }

  Array<GraphPrintingEdgeInfo> edgeArray = PushArray<GraphPrintingEdgeInfo>(out,totalEdges);
  int edgeIndex = 0;
  for(AccelEdgeIterator iter = IterateEdges(top); IsValid(iter); Advance(iter),edgeIndex += 1){
    int edgeLatency = edgeDelay[edgeIndex];

    SimpleEdge edge = Get(iter);

    edgeArray[edgeIndex].color = Color_BLACK;
    edgeArray[edgeIndex].content = PushString(out,"%d",edgeLatency);
    edgeArray[edgeIndex].first = nodeArray[top.GetUnit(edge.outIndex)->localOrder].name;
    edgeArray[edgeIndex].second = nodeArray[top.GetUnit(edge.inIndex)->localOrder].name;
  }
    
  GraphPrintingContent result = {};
  result.edges = edgeArray;
  result.nodes = nodeArray;
  result.graphLabel = STRING("Nodes and edges contain their global latency (nodes also contain special)");
  
  return result;
}

SimpleCalculateDelayResult CalculateDelay(AccelInfoIterator top,Arena* out,Arena* temp){
  int amountOfNodes = 0;
  for(AccelInfoIterator iter = top; iter.IsValid(); iter = iter.Next()){
    amountOfNodes += 1;
  }

  // TODO: There is a lot of repeated code that could be simplified, altought still need to check if we are using a good approach when it comes to the merges later on.
  //       Furthermore, there is a lot of confusion from mixing latency vs delays, especially because we are currently using the same array to calculate delays and latencies. Maybe it would be best to split latency calculation and delay calculation into two functions and to make sure that we have the calculate delay result struct having the correct names for things (we can still return everything and let the top level code use the data as it sees fit).

  // TODO: None of this code should depend on FUInstance or FUDeclaration. Only on InstanceInfo direct members.
  
  // Keyed by order
  Array<DelayInfo> nodeDelayArray = PushArray<DelayInfo>(out,amountOfNodes);
  Memset(nodeDelayArray,{});
  
  Array<int> orderToIndex = PushArray<int>(temp,amountOfNodes);
  for(AccelInfoIterator iter = top; iter.IsValid(); iter = iter.Next()){
    int index = iter.GetIndex();
    InstanceInfo* info = iter.CurrentUnit();
    orderToIndex[info->localOrder] = index;
  }
  
  int totalEdges = 0;
  for(AccelEdgeIterator iter = IterateEdges(top); IsValid(iter); Advance(iter)){
    totalEdges += 1;
  }

  Array<int> edgesDelays = PushArray<int>(out,totalEdges);

  // Sets latency for each edge of the node 
  auto SendLatencyUpwards = [&top,&edgesDelays,&nodeDelayArray,&orderToIndex](int orderIndex){
    int trueIndex = orderToIndex[orderIndex];
    InstanceInfo* info = top.GetUnit(trueIndex);
    DelayInfo b = nodeDelayArray[orderIndex]; 
    int edgeIndex = 0;
    for(AccelEdgeIterator iter = IterateEdges(top); IsValid(iter); Advance(iter),edgeIndex += 1){
      SimpleEdge edge = Get(iter);

      if(edge.outIndex != trueIndex){
        continue;
      }

      int otherIndex = edge.inIndex;
      InstanceInfo* otherInfo = top.GetUnit(otherIndex);
      
      int a = info->outputLatencies[edge.outPort];

      int d = 0;
      if(info->decl == BasicDeclaration::fixedBuffer){
        d = info->special;
      }
      
      ConnectionNode* conn = GetConnectionNode(edge,top);
      
      int e = conn->edgeDelay;
      
      int c = otherInfo->inputDelays[edge.inPort];
      int delay = b.value + a + e - c + d;

      edgesDelays[edgeIndex] = delay;
    }
  };
  
  // Start at sources
  int orderIndex = 0;
  for(; orderIndex < orderToIndex.size; orderIndex++){
    FUInstance* node = top.GetUnit(orderToIndex[orderIndex])->inst;

    int maxInputEdgeLatency = 0;
    int edgeIndex = 0;
    for(AccelEdgeIterator iter = IterateEdges(top); IsValid(iter); Advance(iter),edgeIndex += 1){
      SimpleEdge edge = Get(iter);
      int inIndex = edge.inIndex;

      InstanceInfo* inInfo = top.GetUnit(inIndex);
      FUInstance* inNode = inInfo->inst;
      if(inNode != node){
        continue;
      }

      int edgeLatency = edgesDelays[edgeIndex];
      maxInputEdgeLatency = std::max(maxInputEdgeLatency,edgeLatency);
    }
    
    nodeDelayArray[orderIndex].value = maxInputEdgeLatency;
    nodeDelayArray[orderIndex].isAny = false;

    // Send latency upwards.
    SendLatencyUpwards(orderIndex);
  }

  if(globalOptions.debug){
    static int funCalls = 0;
    BLOCK_REGION(out);

    String fileName = {};
    if(Empty(top.accelName)){
      fileName = PushString(out,"global_latency_%d_%d.dot",funCalls++,top.mergeIndex);
    } else {
      fileName = PushString(out,"global_latency_%.*s_%d.dot",UNPACK_SS(top.accelName),top.mergeIndex);
    }
    
    String filePath = PushDebugPath(out,STRING("TEST"),STRING("delays"),fileName);

    GraphPrintingContent content = GenerateLatencyDotGraph(top,orderToIndex,nodeDelayArray,edgesDelays,temp,out);
    
    String result = GenerateDotGraph(content,out,debugArena);
    OutputContentToFile(filePath,result);
  }

  // This is still the global latency per port. I think it's good
  Array<Array<int>> inputPortDelayByOrder = PushArray<Array<int>>(out,orderToIndex.size);
  
  for(int i = 0; i < orderToIndex.size; i++){
    FUInstance* node = top.GetUnit(orderToIndex[i])->inst;

    int maxPortIndex = -1;
    int edgeIndex = 0;
    for(AccelEdgeIterator iter = IterateEdges(top); IsValid(iter); Advance(iter),edgeIndex += 1){
      SimpleEdge edge = Get(iter);
      int inIndex = edge.inIndex;

      InstanceInfo* inInfo = top.GetUnit(inIndex);
      FUInstance* inNode = inInfo->inst;
      if(inNode != node){
        continue;
      }

      maxPortIndex = std::max(maxPortIndex,edge.inPort);
    }

    if(maxPortIndex == -1){
      inputPortDelayByOrder[i] = {};
      continue;
    }
    inputPortDelayByOrder[i] = PushArray<int>(out,maxPortIndex + 1);

    edgeIndex = 0;
    for(AccelEdgeIterator iter = IterateEdges(top); IsValid(iter); Advance(iter),edgeIndex += 1){
      SimpleEdge edge = Get(iter);
      int inIndex = edge.inIndex;

      InstanceInfo* inInfo = top.GetUnit(inIndex);
      FUInstance* inNode = inInfo->inst;
      if(inNode != node){
        continue;
      }

      inputPortDelayByOrder[i][edge.inPort] = edgesDelays[edgeIndex]; 
    }
  }
  
  // Store latency on data consuming units
  for(int i = 0; i < orderToIndex.size; i++){
    FUInstance* node = top.GetUnit(orderToIndex[i])->inst;

    if(node->type != NodeType_SINK){
      continue;
    }

    // For each edge in that contains that node as an output
    int minEdgeDelay = 9999;
    int edgeIndex = 0;
    for(AccelEdgeIterator iter = IterateEdges(top); IsValid(iter); Advance(iter),edgeIndex += 1){
      SimpleEdge edge = Get(iter);
      int inIndex = edge.inIndex;

      InstanceInfo* inInfo = top.GetUnit(inIndex);
      FUInstance* inNode = inInfo->inst;
      if(inNode != node){
        continue;
      }

      int edgeDelay = edgesDelays[edgeIndex];
      minEdgeDelay = std::min(minEdgeDelay,edgeDelay);
    }

    // Is this even possible?
    Assert(minEdgeDelay != 9999);

    nodeDelayArray[i].value = minEdgeDelay;
  }

  // We have the global latency of each node and edge.
  // We now need to calculate the "extra" latency added to each edge in order to align everything together.

  // Converts global latency into edge delays
  for(int i = 0; i < orderToIndex.size; i++){
    FUInstance* node = top.GetUnit(orderToIndex[i])->inst;

    int nodeDelay = nodeDelayArray[i].value;
    
    int minEdgeDelay = 9999;
    int edgeIndex = 0;
    for(AccelEdgeIterator iter = IterateEdges(top); IsValid(iter); Advance(iter),edgeIndex += 1){
      SimpleEdge edge = Get(iter);
      int inIndex = edge.inIndex;

      InstanceInfo* inInfo = top.GetUnit(inIndex);
      FUInstance* inNode = inInfo->inst;
      if(inNode != node){
        continue;
      }

      edgesDelays[edgeIndex] = nodeDelay - edgesDelays[edgeIndex];
      
      int edgeDelay = edgesDelays[edgeIndex];
      minEdgeDelay = std::min(minEdgeDelay,edgeDelay);
    }

    if(minEdgeDelay == 9999){
      continue;
    }

    edgeIndex = 0;
    for(AccelEdgeIterator iter = IterateEdges(top); IsValid(iter); Advance(iter),edgeIndex += 1){
      SimpleEdge edge = Get(iter);
      int inIndex = edge.inIndex;

      InstanceInfo* inInfo = top.GetUnit(inIndex);
      FUInstance* inNode = inInfo->inst;
      if(inNode != node){
        continue;
      }

      edgesDelays[edgeIndex] -= minEdgeDelay;
    }
  }

  if(globalOptions.debug){
    static int funCalls = 0;
    BLOCK_REGION(out);

    String fileName = {};
    if(Empty(top.accelName)){
      fileName = PushString(out,"edge_delays_%d_%d.dot",funCalls++,top.mergeIndex);
    } else {
      fileName = PushString(out,"edge_delays_%.*s_%d.dot",UNPACK_SS(top.accelName),top.mergeIndex);
    }
    
    String filePath = PushDebugPath(out,STRING("TEST"),STRING("delays"),fileName);

    GraphPrintingContent content = GenerateLatencyDotGraph(top,orderToIndex,nodeDelayArray,edgesDelays,temp,out);
    
    String result = GenerateDotGraph(content,out,debugArena);
    OutputContentToFile(filePath,result);
  }
  
  // Store delays on data producing units
  for(int i = 0; i < orderToIndex.size; i++){
    FUInstance* node = top.GetUnit(orderToIndex[i])->inst;

    if(node->type != NodeType_SOURCE && node->type != NodeType_SOURCE_AND_SINK){
      continue;
    }

    // For each edge in that contains that node as an output
    int minEdgeDelay = 9999;
    int edgeIndex = 0;
    for(AccelEdgeIterator iter = IterateEdges(top); IsValid(iter); Advance(iter),edgeIndex += 1){
      SimpleEdge edge = Get(iter);
      int outIndex = edge.outIndex;

      InstanceInfo* outInfo = top.GetUnit(outIndex);
      FUInstance* outNode = outInfo->inst;
      if(outNode != node){
        continue;
      }

      int edgeDelay = edgesDelays[edgeIndex];
      minEdgeDelay = std::min(minEdgeDelay,edgeDelay);
    }

    // Is this even possible?
    Assert(minEdgeDelay != 9999);

    nodeDelayArray[i].value = minEdgeDelay;
    
    edgeIndex = 0;
    for(AccelEdgeIterator iter = IterateEdges(top); IsValid(iter); Advance(iter),edgeIndex += 1){
      SimpleEdge edge = Get(iter);
      int outIndex = edge.outIndex;

      InstanceInfo* outInfo = top.GetUnit(outIndex);
      FUInstance* outNode = outInfo->inst;
      if(outNode != node){
        continue;
      }

      edgesDelays[edgeIndex] -= minEdgeDelay;
    }
  }

  if(globalOptions.debug){
    static int funCalls = 0;
    BLOCK_REGION(out);
    
    String fileName = {};
    if(Empty(top.accelName)){
      fileName = PushString(out,"final_delays_%d.dot",funCalls++);
    } else {
      fileName = PushString(out,"final_delays_%.*s.dot",UNPACK_SS(top.accelName));
    }

    String filePath = PushDebugPath(out,STRING("TEST"),STRING("delays"),fileName);

    GraphPrintingContent content = GenerateLatencyDotGraph(top,orderToIndex,nodeDelayArray,edgesDelays,temp,out);
    
    String result = GenerateDotGraph(content,out,debugArena);
    OutputContentToFile(filePath,result);
  }
  
  SimpleCalculateDelayResult res = {};
  res.nodeDelayByOrder = nodeDelayArray;
  res.edgesDelay = edgesDelays;
  res.inputPortDelayByOrder = inputPortDelayByOrder;
  // TODO: Missing pushing delays towards inputs and pushing delays towards outputs.

  return res;
}

// The problem:
// Each node adds some latency to the computation path. This is not avoidable.
// Nodes require all the paths to have the same latency to work.
// We work by picking the latency of a node, add the lantency of the edge and then we set the latency of the input node as the maximum of the latencies of each path.
// If we are working with latencies until the very end, we could have a way of transposing latencies from one side to the other. We can increase the latency of a node, which increases the latency of the inputs and decreases the latency of the outputs by the same amount.

CalculateDelayResult CalculateDelay(Accelerator* accel,DAGOrderNodes order,Array<Partition> partitions,Arena* out){
  // TODO: We are currently using the delay pointer inside the ConnectionNode structure. When correct, eventually change to just use the hashmap or an array.
  static int functionCalls = 0;
  
  int nodes = accel->allocated.Size();
  int edges = 9999; // Size(accel->edges);
  EdgeDelay* edgeToDelay = PushHashmap<Edge,DelayInfo>(out,edges);
  NodeDelay* nodeDelay = PushHashmap<FUInstance*,DelayInfo>(out,nodes);
  PortDelay* portDelay = PushHashmap<PortInstance,DelayInfo>(out,edges);
  
  CalculateDelayResult res = {};
  res.edgesDelay = edgeToDelay;
  res.nodeDelay = nodeDelay;
  res.portDelay = portDelay;
  
  for(FUInstance* ptr : accel->allocated){
    nodeDelay->Insert(ptr,{0,false});
  }
  
  EdgeIterator iter = IterateEdges(accel);
  while(iter.HasNext()){
    Edge edge = iter.Next();

    ConnectionNode* fromOut = GetConnectionNode(edge.out.inst->allOutputs,edge.out.port,edge.in);
    ConnectionNode* fromIn = GetConnectionNode(edge.in.inst->allInputs,edge.in.port,edge.out);

    int* delayPtr = &edgeToDelay->Insert(edge,{0,false})->value;
    
    fromOut->delay.value = delayPtr;
    fromIn->delay.value = delayPtr;
  }
  
  Hashmap<FUInstance*,int>* nodeToPart = PushHashmap<FUInstance*,int>(out,nodes); // TODO: Either temp or move out accross call stack
  {
    int partitionIndex = 0;
    for(int index = 0; index < order.instances.size; index++){
      FUInstance* node = order.instances[index];
    
      int part = 0;
      if(HasMultipleConfigs(node->declaration) && partitions.size > 0){
        part = partitions[partitionIndex++].value;
      }

      nodeToPart->Insert(node,part);
    }
  }

  // Used by debug code to print graphs but nothing else, I think
  {
    for(int i = 0; i < order.instances.size; i++){
      FUInstance* node = order.instances[i];
      
      FOREACH_LIST(ConnectionNode*,info,node->allOutputs){
        FUInstance* other = info->instConnectedTo.inst;
        PortInstance node = {.inst = other,.port = info->instConnectedTo.port};
        portDelay->Insert(node,{0,false});
      }
    }
  }
 
  // Start at sources
  int graphs = 0;
  int index = 0;
  int partitionIndex = 0;
  for(; index < order.instances.size; index++){
    FUInstance* node = order.instances[index];

    int part = 0;
    if(HasMultipleConfigs(node->declaration) && partitions.size > 0){
      part = partitions[partitionIndex++].value;
    }

    if(node->type != NodeType_SOURCE && node->type != NodeType_SOURCE_AND_SINK){
      continue;
    }

    nodeDelay->Insert(node,{0,false});
    SendLatencyUpwards(node,edgeToDelay,nodeDelay,nodeToPart);

    if(globalOptions.debug){
      BLOCK_REGION(out);
      String fileName = PushString(out,"1_%d_out1_%d.dot",functionCalls,graphs++);
      String filePath = PushDebugPath(out,accel->name,STRING("delays"),fileName);
      
      GraphPrintingContent content = GenerateDelayDotGraph(accel,res,out,debugArena);
      String result = GenerateDotGraph(content,out,debugArena);
      OutputContentToFile(filePath,result);
    }
  }

  index = 0;
  partitionIndex = 0;
  // Continue up the tree
  for(; index < order.instances.size; index++){
    FUInstance* node = order.instances[index];

    int part = 0;
    if(HasMultipleConfigs(node->declaration) && partitions.size > 0){
      part = partitions[partitionIndex++].value;
    }

    if(node->type == NodeType_UNCONNECTED
       || node->type == NodeType_SOURCE){
      continue;
    }

    int maximum = -(1 << 30);
    FOREACH_LIST(ConnectionNode*,info,node->allInputs){
      //if(!info->delay.isAny){
        maximum = std::max(maximum,*info->delay.value);
      //}
    }
    
    if(maximum == -(1 << 30)){
      continue;
    }
    
    FOREACH_LIST(ConnectionNode*,info,node->allInputs){
      if(info->delay.isAny){
        //*info->delay.value = maximum - *info->delay.value;
        *info->delay.value = 0;
      } else {
        *info->delay.value = maximum - *info->delay.value;
      }
    }

    nodeDelay->Insert(node,{maximum,false});

    if(node->type != NodeType_SOURCE_AND_SINK){
      SendLatencyUpwards(node,edgeToDelay,nodeDelay,nodeToPart);
    }

    if(globalOptions.debug){
      BLOCK_REGION(out);
      String fileName = PushString(out,"1_%d_out2_%d.dot",functionCalls,graphs++);
      String filePath = PushDebugPath(out,accel->name,STRING("delays"),fileName);
      
      GraphPrintingContent content = GenerateDelayDotGraph(accel,res,out,debugArena);
      String result = GenerateDotGraph(content,out,debugArena);
      OutputContentToFile(filePath,result);
    }
  }

  for(int i = 0; i < order.instances.size; i++){
    FUInstance* node = order.instances[i];

    if(node->type != NodeType_SOURCE_AND_SINK){
      continue;
    }

    // Source_and_sink units never have output delay. They can't
    FOREACH_LIST(ConnectionNode*,con,node->allOutputs){
      *con->delay.value = 0;
    }
  }

  int minimum = 0;
  for(int i = 0; i < order.instances.size; i++){
    FUInstance* node = order.instances[i];
    int delay = nodeDelay->GetOrFail(node).value;

    minimum = std::min(minimum,delay);
  }
  for(int i = 0; i < order.instances.size; i++){
    FUInstance* node = order.instances[i];
    int* delay = &(nodeDelay->Get(node)->value);
    *delay -= minimum;
  }
  
  if(globalOptions.debug){
    BLOCK_REGION(out);
    String fileName = PushString(out,"1_%d_out3_%d.dot",functionCalls,graphs++);
    String filePath = PushDebugPath(out,accel->name,STRING("delays"),fileName);
      
    GraphPrintingContent content = GenerateDelayDotGraph(accel,res,out,debugArena);
    String result = GenerateDotGraph(content,out,debugArena);
    OutputContentToFile(filePath,result);
  }

  if(!globalOptions.disableDelayPropagation){
    // Normalizes everything to start on zero
    for(int i = 0; i < order.instances.size; i++){
      FUInstance* node = order.instances[i];

      if(node->type != NodeType_SOURCE && node->type != NodeType_SOURCE_AND_SINK){
        break;
      }

      
      int minimum = 1 << 30;
      FOREACH_LIST(ConnectionNode*,info,node->allOutputs){
        minimum = std::min(minimum,*info->delay.value);
      }

      if(minimum == 1 << 30){
        continue;
      }
      
      // Does not take into account unit latency
      nodeDelay->Insert(node,{minimum,false});

      FOREACH_LIST(ConnectionNode*,info,node->allOutputs){
        *info->delay.value -= minimum;
      }
    }
  }

  if(globalOptions.debug){
    BLOCK_REGION(out);
    String fileName = PushString(out,"1_%d_out4_%d.dot",functionCalls,graphs++);
    String filePath = PushDebugPath(out,accel->name,STRING("delays"),fileName);
      
    GraphPrintingContent content = GenerateDelayDotGraph(accel,res,out,debugArena);
    String result = GenerateDotGraph(content,out,debugArena);
    OutputContentToFile(filePath,result);
  }

  for(int i = 0; i < order.instances.size; i++){
    FUInstance* node = order.instances[i];

    if(node->type == NodeType_UNCONNECTED){
      nodeDelay->Insert(node,{0,false});
    }
  }

  for(auto p :res.nodeDelay){
    Assert(p.second->value >= 0);
  }

  for(int i = 0; i < order.instances.size; i++){
    FUInstance* node = order.instances[i];

    if(node->type != NodeType_SOURCE && node->type != NodeType_SOURCE_AND_SINK){
      continue;
    }

    int minDelay = 9999999;
    FOREACH_LIST(ConnectionNode*,info,node->allOutputs){
      minDelay = std::min(minDelay,*info->delay.value);
    }

    FOREACH_LIST(ConnectionNode*,info,node->allOutputs){
      *info->delay.value -= minDelay;
    }

    nodeDelay->Get(node)->value += minDelay;    
  }

#if 0
  for(int i = order.instances.size - 1; i >= 0; i--){
    FUInstance* inst = order.instances[i];

    int outputMin = 9999;
    int outputMax = 0;
    bool containsAny = false;
    
    FOREACH_LIST(ConnectionNode*,info,inst->allOutputs){
      containsAny |= info->delay.isAny;
      outputMin = std::min(outputMin,*info->delay.value);
      outputMax = std::max(outputMax,*info->delay.value);
    }

    if(containsAny){
      continue;
    }
   
    if(outputMin == outputMax){
      FOREACH_LIST(ConnectionNode*,info,inst->allOutputs){
        *info->delay.value -= outputMin;
      }
      FOREACH_LIST(ConnectionNode*,info,inst->allInputs){
        *info->delay.value += outputMax;
      }

      nodeDelay->Get(inst)->value += outputMin;
    }
  }

  if(globalOptions.debug){
    BLOCK_REGION(out);
    String fileName = PushString(out,"1_%d_out5_%d.dot",functionCalls,graphs++);
    String filePath = PushDebugPath(out,accel->name,STRING("delays"),fileName);
      
    GraphPrintingContent content = GenerateDelayDotGraph(accel,res,out,debugArena);
    String result = GenerateDotGraph(accel,content,out,debugArena);
    OutputContentToFile(filePath,result);
  }
#endif
  
  {
    for(int i = 0; i < order.instances.size; i++){
      FUInstance* node = order.instances[i];
      FUInstance* inst = node;
      int b = nodeDelay->GetOrFail(node).value;
      FUDeclaration* decl = inst->declaration;
      bool multipleTypes = HasMultipleConfigs(decl);

      int nodePart = nodeToPart->GetOrFail(node);
      
      FOREACH_LIST(ConnectionNode*,info,node->allOutputs){
        FUInstance* other = info->instConnectedTo.inst;

        int a = 0;

        if(multipleTypes){
          a = decl->info.infos[nodePart].outputLatencies[info->port];
        } else {
          a = inst->declaration->GetOutputLatencies()[info->port];
        }
    
        int e = info->edgeDelay;

        FOREACH_LIST(ConnectionNode*,otherInfo,other->allInputs){
          int c = 0;
          if(HasMultipleConfigs(other->declaration)){
            int otherPart = nodeToPart->GetOrFail(other);

            c = other->declaration->info.infos[otherPart].inputDelays[info->instConnectedTo.port];
          } else {
            c = other->declaration->GetInputDelays()[info->instConnectedTo.port];
          }

          if(info->instConnectedTo.port == otherInfo->port &&
             otherInfo->instConnectedTo.inst == inst && otherInfo->instConnectedTo.port == info->port){
        
            int delay = b + a + e - c;

            PortInstance node = {.inst = other,.port = info->instConnectedTo.port};
            portDelay->Insert(node,{delay,false});
          }
        }
      }
    }
  }

  if(globalOptions.debug){
    BLOCK_REGION(out);
    String fileName = PushString(out,"1_%d_out_final.dot",functionCalls);
    String filepath = PushDebugPath(out,accel->name,STRING("delays"),fileName);

    GraphPrintingContent content = GenerateDelayDotGraph(accel,res,out,debugArena);
    String result = GenerateDotGraph(content,out,debugArena);
    OutputContentToFile(filepath,result);
  }
  
  functionCalls += 1;
  
  return res;
}

CalculateDelayResult CalculateDelay(Accelerator* accel,DAGOrderNodes order,Arena* out){
  Array<Partition> part = {};
  return CalculateDelay(accel,order,part,out);
}

GraphPrintingContent GenerateDelayDotGraph(Accelerator* accel,CalculateDelayResult delayInfo,Arena* out,Arena* temp){
  BLOCK_REGION(temp);

  auto NodeWithDelayContent = [&](FUInstance* node,Arena* out) -> GraphInfo{
    int delay = delayInfo.nodeDelay->GetOrFail(node).value;
    String content = PushString(out,"%.*s:%d",UNPACK_SS(node->name),delay);

    return {content,DefaultNodeColor(node)};
  };

  auto EdgeWithDelayContent = [&](Edge* edge,Arena* out) -> GraphInfo{
    int inPort = edge->in.port;
    int outPort = edge->out.port;

    int portDelay = delayInfo.portDelay->GetOrFail(edge->in).value;
    
    int edgeBaseDelay = edge->delay;

    DelayInfo delay = delayInfo.edgesDelay->GetOrFail(*edge);
    int edgeDelay = delay.value;
    
    int edgeOutput = edge->out.inst->declaration->GetOutputLatencies()[edge->out.port];
    int edgeInput = edge->in.inst->declaration->GetInputDelays()[edge->in.port];
    
    String content = PushString(out,"%d->%d (%d/%d/%d) %d [%d]",outPort,inPort,edgeOutput,edgeBaseDelay,edgeInput,portDelay,edgeDelay);
    String first = edge->out.inst->name;
    String second = edge->in.inst->name; 

    Color color = Color_BLACK;
    if(delay.isAny){
     color = Color_RED;
    }

    return {content,color};
  };

  GraphPrintingContent result = GeneratePrintingContent(accel,NodeWithDelayContent,EdgeWithDelayContent,out,temp);
  result.graphLabel = STRING("Nodes contain their global latency after the \':\'\nEdge Format: OutPort -> InPort (<Output Latency>/<Edge delay>/<Input Delay>) <Without buffer, node lantency + edge value>  [<Buffer value, difference between without buffer and input node latency>]\nImagine reading from the output node into the input node: Node latency + edge values equals latency which we must fix by inserting buffer of value N");
  
  return result;
}




























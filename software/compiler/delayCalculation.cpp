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

        if(HasVariableDelay(node->declaration)){
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

GraphPrintingContent GenerateLatencyDotGraph(AccelInfoIterator top,Array<int> orderToIndex,Array<DelayInfo> nodeDelay,Array<DelayInfo> edgeDelay,Arena* out,Arena* temp){
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
    DelayInfo edgeLatency = edgeDelay[edgeIndex];

    SimpleEdge edge = Get(iter);

    if(edgeLatency.isAny){
      edgeArray[edgeIndex].color = Color_BLUE;
    } else {
      edgeArray[edgeIndex].color = Color_BLACK;
    }
    edgeArray[edgeIndex].content = PushString(out,"%d",edgeLatency.value);
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
  Assert(!Empty(top.accelName));

  int amountOfNodes = 0;
  for(AccelInfoIterator iter = top; iter.IsValid(); iter = iter.Next()){
    amountOfNodes += 1;
  }

  // TODO: There is a lot of repeated code that could be simplified, altought still need to check if we are using a good approach when it comes to the merges later on.
  //       Furthermore, there is a lot of confusion from mixing latency vs delays, especially because we are currently using the same array to calculate delays and latencies. Maybe it would be best to split latency calculation and delay calculation into two functions and to make sure that we have the calculate delay result struct having the correct names for things (we can still return everything and let the top level code use the data as it sees fit).
  // TODO: Separate latency calculation from delay calculation, even if currently it seems fine, it is hard to reason about.
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

  Array<int> edgeDelay = PushArray<int>(temp,totalEdges);
  // Need to replace this with a DelayInfo array for the edges
  int edgeIndex = 0;
  for(AccelEdgeIterator iter = IterateEdges(top); IsValid(iter); Advance(iter),edgeIndex += 1){
    SimpleEdge edge = Get(iter);
    ConnectionNode* conn = GetConnectionNode(edge,top);
    edgeDelay[edgeIndex] = conn->edgeDelay;
  }
  
  Array<DelayInfo> edgesDelays = PushArray<DelayInfo>(out,totalEdges);

  // Sets latency for each edge of the node 
  auto SendLatencyUpwards = [&top,&edgesDelays,&nodeDelayArray,&orderToIndex,&edgeDelay](int orderIndex){
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
      
      // Need to replace this with a DelayInfo array for the edges
      //ConnectionNode* conn = GetConnectionNode(edge,top);

      int e = edgeDelay[edgeIndex]; //conn->edgeDelay;
      
      int c = otherInfo->inputDelays[edge.inPort];
      int delay = b.value + a + e - c + d;

      edgesDelays[edgeIndex].value = delay;

      // If the node is a buffer, delays are now variable.
      // We want to preserve this information as much as possible. Even if not needed because the merge is simple, we might be able to unlock some optimizations down the line
      if(HasVariableDelay(info->decl)){
        edgesDelays[edgeIndex].isAny = true;
      }
      
      edgesDelays[edgeIndex].isAny |= b.isAny;
    }
  };
  
  // Start at sources
  for(int orderIndex = 0; orderIndex < orderToIndex.size; orderIndex++){
    FUInstance* node = top.GetUnit(orderToIndex[orderIndex])->inst;

    int maxInputEdgeLatency = 0;
    int edgeIndex = 0;
    bool allAny = true;
    for(AccelEdgeIterator iter = IterateEdges(top); IsValid(iter); Advance(iter),edgeIndex += 1){
      SimpleEdge edge = Get(iter);
      int inIndex = edge.inIndex;

      InstanceInfo* inInfo = top.GetUnit(inIndex);
      FUInstance* inNode = inInfo->inst;
      if(inNode != node){
        continue;
      }

      int edgeLatency = edgesDelays[edgeIndex].value;
      maxInputEdgeLatency = std::max(maxInputEdgeLatency,edgeLatency);
      allAny &= edgesDelays[edgeIndex].isAny;
    }
    
    nodeDelayArray[orderIndex].value = maxInputEdgeLatency;
    nodeDelayArray[orderIndex].isAny = (maxInputEdgeLatency != 0 && allAny);

    // Send latency upwards.
    if(node->type != NodeType_SOURCE_AND_SINK){
      SendLatencyUpwards(orderIndex);
    }
  }

  if(globalOptions.debug){
    static int funCalls = 0;
    BLOCK_REGION(out);

    String fileName = PushString(out,"global_latency_%.*s_%d_%d.dot",UNPACK_SS(top.accelName),top.mergeIndex,funCalls++);
    String filePath = PushDebugPath(out,top.accelName,STRING("delays"),fileName);

    GraphPrintingContent content = GenerateLatencyDotGraph(top,orderToIndex,nodeDelayArray,edgesDelays,temp,out);
    
    String result = GenerateDotGraph(content,out,debugArena);
    OutputContentToFile(filePath,result);
  }

  // This is still the global latency per port. I think it's good
  Array<Array<DelayInfo>> inputPortDelayByOrder = PushArray<Array<DelayInfo>>(out,orderToIndex.size);
  
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
    inputPortDelayByOrder[i] = PushArray<DelayInfo>(out,maxPortIndex + 1);

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

    if(!(node->type == NodeType_SINK || node->type == NodeType_SOURCE_AND_SINK)){
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

      int edgeDelay = edgesDelays[edgeIndex].value;
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

      edgesDelays[edgeIndex].value = nodeDelay - edgesDelays[edgeIndex].value;
      
      int edgeDelay = edgesDelays[edgeIndex].value;
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

      edgesDelays[edgeIndex].value -= minEdgeDelay;
    }
  }

  if(globalOptions.debug){
    static int funCalls = 0;
    BLOCK_REGION(out);
    
    String fileName = PushString(out,"edge_delays_%.*s_%d_%d.dot",UNPACK_SS(top.accelName),top.mergeIndex,funCalls++);
    String filePath = PushDebugPath(out,top.accelName,STRING("delays"),fileName);

    GraphPrintingContent content = GenerateLatencyDotGraph(top,orderToIndex,nodeDelayArray,edgesDelays,temp,out);
    
    String result = GenerateDotGraph(content,out,debugArena);
    OutputContentToFile(filePath,result);
  }
  
  // Store delays on data producing units
  for(int i = 0; i < orderToIndex.size; i++){
    FUInstance* node = top.GetUnit(orderToIndex[i])->inst;

    if(node->type != NodeType_SOURCE){
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

      int edgeDelay = edgesDelays[edgeIndex].value;
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

      edgesDelays[edgeIndex].value -= minEdgeDelay;
    }
  }

  if(globalOptions.debug){
    static int funCalls = 0;
    BLOCK_REGION(out);
    
    String fileName = PushString(out,"final_delays_%.*s_%d_%d.dot",UNPACK_SS(top.accelName),top.mergeIndex,funCalls++);

    String filePath = PushDebugPath(out,top.accelName,STRING("delays"),fileName);

    GraphPrintingContent content = GenerateLatencyDotGraph(top,orderToIndex,nodeDelayArray,edgesDelays,temp,out);
    
    String result = GenerateDotGraph(content,out,debugArena);
    OutputContentToFile(filePath,result);
  }
  
  SimpleCalculateDelayResult res = {};
  res.nodeDelayByOrder = nodeDelayArray;
  res.edgesDelay = edgesDelays;
  res.inputPortDelayByOrder = inputPortDelayByOrder;
  
  // TODO: Missing pushing delays towards inputs and pushing delays towards outputs. I think.
  //       Probably best to add more complex tests that push the delay algorithm further.

  return res;
}

CalculateDelayResult CalculateDelay(Accelerator* accel,Arena* out){
  STACK_ARENA(tempInst,Kilobyte(256));
  AccelInfo info = {};

  info.infos = PushArray<MergePartition>(out,1);
  info.infos[0].info = GenerateInitialInstanceInfo(accel,out,&tempInst,{});

  AccelInfoIterator top = StartIteration(&info);
  top.accelName = accel->name;
  SimpleCalculateDelayResult delays = CalculateDelay(top,out,&tempInst);

  EdgeDelay* edgeToDelay = PushHashmap<Edge,DelayInfo>(out,delays.edgesDelay.size);
  NodeDelay* nodeDelay = PushHashmap<FUInstance*,DelayInfo>(out,delays.nodeDelayByOrder.size);
  PortDelay* portDelay = PushHashmap<PortInstance,DelayInfo>(out,delays.edgesDelay.size);

  int index = 0;
  for(AccelEdgeIterator iter = IterateEdges(top); IsValid(iter); Advance(iter),index += 1){
    SimpleEdge simple = Get(iter);

    Edge edge = {};
    edge.out.port = simple.outPort;
    edge.out.inst = top.GetUnit(simple.outIndex)->inst;
    edge.in.port = simple.inPort;
    edge.in.inst = top.GetUnit(simple.inIndex)->inst;

    edgeToDelay->Insert(edge,delays.edgesDelay[index]);
  }
    
  for(AccelInfoIterator iter = top; iter.IsValid(); iter = iter.Next()){
    FUInstance* node = iter.CurrentUnit()->inst;
    int order = iter.CurrentUnit()->localOrder;

    nodeDelay->Insert(node,delays.nodeDelayByOrder[order]);

    Array<DelayInfo> portDelayInfo = delays.inputPortDelayByOrder[order];
    for(int i = 0; i < iter.CurrentUnit()->inputs.size; i++){
      PortInstance p = {};
      p.inst = node;
      p.port = i;

      portDelay->Insert(p,portDelayInfo[i]);
    }
  }

  CalculateDelayResult res = {};
  res.edgesDelay = edgeToDelay;
  res.nodeDelay = nodeDelay;
  res.portDelay = portDelay;

  return res;
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




























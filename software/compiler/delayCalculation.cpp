#include "delayCalculation.hpp"

#include "configurations.hpp"
#include "debug.hpp"
#include "declaration.hpp"
#include "dotGraphPrinting.hpp"
#include "memory.hpp"
#include "versat.hpp"
#include "debugVersat.hpp"
#include "versatSpecificationParser.hpp"

const int ANY_DELAY_MARK = 99999;

static void SendLatencyUpwards(FUInstance* node,Hashmap<Edge,int>* delays,Hashmap<FUInstance*,int>* nodeDelay){
  int b = nodeDelay->GetOrFail(node);
  FUInstance* inst = node;

  FOREACH_LIST(ConnectionNode*,info,node->allOutputs){
    FUInstance* other = info->instConnectedTo.inst;

    // Do not set delay for source units. Source units cannot be found in this, otherwise they wouldn't be source
    Assert(other->type != NodeType_SOURCE);

    int a = inst->declaration->baseConfig.outputLatencies[info->port];
    int e = info->edgeDelay;

    FOREACH_LIST(ConnectionNode*,otherInfo,other->allInputs){
      int c = other->declaration->baseConfig.inputDelays[info->instConnectedTo.port];

      if(info->instConnectedTo.port == otherInfo->port &&
         otherInfo->instConnectedTo.inst == inst && otherInfo->instConnectedTo.port == info->port){
        
        int delay = b + a + e - c;

        if(b == ANY_DELAY_MARK || node->declaration == BasicDeclaration::buffer){
          *otherInfo->delay.value = ANY_DELAY_MARK;
        } else {
          *otherInfo->delay.value = delay;
        }
      }
    }
  }
}

// A quick explanation: Starting from the inputs, we associate to edges a value that indicates how much cycles data needs to be delayed in order to arrive at the needed time. Starting from inputs in a breadth firsty manner makes this work fine but I think that it is not required. This value is given by the latency of the output + delay of the edge + input delay. Afterwards, we can always move delay around (Ex: If we fix a given unit, we can increase the delay of each output edge by one if we subtract the delay of each input edge by one. We can essentially move delay from input edges to output edges).
// NOTE: Thinking things a bit more, the number associated to the edges is not something that we want. If a node as a edge with number 0 and another with number 3, we need to add a fixed buffer of latency 3 to the edge with number 0, not the other way around. This might mean that after doing all the passes that we want, we still might need to invert everything (in this case, the edge with 0 would get a 3 and the edge with a 3 would get a zero). That is, if we still want to preserve the idea that each number associated to the edge is equal to the latency that we need to add. 

// Negative value edges are ok since at the end we can renormalize everything back into positive by adding the absolute value of the lowest negative to every edge (this also works because we use positive values in the input nodes to represent delay).
// In fact, this code could be simplified if we made the process of pushing delay from one place to another more explicit. Also TODO: We technically can use the ability of pushing delay to produce accelerators that contain less buffers. Node with many outputs we want to move delay as much as possible to it's inputs. Node with many inputs we want to move delay as much as possible to it's outputs. Currently we only favor one side because we basically just try to move delays to the outside as much as possible.
// TODO: Simplify the code. Check if removing the breadth first and just iterating by nodes and incrementing the edges values ends up producing the same result. Basically a loop where for each node we accumulate on the respective edges the values of the delays from the nodes respective ports plus the value of the edges themselves and finally we normalize everything to start at zero. I do not see any reason why this wouldn't work.
// TODO: Furthermode, encode the ability to push latency upwards or downwards and look into improving the circuits generated. Should also look into transforming the edge mapping from an Hashmap to an Array but still do not know if we need to map edges in this improved algorithm. (Techically we could make an auxiliary function that flattens everything and replaces edge lookups with indexes to the array.). 

// Instead of an accelerator, it could take a ordered list of instances, and potentialy the max amount of edges for the hashmap instantiation.
// Therefore abstracting from the accelerator completely and being able to be used for things like subgraphs.
// Change later if needed.

CalculateDelayResult CalculateDelay2(Accelerator* accel,DAGOrderNodes order,Arena* out){
  return {};
}

CalculateDelayResult CalculateDelay(Accelerator* accel,DAGOrderNodes order,Arena* out){
  // TODO: We are currently using the delay pointer inside the ConnectionNode structure. When correct, eventually change to just use the hashmap
  static int functionCalls = 0;
  
  int nodes = Size(accel->allocated);
  int edges = 9999;
  //int edges = Size(accel->edges);
  Hashmap<Edge,int>* edgeToDelay = PushHashmap<Edge,int>(out,edges);
  Hashmap<FUInstance*,int>* nodeDelay = PushHashmap<FUInstance*,int>(out,nodes);
  Hashmap<PortInstance,int>* portDelay = PushHashmap<PortInstance,int>(out,edges);
  
  CalculateDelayResult res = {};
  res.edgesDelay = edgeToDelay;
  res.nodeDelay = nodeDelay;
  res.portDelay = portDelay;
  
  FOREACH_LIST(FUInstance*,ptr,accel->allocated){
    nodeDelay->Insert(ptr,0);

    FOREACH_LIST(ConnectionNode*,con,ptr->allInputs){
      Edge edge = {};

      edge.units[0] = con->instConnectedTo;
      edge.units[1].inst = ptr;
      edge.units[1].port = con->port;
      edge.delay = con->edgeDelay;
      
      con->delay.value = edgeToDelay->Insert(edge,0);
    }

    FOREACH_LIST(ConnectionNode*,con,ptr->allOutputs){
      Edge edge = {};

      edge.units[0].inst = ptr;
      edge.units[0].port = con->port;
      edge.units[1] = con->instConnectedTo;
      edge.delay = con->edgeDelay;

      con->delay.value = edgeToDelay->Insert(edge,0);
    }
  }

  // TODO: Much of this code could be simplified
  {
    for(int i = 0; i < order.instances.size; i++){
      FUInstance* node = order.instances[i];
      FUInstance* inst = node;
      
      FOREACH_LIST(ConnectionNode*,info,node->allOutputs){
        FUInstance* other = info->instConnectedTo.inst;

        FOREACH_LIST(ConnectionNode*,otherInfo,other->allInputs){

          if(info->instConnectedTo.port == otherInfo->port &&
             otherInfo->instConnectedTo.inst == inst && otherInfo->instConnectedTo.port == info->port){

            PortInstance node = {.inst = other,.port = info->instConnectedTo.port};
            portDelay->Insert(node,0);
          }
        }
      }
    }
  }
 
  // Start at sources
  int graphs = 0;
  int index = 0;
  for(; index < order.instances.size; index++){
    FUInstance* node = order.instances[index];

    if(node->type != NodeType_SOURCE && node->type != NodeType_SOURCE_AND_SINK){
      continue; // This break is important because further code relies on it. 
    }

    nodeDelay->Insert(node,0);

    SendLatencyUpwards(node,edgeToDelay,nodeDelay);

    region(out){
      String fileName = PushString(out,"%d_out1_%d.dot",functionCalls,graphs++);
      String filePath = PushDebugPath(out,accel->name,STRING("delays"),fileName);
      
      GraphPrintingContent content = GenerateDelayDotGraph(accel,res,out,debugArena);
      String result = GenerateDotGraph(accel,content,out,debugArena);
      OutputContentToFile(filePath,result);
    }
  }

  index = 0;
  // Continue up the tree
  for(; index < order.instances.size; index++){
    FUInstance* node = order.instances[index];
    if(node->type == NodeType_UNCONNECTED
       || node->type == NodeType_SOURCE){
      continue;
    }

    int maximum = -(1 << 30);
    FOREACH_LIST(ConnectionNode*,info,node->allInputs){
      if(*info->delay.value != ANY_DELAY_MARK){
        maximum = std::max(maximum,*info->delay.value);
      }
    }
    
    if(maximum == -(1 << 30)){
      continue;
    }

    FOREACH_LIST(ConnectionNode*,info,node->allInputs){
      if(*info->delay.value == ANY_DELAY_MARK){
        *info->delay.value = 0;
      } else {
        *info->delay.value = maximum - *info->delay.value;
      }
    }

    nodeDelay->Insert(node,maximum);

    if(node->type != NodeType_SOURCE_AND_SINK){
      SendLatencyUpwards(node,edgeToDelay,nodeDelay);
    }

    region(out){
      String fileName = PushString(out,"%d_out2_%d.dot",functionCalls,graphs++);
      String filePath = PushDebugPath(out,accel->name,STRING("delays"),fileName);
      
      GraphPrintingContent content = GenerateDelayDotGraph(accel,res,out,debugArena);
      String result = GenerateDotGraph(accel,content,out,debugArena);
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
    int delay = nodeDelay->GetOrFail(node);

    minimum = std::min(minimum,delay);
  }
  for(int i = 0; i < order.instances.size; i++){
    FUInstance* node = order.instances[i];
    int* delay = nodeDelay->Get(node);
    *delay -= minimum;
  }
  
  region(out){
    String fileName = PushString(out,"%d_out3_%d.dot",functionCalls,graphs++);
    String filePath = PushDebugPath(out,accel->name,STRING("delays"),fileName);
      
    GraphPrintingContent content = GenerateDelayDotGraph(accel,res,out,debugArena);
    String result = GenerateDotGraph(accel,content,out,debugArena);
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
      nodeDelay->Insert(node,minimum);

      FOREACH_LIST(ConnectionNode*,info,node->allOutputs){
        *info->delay.value -= minimum;
      }
    }
  }

  region(out){
    String fileName = PushString(out,"%d_out4_%d.dot",functionCalls,graphs++);
    String filePath = PushDebugPath(out,accel->name,STRING("delays"),fileName);
      
    GraphPrintingContent content = GenerateDelayDotGraph(accel,res,out,debugArena);
    String result = GenerateDotGraph(accel,content,out,debugArena);
    OutputContentToFile(filePath,result);
  }

  for(int i = 0; i < order.instances.size; i++){
    FUInstance* node = order.instances[i];

    if(node->type == NodeType_UNCONNECTED){
      nodeDelay->Insert(node,0);
    }
  }

  for(auto p :res.nodeDelay){
    Assert(*p.second >= 0);
  }
  for(Pair<FUInstance*,int*> p : res.nodeDelay){
    Assert(*p.second >= 0);
  }
  
  {
    for(int i = 0; i < order.instances.size; i++){
      FUInstance* node = order.instances[i];
      FUInstance* inst = node;
      int b = nodeDelay->GetOrFail(node);
      
      FOREACH_LIST(ConnectionNode*,info,node->allOutputs){
        FUInstance* other = info->instConnectedTo.inst;

        int a = inst->declaration->baseConfig.outputLatencies[info->port];
        int e = info->edgeDelay;

        FOREACH_LIST(ConnectionNode*,otherInfo,other->allInputs){
          int c = other->declaration->baseConfig.inputDelays[info->instConnectedTo.port];

          if(info->instConnectedTo.port == otherInfo->port &&
             otherInfo->instConnectedTo.inst == inst && otherInfo->instConnectedTo.port == info->port){
        
            int delay = b + a + e - c;

            PortInstance node = {.inst = other,.port = info->instConnectedTo.port};
            portDelay->Insert(node,delay);
          }
        }
      }
    }
  }
  
  region(out){
    String fileName = PushString(out,"%d_out_final.dot",functionCalls);
    String filepath = PushDebugPath(out,accel->name,STRING("delays"),fileName);

    GraphPrintingContent content = GenerateDelayDotGraph(accel,res,out,debugArena);
    String result = GenerateDotGraph(accel,content,out,debugArena);
    OutputContentToFile(filepath,result);
  }

  functionCalls += 1;
  
  return res;
}

#include "debugVersat.hpp"

Array<FUInstance*> GetNodesWithOutputDelay(Accelerator* accel,Arena* out){
  DynamicArray<FUInstance*> arr = StartArray<FUInstance*>(out); 

  FOREACH_LIST(FUInstance*,node,accel->allocated){
    if(node->type == NodeType_SOURCE || node->type == NodeType_SOURCE_AND_SINK){
      *arr.PushElem() = node;
    }
  }
  
  return EndArray(arr);
}

void OutputDelayDebugInfo(Accelerator* accel,Arena* temp){
  BLOCK_REGION(temp);

  String path = PushDebugPath(temp,accel->name,STRING("delayDebugInfo.txt"));
  FILE* file = fopen(StaticFormat("%.*s",UNPACK_SS(path)),"w");
  DEFER_CLOSE_FILE(file);

  if(!file){
    LOCATION();
    printf("Problem opening file: %s\n",path.data);
    DEBUG_BREAK();
  }
  
  Array<FUInstance*> outputNodes =  GetNodesWithOutputDelay(accel,temp);
  
  for(FUInstance* node : outputNodes){
    fprintf(file,"%.*s\n",UNPACK_SS(node->name));
  }
}

CalculateDelayResult CalculateGlobalInitialLatency(Accelerator* accel,DAGOrderNodes order,Arena* out,Arena* temp){
#if 0
  CalculateDelayResult result = {};

  int nodes = Size(accel->allocated);
  int edges = Size(accel->edges);
  Hashmap<EdgeNode,int>* edgeToDelay = PushHashmap<EdgeNode,int>(out,edges);
  Hashmap<InstanceNode*,int>* nodeDelay = PushHashmap<InstanceNode*,int>(out,nodes);
  Hashmap<PortNode,int>* portDelay = PushHashmap<PortNode,int>(out,edges);
#endif

  return {};
}

GraphPrintingContent GenerateDelayDotGraph(Accelerator* accel,CalculateDelayResult delayInfo,Arena* out,Arena* temp){
  BLOCK_REGION(temp);

  auto NodeWithDelayContent = [&](FUInstance* node,Arena* out) -> GraphInfo{
    int delay = delayInfo.nodeDelay->GetOrFail(node);
    String content = PushString(out,"%.*s:%d",UNPACK_SS(node->name),delay);

    return {content,DefaultNodeColor(node)};
  };

  auto EdgeWithDelayContent = [&](Edge* edge,Arena* out) -> GraphInfo{
    int inPort = edge->in.port;
    int outPort = edge->out.port;

    int delay = delayInfo.portDelay->GetOrFail(edge->in);
    
    int edgeBaseDelay = edge->delay;
    int edgeDelay = delayInfo.edgesDelay->GetOrFail(*edge);
    
    int edgeOutput = edge->out.inst->declaration->baseConfig.outputLatencies[edge->out.port];
    int edgeInput = edge->in.inst->declaration->baseConfig.inputDelays[edge->in.port];
    
    String content = PushString(out,"(%d/%d/%d) %d [%d]",edgeOutput,edgeBaseDelay,edgeInput,delay,edgeDelay);
    String first = edge->out.inst->name;
    String second = edge->in.inst->name; 

    Color color = Color_BLACK;
    if(edgeDelay == ANY_DELAY_MARK){
     color = Color_RED;
    }

    return {content,color};
  };

#if 0
  Array<Edge> edges = GetAllEdges(accel,temp);

  ArenaList<GraphPrintingNodeInfo>* nodeInfo = PushArenaList<GraphPrintingNodeInfo>(temp);
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    FUInstance* inst = ptr->inst;

    int delay = delayInfo.nodeDelay->GetOrFail(ptr);
    String content = PushString(out,"%.*s:%d",UNPACK_SS(inst->name),delay);

    GraphPrintingColor color = DefaultNodeColor(ptr);
    *nodeInfo->PushElem() = {.name = inst->name,.content = content,.color = color};
  }
  Array<GraphPrintingNodeInfo> nodes = PushArrayFromList(out,nodeInfo);

  ArenaList<GraphPrintingEdgeInfo>* edgeInfo = PushArenaList<GraphPrintingEdgeInfo>(temp);
  for(Edge edge : edges){
    int inPort = edge.in.port;
    int outPort = edge.out.port;

    PortNode node = {};
    node.node = GetInstanceNode(accel,edge.in.inst); // TODO: HACK, Edges should be instanceNodes, not fuInstances
    node.port = edge.in.port;
    
    int delay = delayInfo.portDelay->GetOrFail(node);
    
    EdgeNode edgeNode = {};
    edgeNode.node0.node = GetInstanceNode(accel,edge.out.inst);
    edgeNode.node1.node = GetInstanceNode(accel,edge.in.inst);
    edgeNode.node0.port = edge.out.port;
    edgeNode.node1.port = edge.in.port;

    int edgeBaseDelay = edge.delay;
    int edgeDelay = delayInfo.edgesDelay->GetOrFail(edgeNode);
    
    int edgeOutput = edge.out.inst->declaration->baseConfig.outputLatencies[edge.out.port];
    int edgeInput = edge.in.inst->declaration->baseConfig.inputDelays[edge.in.port];
    
    String content = PushString(out,"(%d/%d/%d) %d [%d]",edgeOutput,edgeBaseDelay,edgeInput,delay,edgeDelay);
    String first = edge.out.inst->name;
    String second = edge.in.inst->name; 

    GraphPrintingColor color = GraphPrintingColor_BLACK;
    if(edgeDelay == ANY_DELAY_MARK){
     color = GraphPrintingColor_RED;
    }

    *edgeInfo->PushElem() = {
      .content = content,
      .color = color,
      .first = first,
      .second = second
    };
  }
  Array<GraphPrintingEdgeInfo> edgeResult = PushArrayFromList(out,edgeInfo);
#endif

  GraphPrintingContent result = GeneratePrintingContent(accel,NodeWithDelayContent,EdgeWithDelayContent,out,temp);
  result.graphLabel = STRING("Nodes contain their global latency after the \':\'\nEdge Format: (<Output Latency>/<Edge delay>/<Input Delay>) <Without buffer, node lantency + edge value>  [<Buffer value, difference between without buffer and input node latency>]\nImagine reading from the output node into the input node: Node latency + edge values equals latency which we must fix by inserting buffer of value N");
//  result.nodes = nodes;
//  result.edges = edgeResult;
  
  return result;
}




























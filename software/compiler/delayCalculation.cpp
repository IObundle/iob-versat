#include "delayCalculation.hpp"

#include "configurations.hpp"
#include "debug.hpp"
#include "declaration.hpp"
#include "dotGraphPrinting.hpp"
#include "memory.hpp"
#include "utilsCore.hpp"
#include "versat.hpp"
#include "debugVersat.hpp"
#include "versatSpecificationParser.hpp"

static void SendLatencyUpwards(FUInstance* node,EdgeDelay* delays,NodeDelay* nodeDelay){
  DelayInfo b = nodeDelay->GetOrFail(node); 
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
        
        int delay = b.value + a + e - c;

        Edge edge = {};
        edge.out = {node,info->port};
        edge.in = {other,info->instConnectedTo.port};
        
        if(b.isAny || node->declaration == BasicDeclaration::buffer){
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

// A quick explanation: Starting from the inputs, we associate to edges a value that indicates how much cycles data needs to be delayed in order to arrive at the needed time. Starting from inputs in a breadth firsty manner makes this work fine but I think that it is not required. This value is given by the latency of the output + delay of the edge + input delay. Afterwards, we can always move delay around (Ex: If we fix a given unit, we can increase the delay of each output edge by one if we subtract the delay of each input edge by one. We can essentially move delay from input edges to output edges).
// NOTE: Thinking things a bit more, the number associated to the edges is not something that we want. If a node as a edge with number 0 and another with number 3, we need to add a fixed buffer of latency 3 to the edge with number 0, not the other way around. This might mean that after doing all the passes that we want, we still might need to invert everything (in this case, the edge with 0 would get a 3 and the edge with a 3 would get a zero). That is, if we still want to preserve the idea that each number associated to the edge is equal to the latency that we need to add. 

// Negative value edges are ok since at the end we can renormalize everything back into positive by adding the absolute value of the lowest negative to every edge (this also works because we use positive values in the input nodes to represent delay).
// In fact, this code could be simplified if we made the process of pushing delay from one place to another more explicit. Also TODO: We technically can use the ability of pushing delay to produce accelerators that contain less buffers. Node with many outputs we want to move delay as much as possible to it's inputs. Node with many inputs we want to move delay as much as possible to it's outputs. Currently we only favor one side because we basically just try to move delays to the outside as much as possible.
// TODO: Simplify the code. Check if removing the breadth first and just iterating by nodes and incrementing the edges values ends up producing the same result. Basically a loop where for each node we accumulate on the respective edges the values of the delays from the nodes respective ports plus the value of the edges themselves and finally we normalize everything to start at zero. I do not see any reason why this wouldn't work.
// TODO: Furthermode, encode the ability to push latency upwards or downwards and look into improving the circuits generated. Should also look into transforming the edge mapping from an Hashmap to an Array but still do not know if we need to map edges in this improved algorithm. (Techically we could make an auxiliary function that flattens everything and replaces edge lookups with indexes to the array.). 

// Instead of an accelerator, it could take a ordered list of instances, and potentialy the max amount of edges for the hashmap instantiation.
// Therefore abstracting from the accelerator completely and being able to be used for things like subgraphs.
// Change later if needed.

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

CalculateDelayResult CalculateDelay(Accelerator* accel,DAGOrderNodes order,Arena* out){
  // TODO: We are currently using the delay pointer inside the ConnectionNode structure. When correct, eventually change to just use the hashmap
  static int functionCalls = 0;
  
  int nodes = accel->allocated.Size();
  int edges = 9999;
  //int edges = Size(accel->edges);
  EdgeDelay* edgeDelay = PushHashmap<Edge,DelayInfo>(out,edges);
  NodeDelay* nodeDelay = PushHashmap<FUInstance*,DelayInfo>(out,nodes);
  PortDelay* portDelay = PushHashmap<PortInstance,DelayInfo>(out,edges);
  
  CalculateDelayResult res = {};
  res.edgesDelay = edgeDelay;
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

    int* delayPtr = &edgeDelay->Insert(edge,{0,false})->value;
    
    fromOut->delay.value = delayPtr;
    fromIn->delay.value = delayPtr;
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
            portDelay->Insert(node,{0,false});
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

    nodeDelay->Insert(node,{0,false});

    SendLatencyUpwards(node,edgeDelay,nodeDelay);

    if(globalOptions.debug){
      BLOCK_REGION(out);
      String fileName = PushString(out,"0_%d_out1_%d.dot",functionCalls,graphs++);
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
      SendLatencyUpwards(node,edgeDelay,nodeDelay);
    }

    if(globalOptions.debug){
      BLOCK_REGION(out);
      String fileName = PushString(out,"0_%d_out2_%d.dot",functionCalls,graphs++);
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
    String fileName = PushString(out,"0_%d_out3_%d.dot",functionCalls,graphs++);
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
      nodeDelay->Insert(node,{minimum,false});

      FOREACH_LIST(ConnectionNode*,info,node->allOutputs){
        *info->delay.value -= minimum;
      }
    }
  }

  if(globalOptions.debug){
    BLOCK_REGION(out);
    String fileName = PushString(out,"0_%d_out4_%d.dot",functionCalls,graphs++);
    String filePath = PushDebugPath(out,accel->name,STRING("delays"),fileName);
      
    GraphPrintingContent content = GenerateDelayDotGraph(accel,res,out,debugArena);
    String result = GenerateDotGraph(accel,content,out,debugArena);
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
    String fileName = PushString(out,"0_%d_out5_%d.dot",functionCalls,graphs++);
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
            portDelay->Insert(node,{delay,false});
          }
        }
      }
    }
  }
  
  if(globalOptions.debug){
    BLOCK_REGION(out);
    String fileName = PushString(out,"0_%d_out_final.dot",functionCalls);
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

  for(FUInstance* node : accel->allocated){
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
    
    int edgeOutput = edge->out.inst->declaration->baseConfig.outputLatencies[edge->out.port];
    int edgeInput = edge->in.inst->declaration->baseConfig.inputDelays[edge->in.port];
    
    String content = PushString(out,"(%d/%d/%d) %d [%d]",edgeOutput,edgeBaseDelay,edgeInput,portDelay,edgeDelay);
    String first = edge->out.inst->name;
    String second = edge->in.inst->name; 

    Color color = Color_BLACK;
    if(delay.isAny){
     color = Color_RED;
    }

    return {content,color};
  };

  GraphPrintingContent result = GeneratePrintingContent(accel,NodeWithDelayContent,EdgeWithDelayContent,out,temp);
  result.graphLabel = STRING("Nodes contain their global latency after the \':\'\nEdge Format: (<Output Latency>/<Edge delay>/<Input Delay>) <Without buffer, node lantency + edge value>  [<Buffer value, difference between without buffer and input node latency>]\nImagine reading from the output node into the input node: Node latency + edge values equals latency which we must fix by inserting buffer of value N");
//  result.nodes = nodes;
//  result.edges = edgeResult;
  
  return result;
}




























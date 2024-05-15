#include "declaration.hpp"
#include "globals.hpp"
#include "memory.hpp"
#include "utils.hpp"
#include "utilsCore.hpp"
#include "versat.hpp"

#include <unordered_map>
#include <queue>

#include "debug.hpp"
#include "debugVersat.hpp"
#include "configurations.hpp"

#define TAG_TEMPORARY_MARK 1
#define TAG_PERMANENT_MARK 2

static Pool<Accelerator> accelerators;

typedef Hashmap<FUInstance*,FUInstance*> InstanceMap;

Accelerator* CreateAccelerator(String name){
  Accelerator* accel = accelerators.Alloc();
  
  accel->accelMemory = CreateDynamicArena(1);

  accel->name = PushString(accel->accelMemory,name);

  return accel;
}

InstanceNode* CreateFlatFUInstance(Accelerator* accel,FUDeclaration* type,String name){
  String storedName = PushString(accel->accelMemory,name);

  Assert(CheckValidName(storedName));

  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    if(ptr->inst->name == storedName){
      Assert(false);
      break;
    }
  }

  FUInstance* inst = accel->instances.Alloc();
  InstanceNode* ptr = PushStruct<InstanceNode>(accel->accelMemory);
  ptr->inst = inst;

  ptr->inputs = PushArray<PortNode>(accel->accelMemory,type->NumberInputs());
  ptr->outputs = PushArray<bool>(accel->accelMemory,type->NumberOutputs());

  if(accel->lastAllocated){
    Assert(accel->lastAllocated->next == nullptr);
    accel->lastAllocated->next = ptr;
  } else {
    accel->allocated = ptr;
  }
  accel->lastAllocated = ptr;

  inst->name = storedName;
  inst->accel = accel;
  inst->declaration = type;

  for(Pair<StaticId,StaticData> pair : type->staticUnits){
    StaticId first = pair.first;
    StaticData second = pair.second;
    accel->staticUnits.insert({first,second});
  }

  return ptr;
}

// Used to be CreateAndConfigure. 
InstanceNode* CreateFUInstanceNode(Accelerator* accel,FUDeclaration* type,String name){
  InstanceNode* node = CreateFlatFUInstance(accel,type,name);
  FUInstance* inst = node->inst;

  return node;
}

FUInstance* CreateFUInstance(Accelerator* accel,FUDeclaration* type,String name){
  FUInstance* res = CreateFUInstanceNode(accel,type,name)->inst;

  return res;
}

Accelerator* CopyAccelerator(Accelerator* accel,InstanceMap* map,Arena* out){
  STACK_ARENA(temp,Kilobyte(128));

  Accelerator* newAccel = CreateAccelerator(accel->name);

  if(map == nullptr){
    map = PushHashmap<FUInstance*,FUInstance*>(&temp,999);
  }

  // Copy of instances
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    FUInstance* inst = ptr->inst;
    FUInstance* newInst = CopyInstance(newAccel,inst,inst->name);

    newInst->literal = inst->literal;

    map->Insert(inst,newInst);
  }

  // Flat copy of edges
  FOREACH_LIST(Edge*,edge,accel->edges){
    FUInstance* out = (FUInstance*) map->GetOrFail(edge->units[0].inst);
    int outPort = edge->units[0].port;
    FUInstance* in = (FUInstance*) map->GetOrFail(edge->units[1].inst);
    int inPort = edge->units[1].port;

    ConnectUnits(out,outPort,in,inPort,edge->delay);
  }
  
  return newAccel;
}

FUInstance* CopyFlatInstance(Accelerator* newAccel,FUInstance* oldInstance,String newName){
  FUInstance* newInst = CreateFlatFUInstance(newAccel,oldInstance->declaration,newName)->inst;

  newInst->parameters = oldInstance->parameters;
  newInst->portIndex = oldInstance->portIndex;

  // Static and shared not handled for flat copy. Not sure if makes sense or not. See where it goes

  return newInst;
}

Accelerator* CopyFlatAccelerator(Accelerator* accel,InstanceMap* map){
  Accelerator* newAccel = CreateAccelerator(accel->name);
  InstanceMap nullCaseMap;

  if(map == nullptr){
    map = &nullCaseMap;
  }

  // Copy of instances
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    FUInstance* inst = ptr->inst;
    FUInstance* newInst = CopyFlatInstance(newAccel,inst,inst->name);
    newInst->literal = inst->literal;

    map->Insert(inst,newInst);
  }

  // Flat copy of edges
  FOREACH_LIST(Edge*,edge,accel->edges){
    FUInstance* out = map->GetOrFail(edge->units[0].inst);
    int outPort = edge->units[0].port;
    FUInstance* in = map->GetOrFail(edge->units[1].inst);
    int inPort = edge->units[1].port;

    ConnectUnits(out,outPort,in,inPort,edge->delay);
  }

  return newAccel;
}

FUInstance* CopyInstance(Accelerator* newAccel,FUInstance* oldInstance,String newName){
  FUInstance* newInst = CreateFUInstance(newAccel,oldInstance->declaration,newName);

  newInst->parameters = oldInstance->parameters;
  newInst->portIndex = oldInstance->portIndex;

  if(oldInstance->isStatic){
    SetStatic(newAccel,newInst);
  }
  if(oldInstance->sharedEnable){
    ShareInstanceConfig(newInst,oldInstance->sharedIndex);
  }

  return newInst;
}

InstanceNode* CopyInstance(Accelerator* newAccel,InstanceNode* oldInstance,String newName){
  InstanceNode* newNode = CreateFUInstanceNode(newAccel,oldInstance->inst->declaration,newName);
  FUInstance* newInst = newNode->inst;

  newInst->parameters = oldInstance->inst->parameters;
  newInst->portIndex = oldInstance->inst->portIndex;

  if(oldInstance->inst->isStatic){
    SetStatic(newAccel,newInst);
  }
  if(oldInstance->inst->sharedEnable){
    ShareInstanceConfig(newInst,oldInstance->inst->sharedIndex);
  }

  return newNode;
}

void RemoveFUInstance(Accelerator* accel,InstanceNode* node){
  FUInstance* inst = node->inst;

  FOREACH_LIST(Edge*,edge,accel->edges){
    if(edge->units[0].inst == inst){
      accel->edges = ListRemove(accel->edges,edge);
    } else if(edge->units[1].inst == inst){
      accel->edges = ListRemove(accel->edges,edge);
    }
  }

  accel->allocated = RemoveUnit(accel->allocated,node);
}

Array<int> ExtractInputDelays(Accelerator* accel,CalculateDelayResult delays,Arena* out,Arena* temp){
  Array<InstanceNode*> inputNodes = PushArray<InstanceNode*>(temp,99);

  bool seenOneInput = false;
  int maxInput = 0;
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    if(ptr->inst->declaration == BasicDeclaration::input){
      maxInput = std::max(maxInput,ptr->inst->portIndex);
      inputNodes[ptr->inst->portIndex] = ptr;
      seenOneInput = true;
    }
  }
  if(seenOneInput) maxInput += 1;
  inputNodes.size = maxInput;
  
  Array<int> inputDelay = PushArray<int>(out,maxInput);

  for(int i = 0; i < inputNodes.size; i++){
    InstanceNode* node = inputNodes[i];
    inputDelay[i] = delays.nodeDelay->GetOrFail(node);
  }

  return inputDelay;
}

Array<int> ExtractOutputLatencies(Accelerator* accel,CalculateDelayResult delays,Arena* out,Arena* temp){
  InstanceNode* outputNode = nullptr;
  int maxOutput = 0;
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    if(ptr->inst->declaration == BasicDeclaration::output){
      Assert(outputNode == nullptr);
      outputNode = ptr;
    }
  }

  if(outputNode){
    FOREACH_LIST(ConnectionNode*,ptr,outputNode->allInputs){
      maxOutput = std::max(maxOutput,ptr->port);
    }
    maxOutput += 1;
  } else {
    return {};
  }
  
  Array<int> outputLatencies = PushArray<int>(out,maxOutput);
  Memset(outputLatencies,0);

  if(outputNode){
    FOREACH_LIST(ConnectionNode*,ptr,outputNode->allInputs){
      int port = ptr->port;
      PortNode end = {.node = outputNode,.port = port};

      outputLatencies[port] = delays.portDelay->GetOrFail(end);
    }
  }

  return outputLatencies;
}

Accelerator* Flatten(Accelerator* accel,int times,Arena* temp){
  Arena* perm = globalPermanent;
  BLOCK_REGION(temp);

  InstanceMap* map = PushHashmap<FUInstance*,FUInstance*>(temp,999);
  Accelerator* newAccel = CopyFlatAccelerator(accel,map);

  Pool<InstanceNode*> compositeInstances = {};
  Pool<InstanceNode*> toRemove = {};
  std::unordered_map<StaticId,int> staticToIndex;

  for(int i = 0; i < times; i++){
    int maxSharedIndex = -1;
    FOREACH_LIST(InstanceNode*,instPtr,newAccel->allocated){
      FUInstance* inst = instPtr->inst;
      bool containsStatic = inst->isStatic || inst->declaration->staticUnits != nullptr;
      // TODO: For now we do not flatten units that are static or contain statics. It messes merge configurations and still do not know how to procceed. Need to beef merge up a bit before handling more complex cases. The problem was that after flattening statics would become shared (unions) and as such would appear in places where they where not supposed to appear.
      // TODO: Maybe static and shared units configurations could be stored in a separated structure.
      if(inst->declaration->type == FUDeclarationType_COMPOSITE && !containsStatic){
        InstanceNode** ptr = compositeInstances.Alloc();

        *ptr = instPtr;
      }

      if(inst->sharedEnable){
        maxSharedIndex = std::max(maxSharedIndex,inst->sharedIndex);
      }
    }

    if(compositeInstances.Size() == 0){
      break;
    }

    std::unordered_map<int,int> sharedToFirstChildIndex;

    int freeSharedIndex = (maxSharedIndex != -1 ? maxSharedIndex + 1 : 0);
    int count = 0;
    for(InstanceNode** instPtr : compositeInstances){
      FUInstance* inst = (*instPtr)->inst;

      Assert(inst->declaration->type == FUDeclarationType_COMPOSITE);

      count += 1;
      Accelerator* circuit = inst->declaration->baseCircuit;
      FUInstance* outputInstance = GetOutputInstance(circuit->allocated);

      int savedSharedIndex = freeSharedIndex;
      if(inst->sharedEnable){
        // Flattening a shared unit
        auto iter = sharedToFirstChildIndex.find(inst->sharedIndex);

        if(iter == sharedToFirstChildIndex.end()){
          sharedToFirstChildIndex.insert({inst->sharedIndex,freeSharedIndex});
        } else {
          freeSharedIndex = iter->second;
        }
      }

      std::unordered_map<int,int> sharedToShared;
      // Create new instance and map then
      FOREACH_LIST(InstanceNode*,ptr,circuit->allocated){
        FUInstance* circuitInst = ptr->inst;
        if(circuitInst->declaration->type == FUDeclarationType_SPECIAL){
          continue;
        }

        String newName = PushString(perm,"%.*s_%.*s",UNPACK_SS(inst->name),UNPACK_SS(circuitInst->name));
        FUInstance* newInst = CopyFlatInstance(newAccel,circuitInst,newName);

        if(circuitInst->isStatic){
          bool found = false;
          int shareIndex = 0;
          for(auto iter : staticToIndex){
            if(iter.first.parent == inst->declaration && CompareString(iter.first.name,circuitInst->name)){
              found = true;
              shareIndex = iter.second;
              break;
            }
          }

          if(!found){
            shareIndex = freeSharedIndex++;

            StaticId id = {};
            id.name = circuitInst->name;
            id.parent = inst->declaration;
            staticToIndex.insert({id,shareIndex});
          }

          ShareInstanceConfig(newInst,shareIndex);
        } else if(circuitInst->sharedEnable && inst->sharedEnable){
          auto ptr = sharedToShared.find(circuitInst->sharedIndex);

          if(ptr != sharedToShared.end()){
            ShareInstanceConfig(newInst,ptr->second);
          } else {
            int newIndex = freeSharedIndex++;

            sharedToShared.insert({circuitInst->sharedIndex,newIndex});

            ShareInstanceConfig(newInst,newIndex);
          }
        } else if(inst->sharedEnable){ // Currently flattening instance is shared
               ShareInstanceConfig(newInst,freeSharedIndex++);
        } else if(circuitInst->sharedEnable){
          auto ptr = sharedToShared.find(circuitInst->sharedIndex);

          if(ptr != sharedToShared.end()){
            ShareInstanceConfig(newInst,ptr->second);
          } else {
            int newIndex = freeSharedIndex++;

            sharedToShared.insert({circuitInst->sharedIndex,newIndex});

            ShareInstanceConfig(newInst,newIndex);
          }
        }

        map->Insert(circuitInst,newInst);
      }

      if(inst->sharedEnable && savedSharedIndex > freeSharedIndex){
        freeSharedIndex = savedSharedIndex;
      }

      // Add accel edges to output instances
      FOREACH_LIST(Edge*,edge,newAccel->edges){
        if(edge->units[0].inst == inst){
          FOREACH_LIST(Edge*,circuitEdge,circuit->edges){
            if(circuitEdge->units[1].inst == outputInstance && circuitEdge->units[1].port == edge->units[0].port){
              FUInstance** other = map->Get(circuitEdge->units[0].inst);

              if(other){
                continue;
              }

              FUInstance* out = *other;
              FUInstance* in = edge->units[1].inst;
              int outPort = circuitEdge->units[0].port;
              int inPort = edge->units[1].port;
              int delay = edge->delay + circuitEdge->delay;

              ConnectUnits(out,outPort,in,inPort,delay);
            }
          }
        }
      }

      // Add accel edges to input instances
      FOREACH_LIST(Edge*,edge,newAccel->edges){
        if(edge->units[1].inst == inst){
          FUInstance* circuitInst = GetInputInstance(circuit->allocated,edge->units[1].port);

          FOREACH_LIST(Edge*,circuitEdge,circuit->edges){
            if(circuitEdge->units[0].inst == circuitInst){
              FUInstance** other = map->Get(circuitEdge->units[0].inst);

              if(other){
                continue;
              }


              FUInstance* out = edge->units[0].inst;
              FUInstance* in = *other;
              int outPort = edge->units[0].port;
              int inPort = circuitEdge->units[1].port;
              int delay = edge->delay + circuitEdge->delay;

              ConnectUnits(out,outPort,in,inPort,delay);
            }
          }
        }
      }

      // Add circuit specific edges
      FOREACH_LIST(Edge*,circuitEdge,circuit->edges){
        FUInstance** otherInput = map->Get(circuitEdge->units[0].inst);
        FUInstance** otherOutput = map->Get(circuitEdge->units[1].inst);

        if(otherInput && otherOutput){
          continue;
        }

        FUInstance* out = *otherInput;
        FUInstance* in = *otherOutput;
        int outPort = circuitEdge->units[0].port;
        int inPort = circuitEdge->units[1].port;
        int delay = circuitEdge->delay;

        ConnectUnits(out,outPort,in,inPort,delay);
      }

      // Add input to output specific edges
      FOREACH_LIST(Edge*,edge1,newAccel->edges){
        if(edge1->units[1].inst == inst){
          PortInstance input = edge1->units[0];
          FUInstance* circuitInput = GetInputInstance(circuit->allocated,edge1->units[1].port);

          FOREACH_LIST(Edge*,edge2,newAccel->edges){
            if(edge2->units[0].inst == inst){
              PortInstance output = edge2->units[1];
              int outputPort = edge2->units[0].port;

              FOREACH_LIST(Edge*,circuitEdge,circuit->edges){
                if(circuitEdge->units[0].inst == circuitInput
                   && circuitEdge->units[1].inst == outputInstance
                   && circuitEdge->units[1].port == outputPort){
                  int delay = edge1->delay + circuitEdge->delay + edge2->delay;

                  ConnectUnits(input,output,delay);
                }
              }
            }
          }
        }
      }

      RemoveFUInstance(newAccel,*instPtr);
      AssertGraphValid(newAccel->allocated,temp);

      map->Clear();
    }

    toRemove.Clear();
    compositeInstances.Clear();
  }

  String debugFilepath = PushDebugPath(temp,accel->name,"flatten.dot");
  OutputGraphDotFile(newAccel,true,debugFilepath,temp);

  toRemove.Clear(true);
  compositeInstances.Clear(true);

  newAccel->staticUnits.clear();

  FUDeclaration base = {};
  newAccel->name = accel->name;

  return newAccel;
}

const int ANY_DELAY_MARK = 99999;

static void SendLatencyUpwards(InstanceNode* node,Hashmap<EdgeNode,int>* delays,Hashmap<InstanceNode*,int>* nodeDelay){
  int b = nodeDelay->GetOrFail(node);
  FUInstance* inst = node->inst;

  FOREACH_LIST(ConnectionNode*,info,node->allOutputs){
    InstanceNode* other = info->instConnectedTo.node;

    // Do not set delay for source units. Source units cannot be found in this, otherwise they wouldn't be source
    Assert(other->type != NodeType_SOURCE);

    int a = inst->declaration->baseConfig.outputLatencies[info->port];
    int e = info->edgeDelay;

    FOREACH_LIST(ConnectionNode*,otherInfo,other->allInputs){
      int c = other->inst->declaration->baseConfig.inputDelays[info->instConnectedTo.port];

      if(info->instConnectedTo.port == otherInfo->port &&
         otherInfo->instConnectedTo.node->inst == inst && otherInfo->instConnectedTo.port == info->port){
        
        int delay = b + a + e - c;

        if(b == ANY_DELAY_MARK || node->inst->declaration == BasicDeclaration::buffer){
          *otherInfo->delay = ANY_DELAY_MARK;
        } else {
          *otherInfo->delay = delay;
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

CalculateDelayResult CalculateDelay(Accelerator* accel,DAGOrderNodes order,Arena* out){
  // TODO: We are currently using the delay pointer inside the ConnectionNode structure. When correct, eventually change to just use the hashmap
  static int functionCalls = 0;
  
  int nodes = Size(accel->allocated);
  int edges = Size(accel->edges);
  Hashmap<EdgeNode,int>* edgeToDelay = PushHashmap<EdgeNode,int>(out,edges);
  Hashmap<InstanceNode*,int>* nodeDelay = PushHashmap<InstanceNode*,int>(out,nodes);
  Hashmap<PortNode,int>* portDelay = PushHashmap<PortNode,int>(out,edges);
  
  CalculateDelayResult res = {};
  res.edgesDelay = edgeToDelay;
  res.nodeDelay = nodeDelay;
  res.portDelay = portDelay;
  
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    nodeDelay->Insert(ptr,0);

    FOREACH_LIST(ConnectionNode*,con,ptr->allInputs){
      EdgeNode edge = {};

      edge.node0 = con->instConnectedTo;
      edge.node1.node = ptr;
      edge.node1.port = con->port;

      con->delay = edgeToDelay->Insert(edge,0);
    }

    FOREACH_LIST(ConnectionNode*,con,ptr->allOutputs){
      EdgeNode edge = {};

      edge.node0.node = ptr;
      edge.node0.port = con->port;
      edge.node1 = con->instConnectedTo;

      con->delay = edgeToDelay->Insert(edge,0);
    }
  }

  // Start at sources
  int graphs = 0;
  int index = 0;
  for(; index < order.instances.size; index++){
    InstanceNode* node = order.instances[index];

    if(node->type != NodeType_SOURCE && node->type != NodeType_SOURCE_AND_SINK){
      continue; // This break is important because further code relies on it. 
    }

    nodeDelay->Insert(node,0);

    SendLatencyUpwards(node,edgeToDelay,nodeDelay);

    region(out){
      String filepath = PushDebugPath(out,accel->name,StaticFormat("%d_out1_%d.dot",functionCalls,graphs++));
      OutputGraphDotFile(accel,true,node->inst,res,filepath,out);
    }
  }

  index = 0;
  // Continue up the tree
  for(; index < order.instances.size; index++){
    InstanceNode* node = order.instances[index];
    if(node->type == NodeType_UNCONNECTED
       || node->type == NodeType_SOURCE){
      continue;
    }

    int maximum = -(1 << 30);
    FOREACH_LIST(ConnectionNode*,info,node->allInputs){
      if(*info->delay != ANY_DELAY_MARK){
        maximum = std::max(maximum,*info->delay);
      }
    }
    
    if(maximum == -(1 << 30)){
      continue;
    }
    
    FOREACH_LIST(ConnectionNode*,info,node->allInputs){
      if(*info->delay == ANY_DELAY_MARK){
        *info->delay = 0;
      } else {
        *info->delay = maximum - *info->delay;
      }
    }

    nodeDelay->Insert(node,maximum);

    if(node->type != NodeType_SOURCE_AND_SINK){
      SendLatencyUpwards(node,edgeToDelay,nodeDelay);
    }

    region(out){
      String filepath = PushDebugPath(out,accel->name,StaticFormat("%d_out2_%d.dot",functionCalls,graphs++));
      OutputGraphDotFile(accel,true,node->inst,res,filepath,out);
    }
  }

  for(int i = 0; i < order.instances.size; i++){
    InstanceNode* node = order.instances[i];
  
    if(node->type != NodeType_SOURCE_AND_SINK){
      continue;
    }

    // Source_and_sink units never have output delay. They can't
    FOREACH_LIST(ConnectionNode*,con,node->allOutputs){
      *con->delay = 0;
    }
  }

  int minimum = 0;
  for(int i = 0; i < order.instances.size; i++){
    InstanceNode* node = order.instances[i];
    int delay = nodeDelay->GetOrFail(node);

    minimum = std::min(minimum,delay);
  }
  for(int i = 0; i < order.instances.size; i++){
    InstanceNode* node = order.instances[i];
    int* delay = nodeDelay->Get(node);
    *delay -= minimum;
  }
  
  region(out){
    String filepath = PushDebugPath(out,accel->name,StaticFormat("%d_out3_%d.dot",functionCalls,graphs++));
    OutputGraphDotFile(accel,true,nullptr,res,filepath,out);
  }

  if(!globalOptions.disableDelayPropagation){
    // Normalizes everything to start on zero
    for(int i = 0; i < order.instances.size; i++){
      InstanceNode* node = order.instances[i];

      if(node->type != NodeType_SOURCE && node->type != NodeType_SOURCE_AND_SINK){
        break;
      }

      int minimum = 1 << 30;
      FOREACH_LIST(ConnectionNode*,info,node->allOutputs){
        minimum = std::min(minimum,*info->delay);
      }

      if(minimum == 1 << 30){
        continue;
      }
      
      // Does not take into account unit latency
      nodeDelay->Insert(node,minimum);

      FOREACH_LIST(ConnectionNode*,info,node->allOutputs){
        *info->delay -= minimum;
      }
    }
  }

  region(out){
    String filepath = PushDebugPath(out,accel->name,StaticFormat("%d_out4_%d.dot",functionCalls,graphs++));
    OutputGraphDotFile(accel,true,nullptr,res,filepath,out);
  }

  for(int i = 0; i < order.instances.size; i++){
    InstanceNode* node = order.instances[i];

    if(node->type == NodeType_UNCONNECTED){
      nodeDelay->Insert(node,0);
    }
  }

  for(auto p :res.nodeDelay){
    Assert(p.second >= 0);
  }
  for(Pair<InstanceNode*,int> p : res.nodeDelay){
    Assert(p.second >= 0);
  }
  
  {
    for(int i = 0; i < order.instances.size; i++){
      InstanceNode* node = order.instances[i];
      FUInstance* inst = node->inst;
      int b = nodeDelay->GetOrFail(node);
      
      FOREACH_LIST(ConnectionNode*,info,node->allOutputs){
        InstanceNode* other = info->instConnectedTo.node;

        int a = inst->declaration->baseConfig.outputLatencies[info->port];
        int e = info->edgeDelay;

        FOREACH_LIST(ConnectionNode*,otherInfo,other->allInputs){
          int c = other->inst->declaration->baseConfig.inputDelays[info->instConnectedTo.port];

          if(info->instConnectedTo.port == otherInfo->port &&
             otherInfo->instConnectedTo.node->inst == inst && otherInfo->instConnectedTo.port == info->port){
        
            int delay = b + a + e - c;

            PortNode node = {.node = other,.port = info->instConnectedTo.port};
            portDelay->Insert(node,delay);
          }
        }
      }
    }
  }
  
  functionCalls += 1;
  
  return res;
}

// TODO: This functions should have actual error handling and reporting. Instead of just Asserting.
static int Visit(PushPtr<InstanceNode*>* ordering,InstanceNode* node,Hashmap<InstanceNode*,int>* tags){
  int* tag = tags->Get(node);
  Assert(tag);

  if(*tag == TAG_PERMANENT_MARK){
    return 0;
  }
  if(*tag == TAG_TEMPORARY_MARK){
    Assert(0);
  }

  FUInstance* inst = node->inst;

  if(node->type == NodeType_SINK ||
     (node->type == NodeType_SOURCE_AND_SINK && CHECK_DELAY(inst,DelayType::DelayType_SINK_DELAY))){
    return 0;
  }

  *tag = TAG_TEMPORARY_MARK;

  int count = 0;
  if(node->type == NodeType_COMPUTE){
    FOREACH_LIST(ConnectionNode*,ptr,node->allInputs){
      count += Visit(ordering,ptr->instConnectedTo.node,tags);
    }
  }

  *tag = TAG_PERMANENT_MARK;

  if(node->type == NodeType_COMPUTE){
    *ordering->Push(1) = node;
    count += 1;
  }

  return count;
}

DAGOrderNodes CalculateDAGOrder(InstanceNode* instances,Arena* out){
  int size = Size(instances);

  DAGOrderNodes res = {};

  res.size = 0;
  res.instances = PushArray<InstanceNode*>(out,size);
  res.order = PushArray<int>(out,size);

  PushPtr<InstanceNode*> pushPtr = {};
  pushPtr.Init(res.instances);

  //BLOCK_REGION(out);

  Hashmap<InstanceNode*,int>* tags = PushHashmap<InstanceNode*,int>(out,size);

  FOREACH_LIST(InstanceNode*,ptr,instances){
    res.size += 1;
    tags->Insert(ptr,0);
  }

  int mark = pushPtr.Mark();
  // Add source units, guaranteed to come first
  FOREACH_LIST(InstanceNode*,ptr,instances){
    FUInstance* inst = ptr->inst;
    if(ptr->type == NodeType_SOURCE || (ptr->type == NodeType_SOURCE_AND_SINK && CHECK_DELAY(inst,DelayType::DelayType_SOURCE_DELAY))){
      *pushPtr.Push(1) = ptr;

      tags->Insert(ptr,TAG_PERMANENT_MARK);
    }
  }
  res.sources = pushPtr.PopMark(mark);

  // Add compute units
  mark = pushPtr.Mark();
  FOREACH_LIST(InstanceNode*,ptr,instances){
    if(ptr->type == NodeType_UNCONNECTED){
      *pushPtr.Push(1) = ptr;
      tags->Insert(ptr,TAG_PERMANENT_MARK);
    } else {
      int tag = tags->GetOrFail(ptr);
      if(tag == 0 && ptr->type == NodeType_COMPUTE){
        Visit(&pushPtr,ptr,tags);
      }
    }
  }
  res.computeUnits = pushPtr.PopMark(mark);
  
  // Add sink units
  mark = pushPtr.Mark();
  FOREACH_LIST(InstanceNode*,ptr,instances){
    FUInstance* inst = ptr->inst;
    if(ptr->type == NodeType_SINK || (ptr->type == NodeType_SOURCE_AND_SINK && CHECK_DELAY(inst,DelayType::DelayType_SINK_DELAY))){
      *pushPtr.Push(1) = ptr;

      int* tag = tags->Get(ptr);
      Assert(*tag == 0);

      *tag = TAG_PERMANENT_MARK;
    }
  }
  res.sinks = pushPtr.PopMark(mark);

  FOREACH_LIST(InstanceNode*,ptr,instances){
    int tag = tags->GetOrFail(ptr);
    Assert(tag == TAG_PERMANENT_MARK);
  }

  Assert(res.sources.size + res.computeUnits.size + res.sinks.size == size);

  // Reuse tags hashmap
  Hashmap<InstanceNode*,int>* orderIndex = tags;
  orderIndex->Clear();

  res.maxOrder = 0;
  for(int i = 0; i < size; i++){
    InstanceNode* node = res.instances[i];

    int order = 0;
    FOREACH_LIST(ConnectionNode*,ptr,node->allInputs){
      InstanceNode* other = ptr->instConnectedTo.node;

      if(other->type == NodeType_SOURCE_AND_SINK){
        continue;
      }

      int index = orderIndex->GetOrFail(other);

      order = std::max(order,res.order[index]);
    }

    orderIndex->Insert(node,i); // Only insert after to detect any potential error.
    res.order[i] = order;
    res.maxOrder = std::max(res.maxOrder,order);
  }

  return res;
}

bool IsUnitCombinatorial(FUInstance* instance){
  FUDeclaration* type = instance->declaration;

  // TODO: Could check if it's a multiple type situation and check each one to make sure.
  for(int i = 0; i < type->NumberOutputs(); i++){
    if(type->baseConfig.outputLatencies[i] != 0){
      return false;
    }
  }

  return true;
}

#if 0
void ReorganizeAccelerator(Accelerator* accel,Arena* temp){
  if(accel->ordered){
    return;
  }

  DAGOrderNodes order = CalculateDAGOrder(accel->allocated,temp);
  // Reorganize nodes based on DAG order
  int size = Size(accel->allocated);

  accel->ordered = nullptr; // TODO: We could technically reuse the entirety of nodes just by putting them at the top of the free list when we implement a free list. Otherwise we are just leaky memory
  OrderedInstance* ordered = nullptr;
  for(int i = 0; i < size; i++){
    InstanceNode* ptr = order.instances[i];

    OrderedInstance* newOrdered = PushStruct<OrderedInstance>(accel->accelMemory);
    newOrdered->node = ptr;

    if(!accel->ordered){
      accel->ordered = newOrdered;
      ordered = newOrdered;
    } else {
      ordered->next = newOrdered;
      ordered = newOrdered;
    }
  }
}

void ReorganizeIterative(Accelerator* accel,Arena* temp){
  if(accel->ordered){
    return;
  }

  int size = Size(accel->allocated);
  Hashmap<InstanceNode*,int>* order = PushHashmap<InstanceNode*,int>(temp,size);
  Hashmap<InstanceNode*,bool>* seen = PushHashmap<InstanceNode*,bool>(temp,size);

  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    seen->Insert(ptr,false);
    order->Insert(ptr,size - 1);
  }

  // Start by seeing all inputs
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    if(ptr->allInputs == nullptr){
      seen->Insert(ptr,true);
      order->Insert(ptr,0);
    }
  }

  FOREACH_LIST(InstanceNode*,outerPtr,accel->allocated){
    FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
      bool allInputsSeen = true;
      FOREACH_LIST(ConnectionNode*,input,ptr->allInputs){
        InstanceNode* node = input->instConnectedTo.node;
        if(node == ptr){
          continue;
        }

        bool inputSeen = seen->GetOrFail(node);
        if(!inputSeen){
          allInputsSeen = false;
          break;
        }
      }

      if(!allInputsSeen){
        continue;
      }

      int maxOrder = 0;
      FOREACH_LIST(ConnectionNode*,input,ptr->allInputs){
        InstanceNode* node = input->instConnectedTo.node;
        maxOrder = std::max(maxOrder,order->GetOrFail(node));
      }

      order->Insert(ptr,maxOrder + 1);
    }
  }

  Array<InstanceNode*> nodes = PushArray<InstanceNode*>(temp,size);
  int index = 0;
  for(int i = 0; i < size; i++){
    FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
      int ord = order->GetOrFail(ptr);
      if(ord == i){
        nodes[index++] = ptr;
      }
    }
  }

  OrderedInstance* ordered = nullptr;
  for(int i = 0; i < size; i++){
    InstanceNode* ptr = nodes[i];

    OrderedInstance* newOrdered = PushStruct<OrderedInstance>(accel->accelMemory);
    newOrdered->node = ptr;

    if(!accel->ordered){
      accel->ordered = newOrdered;
      ordered = newOrdered;
    } else {
      ordered->next = newOrdered;
      ordered = newOrdered;
    }
  }

  accel->ordered = ReverseList(accel->ordered);
}
#endif

Array<DelayToAdd> GenerateFixDelays(Accelerator* accel,Hashmap<EdgeNode,int>* edgeDelays,Arena* out,Arena* temp){
  BLOCK_REGION(temp);

  ArenaList<DelayToAdd>* list = PushArenaList<DelayToAdd>(temp);
  
  int buffersInserted = 0;
  for(auto edgePair : edgeDelays){
    EdgeNode edge = edgePair.first;
    int delay = edgePair.second;

    if(delay == 0){
      continue;
    }

    Assert(delay > 0); // Cannot deal with negative delays at this stage.

    DelayToAdd var = {};
    var.edge = edge;
    var.bufferName = PushString(out,"buffer%d",buffersInserted++);
    var.bufferAmount = delay - BasicDeclaration::fixedBuffer->baseConfig.outputLatencies[0];
    var.bufferParameters = PushString(out,"#(.AMOUNT(%d))",var.bufferAmount);

    *list->PushElem() = var;
  }

  return PushArrayFromList(out,list);
}

void FixDelays(Accelerator* accel,Hashmap<EdgeNode,int>* edgeDelays,Arena* temp){
  Arena* perm = globalPermanent;
  BLOCK_REGION(temp);

  int buffersInserted = 0;
  for(auto edgePair : edgeDelays){
    EdgeNode edge = edgePair.first;
    int delay = edgePair.second;

    if(delay == 0){
      continue;
    }

    if(edge.node1.node->inst->declaration == BasicDeclaration::output){
      continue;
    }
    
    InstanceNode* output = edge.node0.node;

    if(output->inst->declaration == BasicDeclaration::buffer){
      //output->inst->baseDelay = delay;
      edgePair.second = 0;
      continue;
    }
    Assert(delay > 0); // Cannot deal with negative delays at this stage.

    FUInstance* buffer = nullptr;
    if(globalOptions.useFixedBuffers){
      String bufferName = PushString(perm,"fixedBuffer%d",buffersInserted);

      buffer = (FUInstance*) CreateFUInstance(accel,BasicDeclaration::fixedBuffer,bufferName);
      buffer->bufferAmount = delay - BasicDeclaration::fixedBuffer->baseConfig.outputLatencies[0];
      buffer->parameters = PushString(perm,"#(.AMOUNT(%d))",buffer->bufferAmount);
    } else {
      String bufferName = PushString(perm,"buffer%d",buffersInserted);

      buffer = (FUInstance*) CreateFUInstance(accel,BasicDeclaration::buffer,bufferName);
      buffer->bufferAmount = delay - BasicDeclaration::buffer->baseConfig.outputLatencies[0];
      Assert(buffer->bufferAmount >= 0);
      SetStatic(accel,buffer);
    }

    InsertUnit(accel,edge.node0,edge.node1,PortNode{GetInstanceNode(accel,buffer),0});

    String filePath = PushDebugPath(temp,accel->name,StaticFormat("fixDelay_%d.dot",buffersInserted));
    OutputGraphDotFile(accel,true,buffer,filePath,temp);
    buffersInserted += 1;
  }
}

InstanceNode* GetInputNode(InstanceNode* nodes,int inputIndex){
  FOREACH_LIST(InstanceNode*,ptr,nodes){
    FUInstance* inst = ptr->inst;
    if(inst->declaration == BasicDeclaration::input && inst->portIndex == inputIndex){
      return ptr;
    }
  }
  return nullptr;
}

FUInstance* GetInputInstance(InstanceNode* nodes,int inputIndex){
  FOREACH_LIST(InstanceNode*,ptr,nodes){
    FUInstance* inst = ptr->inst;
    if(inst->declaration == BasicDeclaration::input && inst->portIndex == inputIndex){
      return inst;
    }
  }
  return nullptr;
}

FUInstance* GetOutputInstance(InstanceNode* nodes){
  FOREACH_LIST(InstanceNode*,ptr,nodes){
    FUInstance* inst = ptr->inst;
    if(inst->declaration == BasicDeclaration::output){
      return inst;
    }
  }

  return nullptr;
}

bool IsCombinatorial(Accelerator* accel){
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    if(ptr->inst->declaration->fixedDelayCircuit == nullptr){
      continue;
    }
    
    bool isComb = IsCombinatorial(ptr->inst->declaration->fixedDelayCircuit);
    if(!isComb){
      return false;
    }
  }
  return true;
}

// TODO: Receive Accel info from above instead of recalculating it.
Array<FUDeclaration*> AllNonSpecialSubTypes(Accelerator* accel,Arena* out,Arena* temp){
  if(accel == nullptr){
    return {};
  }
  
  BLOCK_REGION(temp);
  
  Set<FUDeclaration*>* maps = PushSet<FUDeclaration*>(temp,99);
  
  Array<InstanceInfo> test = CalculateAcceleratorInfoNoDelay(accel,true,temp,out).baseInfo;
  for(InstanceInfo& info : test){
    if(info.decl->type != FUDeclarationType_SPECIAL){
      maps->Insert(info.decl);
    }
  }
  
  Array<FUDeclaration*> subTypes = PushArrayFromSet(out,maps);
  return subTypes;
}

Array<FUDeclaration*> ConfigSubTypes(Accelerator* accel,Arena* out,Arena* temp){
  if(accel == nullptr){
    return {};
  }
  
  BLOCK_REGION(temp);
  
  Set<FUDeclaration*>* maps = PushSet<FUDeclaration*>(temp,99);
  
  Array<InstanceInfo> test = CalculateAcceleratorInfoNoDelay(accel,true,temp,out).baseInfo;
  for(InstanceInfo& info : test){
    if(info.configSize > 0){
      maps->Insert(info.decl);
    }
  }
  
  Array<FUDeclaration*> subTypes = PushArrayFromSet(out,maps);
  return subTypes;
}

Array<FUDeclaration*> MemSubTypes(Accelerator* accel,Arena* out,Arena* temp){
  if(accel == nullptr){
    return {};
  }

  BLOCK_REGION(temp);
  
  Set<FUDeclaration*>* maps = PushSet<FUDeclaration*>(temp,99);

  Array<InstanceInfo> test = CalculateAcceleratorInfoNoDelay(accel,true,temp,out).baseInfo;
  for(InstanceInfo& info : test){
    if(info.memMappedSize.has_value()){
      maps->Insert(info.decl);
    }
  }
  
  Array<FUDeclaration*> subTypes = PushArrayFromSet(out,maps);
  return subTypes;
}

// Checks wether the external memory conforms to the expected interface or not (has valid values)
bool VerifyExternalMemory(ExternalMemoryInterface* inter){
  bool res;

  switch(inter->type){
  case ExternalMemoryType::TWO_P:{
    res = (inter->tp.bitSizeIn == inter->tp.bitSizeOut);
  }break;
  case ExternalMemoryType::DP:{
    res = (inter->dp[0].bitSize == inter->dp[1].bitSize);
  }break;
  default: NOT_IMPLEMENTED("Implemented as needed");
  }

  return res;
}

// Function that calculates byte offset from size of data_w
int DataWidthToByteOffset(int dataWidth){
  // 0-8 : 0,
  // 9-16 : 1
  // 17 - 32 : 2,
  // 33 - 64 : 3

  // 0 : 0
  // 8 : 0
  if(dataWidth == 0){
    return 0;
  }

  int res = log2i((dataWidth - 1) / 8);
  return res;
}

int ExternalMemoryByteSize(ExternalMemoryInterface* inter){
  Assert(VerifyExternalMemory(inter));

  // TODO: The memories size and address is still error prone. We should just store the total size and the address width in the structures (and then we can derive the data width from the given address width and the total size). Storing data width and address size at the same time gives more degrees of freedom than need. The parser is responsible in ensuring that the data is given in the correct format and this code should never have to worry about having to deal with values out of phase
  int addressBitSize = 0;
  int byteOffset = 0;
  switch(inter->type){
  case ExternalMemoryType::TWO_P:{
    addressBitSize = inter->tp.bitSizeIn;
    byteOffset = std::min(DataWidthToByteOffset(inter->tp.dataSizeIn),DataWidthToByteOffset(inter->tp.dataSizeOut));
  }break;
  case ExternalMemoryType::DP:{
    addressBitSize = inter->dp[0].bitSize;
    byteOffset = std::min(DataWidthToByteOffset(inter->dp[0].dataSizeIn),DataWidthToByteOffset(inter->dp[1].dataSizeIn));
  }break;
  default: NOT_IMPLEMENTED("Implemented as needed");
  }

  int byteSize = (1 << (addressBitSize + byteOffset));

  return byteSize;
}

int ExternalMemoryByteSize(Array<ExternalMemoryInterface> interfaces){
  int size = 0;

  // Not to sure about this logic. So far we do not allow different DATA_W values for the same port memories, but technically this should work
  for(ExternalMemoryInterface& inter : interfaces){
    int max = ExternalMemoryByteSize(&inter);

    // Aligns
    int nextPower2 = AlignNextPower2(max);
    int boundary = log2i(nextPower2);

    int aligned = AlignBitBoundary(size,boundary);
    size = aligned + max;
  }

  return size;
}

VersatComputedValues ComputeVersatValues(Accelerator* accel,bool useDMA){
  VersatComputedValues res = {};

  int memoryMappedDWords = 0;
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    FUInstance* inst = ptr->inst;
    FUDeclaration* decl = inst->declaration;

    res.numberConnections += Size(ptr->allOutputs);

    if(decl->memoryMapBits.has_value()){
      memoryMappedDWords = AlignBitBoundary(memoryMappedDWords,decl->memoryMapBits.value());
      memoryMappedDWords += 1 << decl->memoryMapBits.value();

      res.unitsMapped += 1;
    }

    res.nConfigs += decl->baseConfig.configs.size;
    for(Wire& wire : decl->baseConfig.configs){
      res.configBits += wire.bitSize;
    }

    res.nStates += decl->baseConfig.states.size;
    for(Wire& wire : decl->baseConfig.states){
      res.stateBits += wire.bitSize;
    }

    res.nDelays += decl->baseConfig.delayOffsets.max;
    res.delayBits += decl->baseConfig.delayOffsets.max * 32;

    res.nUnitsIO += decl->nIOs;

    res.externalMemoryInterfaces += decl->externalMemory.size;
    res.signalLoop |= decl->signalLoop;
  }

  for(auto pair : accel->staticUnits){
    StaticData data = pair.second;
    res.nStatics += data.configs.size;

    for(Wire& wire : data.configs){
      res.staticBits += wire.bitSize;
    }
  }

  res.staticBitsStart = res.configBits;
  res.delayBitsStart = res.staticBitsStart + res.staticBits;

  // Versat specific registers are treated as a special maping (all 0's) of 1 configuration and 1 state register
  res.versatConfigs = 1;
  res.versatStates = 1;

  if(useDMA){
    res.versatConfigs += 4;
    res.versatStates += 4;

    res.nUnitsIO += 1; // For the DMA
  }

  res.nConfigs += res.versatConfigs;
  res.nStates += res.versatStates;

  res.nConfigurations = res.nConfigs + res.nStatics + res.nDelays;
  res.configurationBits = res.configBits + res.staticBits + res.delayBits;

  res.memoryMappedBytes = memoryMappedDWords * 4;
  res.memoryAddressBits = log2i(memoryMappedDWords);

  res.memoryMappingAddressBits = res.memoryAddressBits;
  res.configurationAddressBits = log2i(res.nConfigurations);
  res.stateAddressBits = log2i(res.nStates);
  res.stateConfigurationAddressBits = std::max(res.configurationAddressBits,res.stateAddressBits);

  res.memoryConfigDecisionBit = std::max(res.stateConfigurationAddressBits,res.memoryMappingAddressBits) + 2;
  
  return res;
}

InstanceNode* GetInstanceNode(Accelerator* accel,FUInstance* inst){
   FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
      if(ptr->inst == inst){
         return ptr;
      }
   }
   Assert(false); // For now, this should not fail
   return nullptr;
}

void CalculateNodeType(InstanceNode* node){
   node->type = NodeType_UNCONNECTED;

   bool hasInput = (node->allInputs != nullptr);
   bool hasOutput = (node->allOutputs != nullptr);

   // If the unit is both capable of acting as a sink or as a source of data
   if(hasInput && hasOutput){
     if(CHECK_DELAY(node->inst,DelayType_SINK_DELAY) || CHECK_DELAY(node->inst,DelayType_SOURCE_DELAY)){
         node->type = NodeType_SOURCE_AND_SINK;
      }  else {
         node->type = NodeType_COMPUTE;
      }
   } else if(hasInput){
      node->type = NodeType_SINK;
   } else if(hasOutput){
      node->type = NodeType_SOURCE;
   } else {
      // Unconnected
   }
}

void FixInputs(InstanceNode* node){
   Memset(node->inputs,{});
   node->multipleSamePortInputs = false;

   FOREACH_LIST(ConnectionNode*,ptr,node->allInputs){
      int port = ptr->port;

      if(node->inputs[port].node){
         node->multipleSamePortInputs = true;
      }

      node->inputs[port] = ptr->instConnectedTo;
   }
}

void FixOutputs(InstanceNode* node){
   Memset(node->outputs,false);

   FOREACH_LIST(ConnectionNode*,ptr,node->allOutputs){
      int port = ptr->port;
      node->outputs[port] = true;
   }
}


// TODO: Bunch of functions that could go to graph.hpp
Edge* FindEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay){
  FUDeclaration* inDecl = in->declaration;
  FUDeclaration* outDecl = out->declaration;

  Assert(out->accel == in->accel);
  Assert(inIndex < inDecl->NumberInputs());
  Assert(outIndex < outDecl->NumberOutputs());

  Accelerator* accel = out->accel;

  FOREACH_LIST(Edge*,edge,accel->edges){
    if(edge->units[0].inst == (FUInstance*) out &&
       edge->units[0].port == outIndex &&
       edge->units[1].inst == (FUInstance*) in &&
       edge->units[1].port == inIndex &&
       edge->delay == delay) {
      return edge;
    }
  }

  return nullptr;
}

Edge* ConnectUnitsGetEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay){
  Edge* edge = ConnectUnitsGetEdge(out,outIndex,in,inIndex,delay,nullptr);

  return edge;
}

void ConnectUnits(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay){
  ConnectUnitsGetEdge(out,outIndex,in,inIndex,delay);
}

void ConnectUnitsIfNotConnected(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay){
  ConnectUnitsIfNotConnectedGetEdge(out,outIndex,in,inIndex,delay);
}

Edge* ConnectUnitsIfNotConnectedGetEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay){
  Accelerator* accel = out->accel;

  FOREACH_LIST(Edge*,edge,accel->edges){
    if(edge->units[0].inst == out && edge->units[0].port == outIndex &&
       edge->units[1].inst == in  && edge->units[1].port == inIndex &&
       delay == edge->delay){
      return edge;
    }
  }

  return ConnectUnitsGetEdge(out,outIndex,in,inIndex,delay);
}

Edge* ConnectUnits(PortInstance out,PortInstance in,int delay){
  return ConnectUnitsGetEdge(out.inst,out.port,in.inst,in.port,delay);
}

Edge* ConnectUnitsGetEdge(PortNode out,PortNode in,int delay){
   FUDeclaration* inDecl = in.node->inst->declaration;
   FUDeclaration* outDecl = out.node->inst->declaration;

   Assert(out.node->inst->accel == in.node->inst->accel);
   Assert(in.port < inDecl->NumberInputs());
   Assert(out.port < outDecl->NumberOutputs());

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
   outputNode->outputs[out.port] = true;
   //outputNode->outputs += 1;
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
   Assert(inIndex < inDecl->NumberInputs());
   Assert(outIndex < outDecl->NumberOutputs());

   Accelerator* accel = out->accel;

   Edge* edge = PushStruct<Edge>(accel->accelMemory);

   if(previous){
      edge->next = previous->next;
      previous->next = edge;
   } else {
      edge->next = accel->edges;
      accel->edges = edge;
   }

   edge->units[0].inst = (FUInstance*) out;
   edge->units[0].port = outIndex;
   edge->units[1].inst = (FUInstance*) in;
   edge->units[1].port = inIndex;
   edge->delay = delay;

   // Update graph data.
   InstanceNode* inputNode = GetInstanceNode(accel,(FUInstance*) in);
   InstanceNode* outputNode = GetInstanceNode(accel,(FUInstance*) out);

   // Add info to outputNode
   // Update all outputs
   {
   ConnectionNode* con = PushStruct<ConnectionNode>(accel->accelMemory);
   con->edgeDelay = delay;
   con->port = outIndex;
   con->instConnectedTo.node = inputNode;
   con->instConnectedTo.port = inIndex;

   outputNode->allOutputs = ListInsert(outputNode->allOutputs,con);
   outputNode->outputs[outIndex] = true;
   //outputNode->outputs += 1;
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

Array<int> GetNumberOfInputConnections(InstanceNode* node,Arena* out){
   Array<int> res = PushArray<int>(out,node->inputs.size);
   Memset(res,0);

   FOREACH_LIST(ConnectionNode*,ptr,node->allInputs){
      int port = ptr->port;
      res[port] += 1;
   }

   return res;
}

Array<Array<PortNode>> GetAllInputs(InstanceNode* node,Arena* out){
   Array<int> connections = GetNumberOfInputConnections(node,out); // Wastes a bit of memory because not deallocated after, its irrelevant

   Array<Array<PortNode>> res = PushArray<Array<PortNode>>(out,node->inputs.size);
   Memset(res,{});

   for(int i = 0; i < res.size; i++){
      res[i] = PushArray<PortNode>(out,connections[i]);
   }

   FOREACH_LIST(ConnectionNode*,ptr,node->allInputs){
      int port = ptr->port;

      Array<PortNode> array = res[port];
      int index = connections[port] - 1;
      connections[port] -= 1;
      array[index] = ptr->instConnectedTo;
   }

   return res;
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
   //out->outputs = Size(out->allOutputs);
   FixOutputs(out);

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
   //out->outputs = Size(out->allOutputs);
   FixOutputs(out);

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

   for(auto pair : toRemove){
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

void AssertGraphValid(InstanceNode* nodes,Arena* arena){
   BLOCK_REGION(arena);

   int size = Size(nodes);
   Hashmap<InstanceNode*,int>* seen = PushHashmap<InstanceNode*,int>(arena,size);

   FOREACH_LIST(InstanceNode*,ptr,nodes){
      seen->Insert(ptr,0);
   }

   FOREACH_LIST(InstanceNode*,ptr,nodes){
     FOREACH_LIST(ConnectionNode*,con,ptr->allInputs){
         seen->GetOrFail(con->instConnectedTo.node);
      }

     FOREACH_LIST(ConnectionNode*,con,ptr->allOutputs){
         seen->GetOrFail(con->instConnectedTo.node);
      }

      for(PortNode& n : ptr->inputs){
         if(n.node){
            seen->GetOrFail(n.node);
         }
      }
   }
}

//

//HACK
#if 1
static void SendLatencyUpwards(InstanceNode* node,Hashmap<EdgeNode,int>* delays,Hashmap<InstanceNode*,int>* nodeDelay,Hashmap<InstanceNode*,int>* nodeToPart){
  int b = nodeDelay->GetOrFail(node);
  FUInstance* inst = node->inst;
  FUDeclaration* decl = inst->declaration;
  bool multipleTypes = HasMultipleConfigs(decl);

  int nodePart = nodeToPart->GetOrFail(node);

  FOREACH_LIST(ConnectionNode*,info,node->allOutputs){
    InstanceNode* other = info->instConnectedTo.node;

    // Do not set delay for source units. Source units cannot be found in this, otherwise they wouldn't be source
    Assert(other->type != NodeType_SOURCE);

    int a = 0;

    if(multipleTypes){
      a = decl->configInfo[nodePart].outputLatencies[info->port];
    } else {
      a = inst->declaration->baseConfig.outputLatencies[info->port];
    }
    
    int e = info->edgeDelay;

    FOREACH_LIST(ConnectionNode*,otherInfo,other->allInputs){
      int c = 0;
      if(HasMultipleConfigs(other->inst->declaration)){
        int otherPart = nodeToPart->GetOrFail(other);
        
        c = other->inst->declaration->configInfo[otherPart].inputDelays[info->instConnectedTo.port];
      } else {
        c = other->inst->declaration->baseConfig.inputDelays[info->instConnectedTo.port];
      }

      if(info->instConnectedTo.port == otherInfo->port &&
         otherInfo->instConnectedTo.node->inst == inst && otherInfo->instConnectedTo.port == info->port){
        
        int delay = b + a + e - c;

        if(b == ANY_DELAY_MARK || node->inst->declaration == BasicDeclaration::buffer){
          *otherInfo->delay = ANY_DELAY_MARK;
        } else {
          *otherInfo->delay = delay;
        }
      }
    }
  }
}

CalculateDelayResult CalculateDelay(Accelerator* accel,DAGOrderNodes order,Array<Partition> partitions,Arena* out){
  // TODO: We are currently using the delay pointer inside the ConnectionNode structure. When correct, eventually change to just use the hashmap
  static int functionCalls = 0;
  
  int nodes = Size(accel->allocated);
  int edges = Size(accel->edges);
  Hashmap<EdgeNode,int>* edgeToDelay = PushHashmap<EdgeNode,int>(out,edges);
  Hashmap<InstanceNode*,int>* nodeDelay = PushHashmap<InstanceNode*,int>(out,nodes);
  Hashmap<PortNode,int>* portDelay = PushHashmap<PortNode,int>(out,edges);
  
  CalculateDelayResult res = {};
  res.edgesDelay = edgeToDelay;
  res.nodeDelay = nodeDelay;
  res.portDelay = portDelay;
  
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    nodeDelay->Insert(ptr,0);

    FOREACH_LIST(ConnectionNode*,con,ptr->allInputs){
      EdgeNode edge = {};

      edge.node0 = con->instConnectedTo;
      edge.node1.node = ptr;
      edge.node1.port = con->port;

      con->delay = edgeToDelay->Insert(edge,0);
    }

    FOREACH_LIST(ConnectionNode*,con,ptr->allOutputs){
      EdgeNode edge = {};

      edge.node0.node = ptr;
      edge.node0.port = con->port;
      edge.node1 = con->instConnectedTo;

      con->delay = edgeToDelay->Insert(edge,0);
    }
  }

  Hashmap<InstanceNode*,int>* nodeToPart = PushHashmap<InstanceNode*,int>(out,nodes); // TODO: Either temp or move out accross call stack
  {
  int partitionIndex = 0;
  for(int index = 0; index < order.instances.size; index++){
    InstanceNode* node = order.instances[index];
    
    int part = 0;
    if(HasMultipleConfigs(node->inst->declaration)){
      part = partitions[partitionIndex++].value;
    }

    nodeToPart->Insert(node,part);
  }
  }
  
  // Start at sources
  int graphs = 0;
  int index = 0;
  int partitionIndex = 0;
  for(; index < order.instances.size; index++){
    InstanceNode* node = order.instances[index];

    int part = 0;
    if(HasMultipleConfigs(node->inst->declaration)){
      part = partitions[partitionIndex++].value;
    }

    if(node->type != NodeType_SOURCE && node->type != NodeType_SOURCE_AND_SINK){
      continue; // This break is important because further code relies on it. 
    }

    nodeDelay->Insert(node,0);
    SendLatencyUpwards(node,edgeToDelay,nodeDelay,nodeToPart);

    region(out){
      String filepath = PushDebugPath(out,accel->name,StaticFormat("%d_out1_%d.dot",functionCalls,graphs++));
      OutputGraphDotFile(accel,true,node->inst,res,filepath,out);
    }
  }

  index = 0;
  partitionIndex = 0;
  // Continue up the tree
  for(; index < order.instances.size; index++){
    InstanceNode* node = order.instances[index];

    int part = 0;
    if(HasMultipleConfigs(node->inst->declaration)){
      part = partitions[partitionIndex++].value;
    }

    if(node->type == NodeType_UNCONNECTED
       || node->type == NodeType_SOURCE){
      continue;
    }

    int maximum = -(1 << 30);
    FOREACH_LIST(ConnectionNode*,info,node->allInputs){
      if(*info->delay != ANY_DELAY_MARK){
        maximum = std::max(maximum,*info->delay);
      }
    }
    
    if(maximum == -(1 << 30)){
      continue;
    }
    
    FOREACH_LIST(ConnectionNode*,info,node->allInputs){
      if(*info->delay == ANY_DELAY_MARK){
        *info->delay = 0;
      } else {
        *info->delay = maximum - *info->delay;
      }
    }

    nodeDelay->Insert(node,maximum);

    if(node->type != NodeType_SOURCE_AND_SINK){
      SendLatencyUpwards(node,edgeToDelay,nodeDelay,nodeToPart);
    }

    region(out){
      String filepath = PushDebugPath(out,accel->name,StaticFormat("%d_out2_%d.dot",functionCalls,graphs++));
      OutputGraphDotFile(accel,true,node->inst,res,filepath,out);
    }
  }

  for(int i = 0; i < order.instances.size; i++){
    InstanceNode* node = order.instances[i];

    if(node->type != NodeType_SOURCE_AND_SINK){
      continue;
    }

    // Source_and_sink units never have output delay. They can't
    FOREACH_LIST(ConnectionNode*,con,node->allOutputs){
      *con->delay = 0;
    }
  }

  int minimum = 0;
  for(int i = 0; i < order.instances.size; i++){
    InstanceNode* node = order.instances[i];
    int delay = nodeDelay->GetOrFail(node);

    minimum = std::min(minimum,delay);
  }
  for(int i = 0; i < order.instances.size; i++){
    InstanceNode* node = order.instances[i];
    int* delay = nodeDelay->Get(node);
    *delay -= minimum;
  }
  
  region(out){
    String filepath = PushDebugPath(out,accel->name,StaticFormat("%d_out3_%d.dot",functionCalls,graphs++));
    OutputGraphDotFile(accel,true,nullptr,res,filepath,out);
  }

  if(!globalOptions.disableDelayPropagation){
    // Normalizes everything to start on zero
    for(int i = 0; i < order.instances.size; i++){
      InstanceNode* node = order.instances[i];

      if(node->type != NodeType_SOURCE && node->type != NodeType_SOURCE_AND_SINK){
        break;
      }

      int minimum = 1 << 30;
      FOREACH_LIST(ConnectionNode*,info,node->allOutputs){
        minimum = std::min(minimum,*info->delay);
      }

      if(minimum == 1 << 30){
        continue;
      }
      
      // Does not take into account unit latency
      nodeDelay->Insert(node,minimum);

      FOREACH_LIST(ConnectionNode*,info,node->allOutputs){
        *info->delay -= minimum;
      }
    }
  }

  region(out){
    String filepath = PushDebugPath(out,accel->name,StaticFormat("%d_out4_%d.dot",functionCalls,graphs++));
    OutputGraphDotFile(accel,true,nullptr,res,filepath,out);
  }

  for(int i = 0; i < order.instances.size; i++){
    InstanceNode* node = order.instances[i];

    if(node->type == NodeType_UNCONNECTED){
      nodeDelay->Insert(node,0);
    }
  }

  for(auto p :res.nodeDelay){
    Assert(p.second >= 0);
  }
  for(Pair<InstanceNode*,int> p : res.nodeDelay){
    Assert(p.second >= 0);
  }
  
  {
    for(int i = 0; i < order.instances.size; i++){
      InstanceNode* node = order.instances[i];
      FUInstance* inst = node->inst;
      int b = nodeDelay->GetOrFail(node);
      FUDeclaration* decl = inst->declaration;
      bool multipleTypes = HasMultipleConfigs(decl);

      int nodePart = nodeToPart->GetOrFail(node);
      
      FOREACH_LIST(ConnectionNode*,info,node->allOutputs){
        InstanceNode* other = info->instConnectedTo.node;

        int a = 0;

        if(multipleTypes){
          a = decl->configInfo[nodePart].outputLatencies[info->port];
        } else {
          a = inst->declaration->baseConfig.outputLatencies[info->port];
        }
    
        int e = info->edgeDelay;

        FOREACH_LIST(ConnectionNode*,otherInfo,other->allInputs){
          int c = 0;
          if(HasMultipleConfigs(other->inst->declaration)){
            int otherPart = nodeToPart->GetOrFail(other);
        
            c = other->inst->declaration->configInfo[otherPart].inputDelays[info->instConnectedTo.port];
          } else {
            c = other->inst->declaration->baseConfig.inputDelays[info->instConnectedTo.port];
          }

          if(info->instConnectedTo.port == otherInfo->port &&
             otherInfo->instConnectedTo.node->inst == inst && otherInfo->instConnectedTo.port == info->port){
        
            int delay = b + a + e - c;

            PortNode node = {.node = other,.port = info->instConnectedTo.port};
            portDelay->Insert(node,delay);
          }
        }
      }
    }
  }
  
  functionCalls += 1;
  
  return res;
}

#endif

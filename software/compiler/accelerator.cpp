#include "acceleratorStats.hpp"
#include "versat.hpp"

#include <unordered_map>
#include <queue>

#include "debug.hpp"
#include "debugVersat.hpp"
#include "configurations.hpp"
#include "graph.hpp"

#define TAG_TEMPORARY_MARK 1
#define TAG_PERMANENT_MARK 2

typedef std::unordered_map<FUInstance*,FUInstance*> InstanceMap;

Accelerator* CreateAccelerator(Versat* versat,String name){
  Accelerator* accel = versat->accelerators.Alloc();
  accel->versat = versat;
  accel->name = name;

  accel->accelMemory = CreateDynamicArena(1);

  return accel;
}

void DestroyAccelerator(Versat* versat,Accelerator* accel){
  versat->accelerators.Remove(accel);
}

InstanceNode* CreateFlatFUInstance(Accelerator* accel,FUDeclaration* type,String name){
  Assert(CheckValidName(name));

  accel->ordered = nullptr; // TODO: Will leak

  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    if(ptr->inst->name == name){
      //Assert(false);
      break;
    }
  }

  FUInstance* inst = accel->instances.Alloc();
  InstanceNode* ptr = PushStruct<InstanceNode>(accel->accelMemory);
  ptr->inst = inst;

  ptr->inputs = PushArray<PortNode>(accel->accelMemory,type->inputDelays.size);
  ptr->outputs = PushArray<bool>(accel->accelMemory,type->outputLatencies.size);

  if(accel->lastAllocated){
    Assert(accel->lastAllocated->next == nullptr);
    accel->lastAllocated->next = ptr;
  } else {
    accel->allocated = ptr;
  }
  accel->lastAllocated = ptr;

  inst->name = name;
  inst->accel = accel;
  inst->declaration = type;

  for(Pair<StaticId,StaticData>& pair : type->staticUnits){
    StaticId first = pair.first;
    StaticData second = pair.second;
    accel->staticUnits.insert({first,second});
  }

  return ptr;
}

// Used to be CreateAndConfigure. 
InstanceNode* CreateFUInstanceNode(Accelerator* accel,FUDeclaration* type,String name){
  accel->ordered = nullptr;

  InstanceNode* node = CreateFlatFUInstance(accel,type,name);
  FUInstance* inst = node->inst;

  return node;
}

FUInstance* CreateFUInstance(Accelerator* accel,FUDeclaration* type,String name){
  FUInstance* res = CreateFUInstanceNode(accel,type,name)->inst;

  return res;
}

Accelerator* CopyAccelerator(Versat* versat,Accelerator* accel,InstanceMap* map){
  Accelerator* newAccel = CreateAccelerator(versat,accel->name);
  InstanceMap nullCaseMap;

  if(map == nullptr){
    map = &nullCaseMap;
  }

  // Copy of instances
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    FUInstance* inst = ptr->inst;
    FUInstance* newInst = CopyInstance(newAccel,inst,inst->name);

    newInst->literal = inst->literal;

    map->insert({inst,newInst});
  }

  // Flat copy of edges
  FOREACH_LIST(Edge*,edge,accel->edges){
    FUInstance* out = (FUInstance*) map->at(edge->units[0].inst);
    int outPort = edge->units[0].port;
    FUInstance* in = (FUInstance*) map->at(edge->units[1].inst);
    int inPort = edge->units[1].port;

    ConnectUnits(out,outPort,in,inPort,edge->delay);
  }

  return newAccel;
}

FUInstance* CopyInstance(Accelerator* newAccel,FUInstance* oldInstance,String newName){
  FUInstance* newInst = CreateFUInstance(newAccel,oldInstance->declaration,newName);

  newInst->parameters = oldInstance->parameters;
  newInst->baseDelay = oldInstance->baseDelay;
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
  newInst->baseDelay = oldInstance->inst->baseDelay;
  newInst->portIndex = oldInstance->inst->portIndex;

  if(oldInstance->inst->isStatic){
    SetStatic(newAccel,newInst);
  }
  if(oldInstance->inst->sharedEnable){
    ShareInstanceConfig(newInst,oldInstance->inst->sharedIndex);
  }

  return newNode;
}

FUInstance* CopyFlatInstance(Accelerator* newAccel,FUInstance* oldInstance,String newName){
  FUInstance* newInst = CreateFlatFUInstance(newAccel,oldInstance->declaration,newName)->inst;

  newInst->parameters = oldInstance->parameters;
  newInst->baseDelay = oldInstance->baseDelay;
  newInst->portIndex = oldInstance->portIndex;

  // Static and shared not handled for flat copy. Not sure if makes sense or not. See where it goes
#if 0
  if(oldInstance->isStatic){
    SetStatic(newAccel,newInst);
  }
  if(oldInstance->sharedEnable){
    ShareInstanceConfig(newInst,oldInstance->sharedIndex);
  }
#endif

  return newInst;
}

Accelerator* CopyFlatAccelerator(Versat* versat,Accelerator* accel,InstanceMap* map){
  Accelerator* newAccel = CreateAccelerator(versat,accel->name);
  InstanceMap nullCaseMap;

  if(map == nullptr){
    map = &nullCaseMap;
  }

  // Copy of instances
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    FUInstance* inst = ptr->inst;
    FUInstance* newInst = CopyFlatInstance(newAccel,inst,inst->name);
    newInst->literal = inst->literal;

    map->insert({inst,newInst});
  }

  // Flat copy of edges
  FOREACH_LIST(Edge*,edge,accel->edges){
    FUInstance* out = (FUInstance*) map->at(edge->units[0].inst);
    int outPort = edge->units[0].port;
    FUInstance* in = (FUInstance*) map->at(edge->units[1].inst);
    int inPort = edge->units[1].port;

    ConnectUnits(out,outPort,in,inPort,edge->delay);
  }

  return newAccel;
}

// This function works, but not for the flatten algorithm, as we hope that the configs match but they don't match for the outputs after removing a composite instance
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

Accelerator* Flatten(Versat* versat,Accelerator* accel,int times){
  Arena* temp = &versat->temp;
  BLOCK_REGION(temp);

  InstanceMap map;
  Accelerator* newAccel = CopyFlatAccelerator(versat,accel,&map);
  map.clear();

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
      if(inst->declaration->type == FUDeclaration::COMPOSITE && !containsStatic){
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

      Assert(inst->declaration->type == FUDeclaration::COMPOSITE);

      count += 1;
      Accelerator* circuit = inst->declaration->baseCircuit; // TODO: we replaced fixedDelay with base circuit. Future care
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
        if(circuitInst->declaration->type == FUDeclaration::SPECIAL){
          continue;
        }

        String newName = PushString(&versat->permanent,"%.*s_%.*s",UNPACK_SS(inst->name),UNPACK_SS(circuitInst->name));
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

        map.insert({circuitInst,newInst});
      }

      if(inst->sharedEnable && savedSharedIndex > freeSharedIndex){
        freeSharedIndex = savedSharedIndex;
      }

      // Add accel edges to output instances
      FOREACH_LIST(Edge*,edge,newAccel->edges){
        if(edge->units[0].inst == inst){
          FOREACH_LIST(Edge*,circuitEdge,circuit->edges){
            if(circuitEdge->units[1].inst == outputInstance && circuitEdge->units[1].port == edge->units[0].port){
              auto iter = map.find(circuitEdge->units[0].inst);

              if(iter == map.end()){
                continue;
              }

              FUInstance* out = iter->second;
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
              auto iter = map.find(circuitEdge->units[1].inst);

              if(iter == map.end()){
                continue;
              }

              FUInstance* out = edge->units[0].inst;
              FUInstance* in = iter->second;
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
        auto input = map.find(circuitEdge->units[0].inst);
        auto output = map.find(circuitEdge->units[1].inst);

        if(input == map.end() || output == map.end()){
          continue;
        }

        FUInstance* out = input->second;
        FUInstance* in = output->second;
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

      map.clear();
    }

#if 0
    for(FUInstance** instPtr : toRemove){
      FUInstance* inst = *instPtr;

      FlattenRemoveFUInstance(newAccel,inst);
    }
#endif

#if 0
    for(InstanceNode** instPtr : toRemove){
      RemoveFUInstance(newAccel,*instPtr);
    }
    AssertGraphValid(newAccel->allocated,temp);
#endif

    toRemove.Clear();
    compositeInstances.Clear();
  }

  String debugFilepath = PushString(temp,"debug/%.*s/flatten.dot",UNPACK_SS(accel->name));
  OutputGraphDotFile(versat,newAccel,true,debugFilepath,temp);

  toRemove.Clear(true);
  compositeInstances.Clear(true);

  newAccel->staticUnits.clear();

  FUDeclaration base = {};
  newAccel->name = accel->name;

  newAccel->ordered = nullptr;
  ReorganizeAccelerator(newAccel,temp);

  return newAccel;
}

static void SendLatencyUpwards(InstanceNode* node,Hashmap<EdgeNode,int>* delays,Hashmap<InstanceNode*,int>* nodeDelay){
  int b = nodeDelay->GetOrFail(node);
  FUInstance* inst = node->inst;

  FOREACH_LIST(ConnectionNode*,info,node->allOutputs){
    InstanceNode* other = info->instConnectedTo.node;

    // Do not set delay for source and sink units. Source units cannot be found in this, otherwise they wouldn't be source
    Assert(other->type != InstanceNode::TAG_SOURCE);

    int a = inst->declaration->outputLatencies[info->port];
    int e = info->edgeDelay;

    FOREACH_LIST(ConnectionNode*,otherInfo,other->allInputs){
      int c = other->inst->declaration->inputDelays[info->instConnectedTo.port];

      if(info->instConnectedTo.port == otherInfo->port &&
         otherInfo->instConnectedTo.node->inst == inst && otherInfo->instConnectedTo.port == info->port){

        int delay = b + a + e - c;

        *otherInfo->delay = delay;
      }
    }
  }
}

// A quick explanation: Starting from the inputs, we associate to edges a value that indicates how much cycles data needs to be delayed in order to arrive at the needed time. Starting from inputs in a breadth firsty manner makes this work fine but I think that it is not required. This value is given by the latency of the output + delay of the edge + input delay. Afterwards, we can always move delay around (Ex: If we fix a given unit, we can increase the delay of each output edge by one if we subtract the delay of each input edge by one. We can essentially move delay from input edges to output edges).
// Negative value edges are ok since at the end we can renormalize everything back into positive by adding the absolute value of the lowest negative to every edge (this also works because we use positive values in the input nodes to represent delay).
// In fact, this code could be simplified if we made the process of pushing delay from one place to another more explicit. Also TODO: We technically can use the ability of pushing delay to produce accelerators that contain less buffers. Node with many outputs we want to move delay as much as possible to it's inputs. Node with many inputs we want to move delay as much as possible to it's outputs. Currently we only favor one side because we basically just try to move delays to the outside as much as possible.
// TODO: Simplify the code. Check if removing the breadth first and just iterating by nodes and incrementing the edges values ends up producing the same result. Basically a loop where for each node we accumulate on the respective edges the values of the delays from the nodes respective ports plus the value of the edges themselves and finally we normalize everything to start at zero. I do not see any reason why this wouldn't work.
// TODO: Furthermode, encode the ability to push latency upwards or downwards and look into improving the circuits generated. Should also look into transforming the edge mapping from an Hashmap to an Array but still do not know if we need to map edges in this improved algorithm. (Techically we could make an auxiliary function that flattens everything and replaces edge lookups with indexes to the array.). 

// Instead of an accelerator, it could take a ordered list of instances, and potently the max amount of edges for the hashmap instantiation.
// Therefore abstracting from the accelerator completely and being able to be used for things like subgraphs.
// Change later if needed.

CalculateDelayResult CalculateDelay(Versat* versat,Accelerator* accel,Arena* out){
  // TODO: We are currently using the delay pointer inside the ConnectionNode structure. When correct, eventually change to just use the hashmap

  int nodes = Size(accel->allocated);
  int edges = Size(accel->edges);
  Hashmap<EdgeNode,int>* edgeToDelay = PushHashmap<EdgeNode,int>(out,edges);
  Hashmap<InstanceNode*,int>* nodeDelay = PushHashmap<InstanceNode*,int>(out,nodes);
  
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
  OrderedInstance* ptr = accel->ordered;
  for(; ptr; ptr = ptr->next){
    InstanceNode* node = ptr->node;

    if(node->type != InstanceNode::TAG_SOURCE && node->type != InstanceNode::TAG_SOURCE_AND_SINK){
      break; // This break is important because further code relies on it. 
    }

    nodeDelay->Insert(node,0);
    node->inst->baseDelay = 0;

    SendLatencyUpwards(node,edgeToDelay,nodeDelay);

    region(out){
      String filepath = PushString(out,"debug/%.*s/out1_%d.dot",UNPACK_SS(accel->name),graphs++);
      OutputGraphDotFile(versat,accel,true,node->inst,filepath,out);
    }
  }

  // Continue up the tree
  for(; ptr; ptr = ptr->next){
    InstanceNode* node = ptr->node;
    if(node->type == InstanceNode::TAG_UNCONNECTED){
      continue;
    }

    int maximum = -(1 << 30);
    FOREACH_LIST(ConnectionNode*,info,node->allInputs){
      maximum = std::max(maximum,*info->delay);
    }

    if(maximum == -(1 << 30)){
      continue;
    }
    
    FOREACH_LIST(ConnectionNode*,info,node->allInputs){
      *info->delay = maximum - *info->delay;
    }

    nodeDelay->Insert(node,maximum);
    node->inst->baseDelay = maximum;

    if(node->type != InstanceNode::TAG_SOURCE_AND_SINK){
      SendLatencyUpwards(node,edgeToDelay,nodeDelay);
    }

    region(out){
      String filepath = PushString(out,"debug/%.*s/out2_%d.dot",UNPACK_SS(accel->name),graphs++);
      OutputGraphDotFile(versat,accel,true,node->inst,filepath,out);
    }
  }

  FOREACH_LIST(OrderedInstance*,ptr,accel->ordered){
    InstanceNode* node = ptr->node;

    if(node->type != InstanceNode::TAG_SOURCE_AND_SINK){
      continue;
    }

    // Source and sink units never have output delay. They can't
    FOREACH_LIST(ConnectionNode*,con,node->allOutputs){
      *con->delay = 0;
    }
  }

  int minimum = 0;
  FOREACH_LIST(OrderedInstance*,ptr,accel->ordered){
    InstanceNode* node = ptr->node;
    int delay = nodeDelay->GetOrFail(node);

    minimum = std::min(minimum,delay);
  }
  FOREACH_LIST(OrderedInstance*,ptr,accel->ordered){
    InstanceNode* node = ptr->node;
    int* delay = nodeDelay->Get(node);
    *delay -= minimum;
  }
  
  region(out){
    String filepath = PushString(out,"debug/%.*s/out3_%d.dot",UNPACK_SS(accel->name),graphs++);
    OutputGraphDotFile(versat,accel,true,filepath,out);
  }

  if(!versat->opts->noDelayPropagation){
    // Normalizes everything to start on zero
    FOREACH_LIST(OrderedInstance*,ptr,accel->ordered){
      InstanceNode* node = ptr->node;

      if(node->type != InstanceNode::TAG_SOURCE && node->type != InstanceNode::TAG_SOURCE_AND_SINK){
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
      node->inst->baseDelay = minimum;

      FOREACH_LIST(ConnectionNode*,info,node->allOutputs){
        *info->delay -= minimum;
      }
    }
  }

  region(out){
    String filepath = PushString(out,"debug/%.*s/out4_%d.dot",UNPACK_SS(accel->name),graphs++);
    OutputGraphDotFile(versat,accel,true,filepath,out);
  }

  FOREACH_LIST(OrderedInstance*,ptr,accel->ordered){
    InstanceNode* node = ptr->node;

    if(node->type == InstanceNode::TAG_UNCONNECTED){
      nodeDelay->Insert(node,0);
    } else if(node->type == InstanceNode::TAG_SOURCE_AND_SINK){
    }
    node->inst->baseDelay = nodeDelay->GetOrFail(node);
  }
  
  CalculateDelayResult res = {};
  res.edgesDelay = edgeToDelay;
  res.nodeDelay = nodeDelay;
  
  return res;
}

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

  if(node->type == InstanceNode::TAG_SINK ||
     (node->type == InstanceNode::TAG_SOURCE_AND_SINK && CHECK_DELAY(inst,DelayType::DELAY_TYPE_SINK_DELAY))){
    return 0;
  }

  *tag = TAG_TEMPORARY_MARK;

  int count = 0;
  if(node->type == InstanceNode::TAG_COMPUTE){
    FOREACH_LIST(ConnectionNode*,ptr,node->allInputs){
      count += Visit(ordering,ptr->instConnectedTo.node,tags);
    }
  }

  *tag = TAG_PERMANENT_MARK;

  if(node->type == InstanceNode::TAG_COMPUTE){
    *ordering->Push(1) = node;
    count += 1;
  }

  return count;
}

DAGOrderNodes CalculateDAGOrder(InstanceNode* instances,Arena* out){
  int size = Size(instances);

  DAGOrderNodes res = {};

  res.numberComputeUnits = 0;
  res.numberSinks = 0;
  res.numberSources = 0;
  res.size = 0;
  res.instances = PushArray<InstanceNode*>(out,size).data;
  res.order = PushArray<int>(out,size).data;

  PushPtr<InstanceNode*> pushPtr = {};
  pushPtr.Init(res.instances,size);

  BLOCK_REGION(out);

  Hashmap<InstanceNode*,int>* tags = PushHashmap<InstanceNode*,int>(out,size);

  FOREACH_LIST(InstanceNode*,ptr,instances){
    res.size += 1;
    tags->Insert(ptr,0);
  }

  res.sources = pushPtr.Push(0);
  // Add source units, guaranteed to come first
  FOREACH_LIST(InstanceNode*,ptr,instances){
    FUInstance* inst = ptr->inst;
    if(ptr->type == InstanceNode::TAG_SOURCE || (ptr->type == InstanceNode::TAG_SOURCE_AND_SINK && CHECK_DELAY(inst,DelayType::DELAY_TYPE_SOURCE_DELAY))){
      *pushPtr.Push(1) = ptr;
      res.numberSources += 1;

      tags->Insert(ptr,TAG_PERMANENT_MARK);
    }
  }

  // Add compute units
  res.computeUnits = pushPtr.Push(0);
  FOREACH_LIST(InstanceNode*,ptr,instances){
    if(ptr->type == InstanceNode::TAG_UNCONNECTED){
      *pushPtr.Push(1) = ptr;
      res.numberComputeUnits += 1;
      tags->Insert(ptr,TAG_PERMANENT_MARK);
    } else {
      int tag = tags->GetOrFail(ptr);
      if(tag == 0 && ptr->type == InstanceNode::TAG_COMPUTE){
        res.numberComputeUnits += Visit(&pushPtr,ptr,tags);
      }
    }
  }

  // Add sink units
  res.sinks = pushPtr.Push(0);
  FOREACH_LIST(InstanceNode*,ptr,instances){
    FUInstance* inst = ptr->inst;
    if(ptr->type == InstanceNode::TAG_SINK || (ptr->type == InstanceNode::TAG_SOURCE_AND_SINK && CHECK_DELAY(inst,DelayType::DELAY_TYPE_SINK_DELAY))){
      *pushPtr.Push(1) = ptr;
      res.numberSinks += 1;

      int* tag = tags->Get(ptr);
      Assert(*tag == 0);

      *tag = TAG_PERMANENT_MARK;
    }
  }

  FOREACH_LIST(InstanceNode*,ptr,instances){
    int tag = tags->GetOrFail(ptr);
    Assert(tag == TAG_PERMANENT_MARK);
  }

  Assert(res.numberSources + res.numberComputeUnits + res.numberSinks == size);

  // Reuse tags hashmap
  Hashmap<InstanceNode*,int>* orderIndex = tags;
  orderIndex->Clear();

  res.maxOrder = 0;
  for(int i = 0; i < size; i++){
    InstanceNode* node = res.instances[i];

    int order = 0;
    FOREACH_LIST(ConnectionNode*,ptr,node->allInputs){
      InstanceNode* other = ptr->instConnectedTo.node;

      if(other->type == InstanceNode::TAG_SOURCE_AND_SINK){
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

struct HuffmanBlock{
  int bits;
  FUInstance* instance; // TODO: Maybe add the instance index (on the list) so we can push to the left instances that appear first and make it easier to see the mapping taking place
  HuffmanBlock* left;
  HuffmanBlock* right;
  enum {LEAF,NODE} type;
};

bool IsUnitCombinatorial(FUInstance* instance){
  FUDeclaration* type = instance->declaration;

  if(type->outputLatencies.size && type->outputLatencies[0] == 0){
    return true;
  } else {
    return false;
  }
}

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

#if 0
static int CalculateLatency_(PortInstance portInst, std::unordered_map<PortInstance,int>* memoization,bool seenSourceAndSink){
  FUInstance* inst = portInst.inst;
  int port = portInst.port;

  if(inst->graphData->nodeType == InstanceNode::TAG_SOURCE_AND_SINK && seenSourceAndSink){
    return inst->declaration->latencies[port];
  } else if(inst->graphData->nodeType == InstanceNode::TAG_SOURCE){
    return inst->declaration->latencies[port];
  } else if(inst->graphData->nodeType == InstanceNode::TAG_UNCONNECTED){
    Assert(false);
  }

  auto iter = memoization->find(portInst);

  if(iter != memoization->end()){
    return iter->second;
  }

  int latency = 0;
  for(int i = 0; i < inst->graphData->numberInputs; i++){
    ConnectionInfo* info = &inst->graphData->inputs[i];

    int lat = CalculateLatency_(info->instConnectedTo,memoization,true);

    Assert(inst->declaration->inputDelays[i] < 1000);

    latency = std::max(latency,lat);
  }

  int finalLatency = latency + inst->declaration->latencies[port];
  memoization->insert({portInst,finalLatency});

  return finalLatency;
}

int CalculateLatency(FUInstance* inst){
  std::unordered_map<PortInstance,int> map;

  Assert(inst->graphData->nodeType == InstanceNode::TAG_SOURCE_AND_SINK || inst->graphData->nodeType == InstanceNode::TAG_SINK);

  int maxLatency = 0;
  for(int i = 0; i < inst->graphData->numberInputs; i++){
    ConnectionInfo* info = &inst->graphData->inputs[i];
    int latency = CalculateLatency_(info->instConnectedTo,&map,false);
    maxLatency = std::max(maxLatency,latency);
  }

  return maxLatency;
}
#endif

#if 1
void FixDelays(Versat* versat,Accelerator* accel,Hashmap<EdgeNode,int>* edgeDelays){
  Arena* temp = &versat->temp;
  BLOCK_REGION(temp);

  int buffersInserted = 0;
  for(auto& edgePair : edgeDelays){
    EdgeNode edge = edgePair.first;
    int delay = edgePair.second;

    if(delay == 0){
      continue;
    }

    InstanceNode* output = edge.node0.node;

    if(output->inst->declaration == BasicDeclaration::buffer){
      output->inst->baseDelay = delay;
      edgePair.second = 0;
      continue;
    }
    // TODO: Fixed buffer

    Assert(delay > 0); // Cannot deal with negative delays at this stage.

    FUInstance* buffer = nullptr;
    if(versat->opts->useFixedBuffers){
      String bufferName = PushString(&versat->permanent,"fixedBuffer%d",buffersInserted);

      buffer = (FUInstance*) CreateFUInstance(accel,BasicDeclaration::fixedBuffer,bufferName);
      buffer->bufferAmount = delay - BasicDeclaration::fixedBuffer->outputLatencies[0];
      buffer->parameters = PushString(&versat->permanent,"#(.AMOUNT(%d))",buffer->bufferAmount);
    } else {
      String bufferName = PushString(&versat->permanent,"buffer%d",buffersInserted);

      buffer = (FUInstance*) CreateFUInstance(accel,BasicDeclaration::buffer,bufferName);
      buffer->bufferAmount = delay - BasicDeclaration::buffer->outputLatencies[0];
      Assert(buffer->bufferAmount >= 0);
      SetStatic(accel,buffer);
      
#if 0
      if(buffer->config){
        buffer->config[0] = buffer->bufferAmount;
      }
#endif
    }

    InsertUnit(accel,edge.node0,edge.node1,PortNode{GetInstanceNode(accel,buffer),0});

    String filePath = PushString(temp,"debug/%.*s/fixDelay_%d.dot",UNPACK_SS(accel->name),buffersInserted);
    OutputGraphDotFile(versat,accel,true,buffer,filePath,temp);
    buffersInserted += 1;
  }
}
#endif

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

InstanceNode* GetOutputNode(InstanceNode* nodes){
  FOREACH_LIST(InstanceNode*,ptr,nodes){
    FUInstance* inst = ptr->inst;
    if(inst->declaration == BasicDeclaration::output){
      return ptr;
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

ComputedData CalculateVersatComputedData(Array<InstanceInfo> info,VersatComputedValues val,Arena* out){
  Array<ExternalMemoryInterface> external = PushArray<ExternalMemoryInterface>(out,val.externalMemoryInterfaces);
  int index = 0;
  int externalIndex = 0;
  int memoryMapped = 0;
  for(InstanceInfo& in : info){
#if 0
    if(in.level != 0){
      continue;
    }
#endif
    
    if(!in.isComposite){
      for(ExternalMemoryInterface& inter : in.decl->externalMemory){
        external[externalIndex++] = inter;
      }
    }

    if(in.decl->memoryMapBits.has_value()){
      memoryMapped += 1;
    }
  }

  Array<VersatComputedData> data = PushArray<VersatComputedData>(out,memoryMapped);

  index = 0;
  for(InstanceInfo& in : info){
    if(in.level != 0){
      continue;
    }

    if(in.decl->memoryMapBits.has_value()){
      FUDeclaration* decl = in.decl;
      iptr offset = (iptr) in.memMapped.value();
      iptr mask = offset >> decl->memoryMapBits.value();

#if 0
      FUDeclaration* parent = in.parentDeclaration;
      iptr maskSize;
      if(parent){
        maskSize = parent->memoryMapBits.value() - decl->memoryMapBits.value();
      } else {
        maskSize = 32 - decl->memoryMapBits.value(); // TODO: The 32 is the ADDR_W. Need to receive as argument.
      }
#endif
      
      iptr maskSize = val.memoryAddressBits - log2i(in.memMappedSize.value());
        
      data[index].memoryMask = data[index].memoryMaskBuffer;
      memset(data[index].memoryMask,0,32);
      data[index].memoryMaskSize = maskSize;

      for(int i = 0; i < maskSize; i++){
        Assert(maskSize - i - 1 >= 0);
        data[index].memoryMask[i] = GET_BIT(mask,(maskSize - i - 1)) ? '1' : '0';
      }
      index += 1;
    }
  }

  ComputedData res = {};
  res.external = external;
  res.data = data;

  return res;
}

ComputedData CalculateVersatComputedData(InstanceNode* instances,VersatComputedValues val,Arena* out){
  Array<ExternalMemoryInterface> external = PushArray<ExternalMemoryInterface>(out,val.externalMemoryInterfaces);
  int index = 0;
  int externalIndex = 0;
  int memoryMapped = 0;
  FOREACH_LIST(InstanceNode*,ptr,instances){
    for(ExternalMemoryInterface& inter : ptr->inst->declaration->externalMemory){
      external[externalIndex++] = inter;
    }

    if(ptr->inst->declaration->memoryMapBits.has_value()){
      memoryMapped += 1;
    }
  }

  Array<VersatComputedData> data = PushArray<VersatComputedData>(out,memoryMapped);

  index = 0;
  FOREACH_LIST(InstanceNode*,ptr,instances){
    if(ptr->inst->declaration->memoryMapBits.has_value()){
      FUDeclaration* decl = ptr->inst->declaration;
      iptr offset = (iptr) ptr->inst->memMapped;
      iptr mask = offset >> decl->memoryMapBits.value();

      iptr maskSize = val.memoryAddressBits - decl->memoryMapBits.value();

      data[index].memoryMask = data[index].memoryMaskBuffer;
      memset(data[index].memoryMask,0,32);
      data[index].memoryMaskSize = maskSize;

      for(int i = 0; i < maskSize; i++){
        Assert(maskSize - i - 1 >= 0);
        data[index].memoryMask[i] = GET_BIT(mask,(maskSize - i - 1)) ? '1' : '0';
      }
      index += 1;
    }
  }

  ComputedData res = {};
  res.external = external;
  res.data = data;

  return res;
}

bool IsComposite(FUDeclaration* decl){
  bool res = (decl->baseCircuit != nullptr);
  return res;
}

bool IsCombinatorial(FUDeclaration* decl){
  bool res;
  if(IsTypeHierarchical(decl)){
    res = IsCombinatorial(decl->fixedDelayCircuit);
  } else {
    int maxLatency = 0;
    for(int val : decl->outputLatencies){
      maxLatency = std::max(maxLatency,val);
    }

    res = (decl->isOperation || (maxLatency != 0));
  }
  return res;
}

bool IsCombinatorial(Accelerator* accel){
  FOREACH_LIST(InstanceNode*,ptr,accel->allocated){
    bool isComb = IsCombinatorial(ptr->inst->declaration);
    if(!isComb){
      return false;
    }
  }
  return true;
}

bool ContainsConfigs(FUDeclaration* decl){
  bool res = (decl->configInfo.configs.size);
  return res;
}

bool ContainsStatics(FUDeclaration* decl){
  bool res = false;
  for(auto& pair : decl->staticUnits){
    if(pair.data.configs.size){
      res = true;
      break;
    }
  }

  return res;
}

Array<FUDeclaration*> ConfigSubTypes(Accelerator* accel,Arena* out,Arena* temp){
  if(accel == nullptr){
    return {};
  }
  
  BLOCK_REGION(temp);
  
  Set<FUDeclaration*>* maps = PushSet<FUDeclaration*>(temp,99);
  
  Array<InstanceInfo> test = TransformGraphIntoArray(accel,true,temp,out);
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

  Array<InstanceInfo> test = TransformGraphIntoArray(accel,true,temp,out);
  for(InstanceInfo& info : test){
    if(info.memMappedSize.has_value()){
      maps->Insert(info.decl);
    }
  }
  
  Array<FUDeclaration*> subTypes = PushArrayFromSet(out,maps);
  return subTypes;
}



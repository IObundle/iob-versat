#include "accelerator.hpp"
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
#include "textualRepresentation.hpp"

#define TAG_TEMPORARY_MARK 1
#define TAG_PERMANENT_MARK 2

static Pool<Accelerator> accelerators;

typedef Hashmap<FUInstance*,FUInstance*> InstanceMap;

Accelerator* GetAcceleratorById(int id){
  for(Accelerator* accel : accelerators){
    if(accel->id == id){
      return accel;
    }
  }

  return nullptr;
}

Accelerator* CreateAccelerator(String name,AcceleratorPurpose purpose){
  static int globalID = 0;
  Accelerator* accel = accelerators.Alloc();
  
  accel->accelMemory = CreateDynamicArena(1);
  accel->id = globalID++;
  
  accel->name = PushString(accel->accelMemory,name);
  accel->purpose = purpose;
  PushString(accel->accelMemory,{"",1});
  
  return accel;
}

#if 0
// For now hack it away.
// Deal with the falout later
void LockAccelerator(Accelerator* accel){
  if(accel->locked.size > 0){
    return;
  }

  int size = Size(accel->allocated);
  
  accel->locked = PushArray<FUInstance*>(accel->accelMemory,size);

  int index = 0;
  FOREACH_LIST_INDEXED(FUInstance*,inst,accel->allocated,index){
    accel->locked[index] = inst;
  }
}
#endif

bool NameExists(Accelerator* accel,String name){
  FOREACH_LIST(FUInstance*,ptr,accel->allocated){
    if(ptr->name == name){
      return true;
    }
  }

  return false;
}

int GetFreeShareIndex(Accelerator* accel){  // TODO: Slow 
  int attempt = 0; 
  while(1){
    bool found = true;
    FOREACH_LIST(FUInstance*,inst,accel->allocated){
      if(inst->sharedEnable && inst->sharedIndex == attempt){
        attempt += 1;
        found = false;
        break;
      }
    }

    if(found){
      return attempt;
    }
  }
}

FUInstance* CreateFUInstance(Accelerator* accel,FUDeclaration* type,String name){
  static int globalID = 0;
  String storedName = PushString(accel->accelMemory,name);

  Assert(CheckValidName(storedName));

  FUInstance* inst = PushStruct<FUInstance>(accel->accelMemory);

  inst->inputs = PushArray<PortInstance>(accel->accelMemory,type->NumberInputs());
  inst->outputs = PushArray<bool>(accel->accelMemory,type->NumberOutputs());

  if(accel->lastAllocated){
    Assert(accel->lastAllocated->next == nullptr);
    accel->lastAllocated->next = inst;
  } else {
    accel->allocated = inst;
  }
  accel->lastAllocated = inst;

  inst->name = storedName;
  inst->accel = accel;
  inst->declaration = type;
  inst->id = globalID++;
  
  return inst;
}

Pair<Accelerator*,AcceleratorMapping*> CopyAcceleratorWithMapping(Accelerator* accel,AcceleratorPurpose purpose,bool preserveIds,Arena* out){
  Accelerator* newAccel = CreateAccelerator(accel->name,purpose);

  AcceleratorMapping* map = MappingSimple(accel,newAccel,out);
  
  // Copy of instances
  FOREACH_LIST(FUInstance*,inst,accel->allocated){
    FUInstance* newInst = CopyInstance(newAccel,inst,preserveIds,inst->name);

    newInst->literal = inst->literal;

    if(inst->sharedEnable){
      ShareInstanceConfig(newInst,inst->sharedIndex);
    }
    if(inst->isStatic){
      newInst->isStatic = inst->isStatic;
    }
    newInst->isMergeMultiplexer = inst->isMergeMultiplexer;
    newInst->mergeMultiplexerId = inst->mergeMultiplexerId;
    
    MappingInsertEqualNode(map,inst,newInst);
  }

  EdgeIterator iter = IterateEdges(accel);

  // Flat copy of edges
  while(iter.HasNext()){
    Edge edge = iter.Next();

    PortInstance out = MappingMapOutput(map,edge.out);
    PortInstance in = MappingMapInput(map,edge.in);
    
    ConnectUnits(out.inst,out.port,in.inst,in.port,edge.delay);
  }
  
  return {newAccel,map};
}

Accelerator* CopyAccelerator(Accelerator* accel,AcceleratorPurpose purpose,bool preserveIds,InstanceMap* map){
  STACK_ARENA(temp,Kilobyte(128));

  Accelerator* newAccel = CreateAccelerator(accel->name,purpose);

  if(map == nullptr){
    map = PushHashmap<FUInstance*,FUInstance*>(&temp,999);
  }

  // Copy of instances
  FOREACH_LIST(FUInstance*,ptr,accel->allocated){
    FUInstance* inst = ptr;
    FUInstance* newInst = CopyInstance(newAccel,inst,preserveIds,inst->name);

    newInst->literal = inst->literal;
    if(inst->sharedEnable){
      ShareInstanceConfig(newInst,inst->sharedIndex);
    }
    if(inst->isStatic){
      newInst->isStatic = inst->isStatic;
    }
    newInst->isMergeMultiplexer = inst->isMergeMultiplexer;
    newInst->mergeMultiplexerId = inst->mergeMultiplexerId;
    
    map->Insert(inst,newInst);
  }

  EdgeIterator iter = IterateEdges(accel);

  // Flat copy of edges
  while(iter.HasNext()){
    Edge edge = iter.Next();
    FUInstance* out = map->GetOrFail(edge.units[0].inst);
    int outPort = edge.units[0].port;
    FUInstance* in = map->GetOrFail(edge.units[1].inst);
    int inPort = edge.units[1].port;
    
    ConnectUnits(out,outPort,in,inPort,edge.delay);
  }
  
  return newAccel;
}

FUInstance* CopyInstance(Accelerator* newAccel,FUInstance* oldInstance,bool preserveIds,String newName){
  FUInstance* newInst = CreateFUInstance(newAccel,oldInstance->declaration,newName);

  newInst->parameters = oldInstance->parameters;
  newInst->portIndex = oldInstance->portIndex;
  
  if(preserveIds){
    newInst->id = oldInstance->id;
  }
  newInst->isMergeMultiplexer = oldInstance->isMergeMultiplexer;
  newInst->mergeMultiplexerId = oldInstance->mergeMultiplexerId;
  
  return newInst;
}

void RemoveFUInstance(Accelerator* accel,FUInstance* inst){
  accel->allocated = RemoveUnit(accel->allocated,inst);
}

Array<int> ExtractInputDelays(Accelerator* accel,CalculateDelayResult delays,int mimimumAmount,Arena* out,Arena* temp){
  Array<FUInstance*> inputNodes = PushArray<FUInstance*>(temp,99);
  Memset(inputNodes,{});
  
  bool seenOneInput = false;
  int maxInput = 0;
  FOREACH_LIST(FUInstance*,ptr,accel->allocated){
    if(ptr->declaration == BasicDeclaration::input){
      maxInput = std::max(maxInput,ptr->portIndex);
      inputNodes[ptr->portIndex] = ptr;
      seenOneInput = true;
    }
  }
  if(seenOneInput) maxInput += 1;
  inputNodes.size = std::max(maxInput,mimimumAmount);
  
  Array<int> inputDelay = PushArray<int>(out,inputNodes.size);

  for(int i = 0; i < inputDelay.size; i++){
    FUInstance* node = inputNodes[i];
    
    if(node){
      inputDelay[i] = delays.nodeDelay->GetOrFail(node).value;
    } else {
      inputDelay[i] = 0;
    }
  }

  return inputDelay;
}

Array<int> ExtractOutputLatencies(Accelerator* accel,CalculateDelayResult delays,Arena* out,Arena* temp){
  FUInstance* outputNode = nullptr;
  int maxOutput = 0;
  FOREACH_LIST(FUInstance*,ptr,accel->allocated){
    if(ptr->declaration == BasicDeclaration::output){
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
      PortInstance end = {.inst = outputNode,.port = port};

      outputLatencies[port] = delays.portDelay->GetOrFail(end).value;
    }
  }

  return outputLatencies;
}

Accelerator* Flatten(Accelerator* accel,int times,Arena* temp){
  Arena* perm = globalPermanent;
  BLOCK_REGION(temp);

  Pair<Accelerator*,AcceleratorMapping*> pair = CopyAcceleratorWithMapping(accel,AcceleratorPurpose_FLATTEN,true,temp);
  Accelerator* newAccel = pair.first;
  
  Pool<FUInstance*> compositeInstances = {};
  Pool<FUInstance*> toRemove = {};
  std::unordered_map<StaticId,int> staticToIndex;

  for(int i = 0; i < times; i++){
    FOREACH_LIST(FUInstance*,instPtr,newAccel->allocated){
      FUInstance* inst = instPtr;

      bool containsStatic = false;
      containsStatic = inst->isStatic || inst->declaration->staticUnits != nullptr;
      // TODO: For now we do not flatten units that are static or contain statics. It messes merge configurations and still do not know how to procceed. Need to beef merge up a bit before handling more complex cases. The problem was that after flattening statics would become shared (unions) and as such would appear in places where they where not supposed to appear.

      // TODO: Maybe static and shared units configurations could be stored in a separated structure.
      if(inst->declaration->baseCircuit && !containsStatic){
        FUInstance** ptr = compositeInstances.Alloc();

        *ptr = instPtr;
      }
    }

    if(compositeInstances.Size() == 0){
      break;
    }

    std::unordered_map<int,int> sharedToFirstChildIndex;

    int freeSharedIndex = GetFreeShareIndex(newAccel); //(maxSharedIndex != -1 ? maxSharedIndex + 1 : 0);
    int count = 0;
    for(FUInstance** instPtr : compositeInstances){
      FUInstance* inst = (*instPtr);

      Assert(inst->declaration->baseCircuit);

      count += 1;
      Accelerator* circuit = inst->declaration->baseCircuit;
      FUInstance* outputInstance = GetOutputInstance(circuit->allocated);

      int savedSharedIndex = freeSharedIndex;
      int instSharedIndex = freeSharedIndex;
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
      AcceleratorMapping* map = MappingSimple(circuit,newAccel,temp); // TODO: Leaking

      FOREACH_LIST(FUInstance*,ptr,circuit->allocated){
        FUInstance* circuitInst = ptr;
        if(circuitInst->declaration->type == FUDeclarationType_SPECIAL){
          continue;
        }

        String newName = PushString(perm,"%.*s_%.*s",UNPACK_SS(inst->name),UNPACK_SS(circuitInst->name));
        FUInstance* newInst = CopyInstance(newAccel,circuitInst,true,newName);
        MappingInsertEqualNode(map,circuitInst,newInst);

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
            shareIndex = GetFreeShareIndex(newAccel);

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
            int newIndex = GetFreeShareIndex(newAccel);

            sharedToShared.insert({circuitInst->sharedIndex,newIndex});

            ShareInstanceConfig(newInst,newIndex);
          }
        } else if(inst->sharedEnable){ // Currently flattening instance is shared
          ShareInstanceConfig(newInst,instSharedIndex); // Always use the same inst index
          //freeSharedIndex = GetFreeShareIndex(newAccel);
        } else if(circuitInst->sharedEnable){
          auto ptr = sharedToShared.find(circuitInst->sharedIndex);

          if(ptr != sharedToShared.end()){
            ShareInstanceConfig(newInst,ptr->second);
          } else {
            int newIndex = freeSharedIndex;
            
            sharedToShared.insert({circuitInst->sharedIndex,newIndex});

            ShareInstanceConfig(newInst,newIndex);
            freeSharedIndex = GetFreeShareIndex(newAccel);
          }
        }
      }

      if(inst->sharedEnable && savedSharedIndex > freeSharedIndex){
        freeSharedIndex = savedSharedIndex;
      }

      // TODO: All these iterations could be converted to a single loop where inside we check if its input,output or inernal edge

      EdgeIterator iter = IterateEdges(newAccel);
      // Add accel edges to output instances
      while(iter.HasNext()){
        Edge edgeInst = iter.Next();
        Edge* edge = &edgeInst;
        if(edge->units[0].inst == inst){
          EdgeIterator iter2 = IterateEdges(circuit);
          while(iter2.HasNext()){
            Edge edge2Inst = iter2.Next();
            Edge* circuitEdge = &edge2Inst;

            if(circuitEdge->units[1].inst == outputInstance && circuitEdge->units[1].port == edge->units[0].port){
              FUInstance* other = MappingMapOutput(map,circuitEdge->units[0]).inst;

              if(other == nullptr){
                continue;
              }

              FUInstance* out = other;
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
      iter = IterateEdges(newAccel);
      while(iter.HasNext()){
        Edge edgeInst = iter.Next();
        Edge* edge = &edgeInst;
        if(edge->units[1].inst == inst){
          FUInstance* circuitInst = GetInputInstance(circuit->allocated,edge->units[1].port);

          EdgeIterator iter2 = IterateEdges(circuit);
          while(iter2.HasNext()){
            Edge edge2Inst = iter2.Next();
            Edge* circuitEdge = &edge2Inst;
          
            if(circuitEdge->units[0].inst == circuitInst){
              FUInstance* other = MappingMapInput(map,circuitEdge->units[1]).inst;

              if(other == nullptr){
                continue;
              }

              FUInstance* out = edge->units[0].inst;
              FUInstance* in = other;
              int outPort = edge->units[0].port;
              int inPort = circuitEdge->units[1].port;
              int delay = edge->delay + circuitEdge->delay;

              ConnectUnits(out,outPort,in,inPort,delay);
            }
          }
        }
      }

      // Add circuit specific edges
      iter = IterateEdges(circuit);
      while(iter.HasNext()){
        Edge edgeInst = iter.Next();
        Edge* circuitEdge = &edgeInst;

        FUInstance* otherInput = MappingMapOutput(map,circuitEdge->units[0]).inst;
        FUInstance* otherOutput = MappingMapInput(map,circuitEdge->units[1]).inst;

        if(otherInput == nullptr || otherOutput == nullptr){
          continue;
        }

        FUInstance* out = otherInput;
        FUInstance* in = otherOutput;
        int outPort = circuitEdge->units[0].port;
        int inPort = circuitEdge->units[1].port;
        int delay = circuitEdge->delay;

        ConnectUnits(out,outPort,in,inPort,delay);
      }

      // Add input to output specific edges
      iter = IterateEdges(newAccel);
      while(iter.HasNext()){
        Edge edge1Inst = iter.Next();
        Edge* edge1 = &edge1Inst;
        if(edge1->units[1].inst == inst){
          PortInstance input = edge1->units[0];
          FUInstance* circuitInput = GetInputInstance(circuit->allocated,edge1->units[1].port);

          EdgeIterator iter2 = IterateEdges(newAccel);
          while(iter2.HasNext()){
            Edge edge2Inst = iter2.Next();
            Edge* edge2 = &edge2Inst;
            if(edge2->units[0].inst == inst){
              PortInstance output = edge2->units[1];
              int outputPort = edge2->units[0].port;

              EdgeIterator iter3 = IterateEdges(circuit);
              while(iter3.HasNext()){
                Edge edge3Inst = iter3.Next();
                Edge* circuitEdge = &edge3Inst;

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
    }

    toRemove.Clear();
    compositeInstances.Clear();
  }

  OutputDebugDotGraph(newAccel,STRING("flatten.dot"),temp);
  
  toRemove.Clear(true);
  compositeInstances.Clear(true);

  FUDeclaration base = {};
  newAccel->name = accel->name;

  return newAccel;
}

// TODO: This functions should have actual error handling and reporting. Instead of just Asserting.
static int Visit(PushPtr<FUInstance*>* ordering,FUInstance* node,Hashmap<FUInstance*,int>* tags){
  int* tag = tags->Get(node);
  Assert(tag);

  if(*tag == TAG_PERMANENT_MARK){
    return 0;
  }
  if(*tag == TAG_TEMPORARY_MARK){
    Assert(0);
  }

  FUInstance* inst = node;

  if(node->type == NodeType_SINK ||
     (node->type == NodeType_SOURCE_AND_SINK && CHECK_DELAY(inst,DelayType::DelayType_SINK_DELAY))){
    return 0;
  }

  *tag = TAG_TEMPORARY_MARK;

  int count = 0;
  if(node->type == NodeType_COMPUTE){
    FOREACH_LIST(ConnectionNode*,ptr,node->allInputs){
      count += Visit(ordering,ptr->instConnectedTo.inst,tags);
    }
  }

  *tag = TAG_PERMANENT_MARK;

  if(node->type == NodeType_COMPUTE){
    *ordering->Push(1) = node;
    count += 1;
  }

  return count;
}

DAGOrderNodes CalculateDAGOrder(FUInstance* instances,Arena* out){
  int size = Size(instances);

  DAGOrderNodes res = {};

  res.size = 0;
  res.instances = PushArray<FUInstance*>(out,size);
  res.order = PushArray<int>(out,size);

  PushPtr<FUInstance*> pushPtr = {};
  pushPtr.Init(res.instances);

  Hashmap<FUInstance*,int>* tags = PushHashmap<FUInstance*,int>(out,size);

  FOREACH_LIST(FUInstance*,ptr,instances){
    res.size += 1;
    tags->Insert(ptr,0);
  }

  int mark = pushPtr.Mark();
  // Add source units, guaranteed to come first
  FOREACH_LIST(FUInstance*,ptr,instances){
    FUInstance* inst = ptr;
    if(ptr->type == NodeType_SOURCE || (ptr->type == NodeType_SOURCE_AND_SINK && CHECK_DELAY(inst,DelayType::DelayType_SOURCE_DELAY))){
      *pushPtr.Push(1) = ptr;

      tags->Insert(ptr,TAG_PERMANENT_MARK);
    }
  }
  res.sources = pushPtr.PopMark(mark);

  // Add compute units
  mark = pushPtr.Mark();
  FOREACH_LIST(FUInstance*,ptr,instances){
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
  FOREACH_LIST(FUInstance*,ptr,instances){
    FUInstance* inst = ptr;
    if(ptr->type == NodeType_SINK || (ptr->type == NodeType_SOURCE_AND_SINK && CHECK_DELAY(inst,DelayType::DelayType_SINK_DELAY))){
      *pushPtr.Push(1) = ptr;

      int* tag = tags->Get(ptr);
      Assert(*tag == 0);

      *tag = TAG_PERMANENT_MARK;
    }
  }
  res.sinks = pushPtr.PopMark(mark);

  FOREACH_LIST(FUInstance*,ptr,instances){
    int tag = tags->GetOrFail(ptr);
    Assert(tag == TAG_PERMANENT_MARK);
  }

  Assert(res.sources.size + res.computeUnits.size + res.sinks.size == size);

  // Reuse tags hashmap
  Hashmap<FUInstance*,int>* orderIndex = tags;
  orderIndex->Clear();

  res.maxOrder = 0;
  for(int i = 0; i < size; i++){
    FUInstance* node = res.instances[i];

    int order = 0;
    FOREACH_LIST(ConnectionNode*,ptr,node->allInputs){
      FUInstance* other = ptr->instConnectedTo.inst;

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
    FUInstance* ptr = order.instances[i];

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
  Hashmap<FUInstance*,int>* order = PushHashmap<FUInstance*,int>(temp,size);
  Hashmap<FUInstance*,bool>* seen = PushHashmap<FUInstance*,bool>(temp,size);

  FOREACH_LIST(FUInstance*,ptr,accel->allocated){
    seen->Insert(ptr,false);
    order->Insert(ptr,size - 1);
  }

  // Start by seeing all inputs
  FOREACH_LIST(FUInstance*,ptr,accel->allocated){
    if(ptr->allInputs == nullptr){
      seen->Insert(ptr,true);
      order->Insert(ptr,0);
    }
  }

  FOREACH_LIST(FUInstance*,outerPtr,accel->allocated){
    FOREACH_LIST(FUInstance*,ptr,accel->allocated){
      bool allInputsSeen = true;
      FOREACH_LIST(ConnectionNode*,input,ptr->allInputs){
        FUInstance* node = input->instConnectedTo.node;
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
        FUInstance* node = input->instConnectedTo.node;
        maxOrder = std::max(maxOrder,order->GetOrFail(node));
      }

      order->Insert(ptr,maxOrder + 1);
    }
  }

  Array<FUInstance*> nodes = PushArray<FUInstance*>(temp,size);
  int index = 0;
  for(int i = 0; i < size; i++){
    FOREACH_LIST(FUInstance*,ptr,accel->allocated){
      int ord = order->GetOrFail(ptr);
      if(ord == i){
        nodes[index++] = ptr;
      }
    }
  }

  OrderedInstance* ordered = nullptr;
  for(int i = 0; i < size; i++){
    FUInstance* ptr = nodes[i];

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

Array<DelayToAdd> GenerateFixDelays(Accelerator* accel,EdgeDelay* edgeDelays,Arena* out,Arena* temp){
  BLOCK_REGION(temp);

  ArenaList<DelayToAdd>* list = PushArenaList<DelayToAdd>(temp);
  
  int buffersInserted = 0;
  for(auto edgePair : edgeDelays){
    Edge edge = edgePair.first;
    int delay = edgePair.second->value;

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

void FixDelays(Accelerator* accel,Hashmap<Edge,DelayInfo>* edgeDelays,Arena* temp){
  Arena* perm = globalPermanent;
  BLOCK_REGION(temp);

  int buffersInserted = 0;
  for(auto edgePair : edgeDelays){
    Edge edge = edgePair.first;
    int delay = edgePair.second->value;

    if(delay == 0){
      continue;
    }

    if(edge.units[1].inst->declaration == BasicDeclaration::output){
      continue;
    }
    
    FUInstance* output = edge.units[0].inst;

    if(output->declaration == BasicDeclaration::buffer){
      //output->inst->baseDelay = delay;
      edgePair.second = 0;
      continue;
    }
    Assert(delay > 0); // Cannot deal with negative delays at this stage.

    FUInstance* buffer = nullptr;
    if(globalOptions.useFixedBuffers){
      String bufferName = PushString(perm,"fixedBuffer%d",buffersInserted);

      buffer = CreateFUInstance(accel,BasicDeclaration::fixedBuffer,bufferName);
      buffer->bufferAmount = delay - BasicDeclaration::fixedBuffer->baseConfig.outputLatencies[0];
      buffer->parameters = PushString(perm,"#(.AMOUNT(%d))",buffer->bufferAmount);
    } else {
      String bufferName = PushString(perm,"buffer%d",buffersInserted);

      buffer = CreateFUInstance(accel,BasicDeclaration::buffer,bufferName);
      buffer->bufferAmount = delay - BasicDeclaration::buffer->baseConfig.outputLatencies[0];
      Assert(buffer->bufferAmount >= 0);
      SetStatic(accel,buffer);
    }

    InsertUnit(accel,edge.units[0],edge.units[1],PortInstance{buffer,0});

    OutputDebugDotGraph(accel,STRING(StaticFormat("fixDelay_%d.dot",buffersInserted)),buffer,temp);

    buffersInserted += 1;
  }
}

FUInstance* GetInputInstance(FUInstance* nodes,int inputIndex){
  FOREACH_LIST(FUInstance*,ptr,nodes){
    FUInstance* inst = ptr;
    if(inst->declaration == BasicDeclaration::input && inst->portIndex == inputIndex){
      return inst;
    }
  }
  return nullptr;
}

FUInstance* GetOutputInstance(FUInstance* nodes){
  FOREACH_LIST(FUInstance*,ptr,nodes){
    FUInstance* inst = ptr;
    if(inst->declaration == BasicDeclaration::output){
      return inst;
    }
  }

  return nullptr;
}

bool IsCombinatorial(Accelerator* accel){
  FOREACH_LIST(FUInstance*,ptr,accel->allocated){
    if(ptr->declaration->fixedDelayCircuit == nullptr){
      continue;
    }
    
    bool isComb = IsCombinatorial(ptr->declaration->fixedDelayCircuit);
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

Hashmap<StaticId,StaticData>* CollectStaticUnits(Accelerator* accel,FUDeclaration* topDecl,Arena* out){
  Hashmap<StaticId,StaticData>* staticUnits = PushHashmap<StaticId,StaticData>(out,999);

  FOREACH_LIST(FUInstance*,ptr,accel->allocated){
    FUInstance* inst = ptr;
    if(IsTypeHierarchical(inst->declaration)){
      for(auto pair : inst->declaration->staticUnits){
        StaticData newData = *pair.second;
        staticUnits->InsertIfNotExist(pair.first,newData);
      }
    }
    if(inst->isStatic){
      StaticId id = {};
      id.name = inst->name;
      id.parent = topDecl;

      StaticData data = {};
      data.configs = inst->declaration->baseConfig.configs;
      staticUnits->InsertIfNotExist(id,data);
    }
  }

  return staticUnits;
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
  FOREACH_LIST(FUInstance*,ptr,accel->allocated){
    FUInstance* inst = ptr;
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
  
  region(debugArena){
    Arena temp = SubArena(debugArena,debugArena->totalAllocated / 2);
    AccelInfo info = CalculateAcceleratorInfo(accel,true,debugArena,&temp);
    
    res.nStatics = info.statics;
    res.staticBits = info.staticBits;
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

#if 0
FUInstance* GetFUInstance(Accelerator* accel,FUInstance* inst){
  FOREACH_LIST(FUInstance*,ptr,accel->allocated){
    if(ptr == inst){
      return ptr;
    }
  }
  Assert(false); // For now, this should not fail
  return nullptr;
}
#endif

void CalculateNodeType(FUInstance* node){
  node->type = NodeType_UNCONNECTED;

  bool hasInput = (node->allInputs != nullptr);
  bool hasOutput = (node->allOutputs != nullptr);

  // If the unit is both capable of acting as a sink or as a source of data
  if(hasInput && hasOutput){
    if(CHECK_DELAY(node,DelayType_SINK_DELAY) || CHECK_DELAY(node,DelayType_SOURCE_DELAY)){
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

void FixInputs(FUInstance* node){
  Memset(node->inputs,{});
  node->multipleSamePortInputs = false;

  FOREACH_LIST(ConnectionNode*,ptr,node->allInputs){
    int port = ptr->port;

    if(node->inputs[port].inst){
      node->multipleSamePortInputs = true;
    }

    node->inputs[port] = ptr->instConnectedTo;
  }
}

void FixOutputs(FUInstance* node){
  Memset(node->outputs,false);

  FOREACH_LIST(ConnectionNode*,ptr,node->allOutputs){
    int port = ptr->port;
    node->outputs[port] = true;
  }
}

Array<Edge> GetAllEdges(Accelerator* accel,Arena* out){
  DynamicArray<Edge> arr = StartArray<Edge>(out);
  FOREACH_LIST(FUInstance*,ptr,accel->allocated){
    FOREACH_LIST(ConnectionNode*,con,ptr->allOutputs){
      Edge edge = {};
      edge.out.inst = ptr;
      edge.out.port = con->port;
      edge.in.inst = con->instConnectedTo.inst;
      edge.in.port = con->instConnectedTo.port;
      edge.delay = con->edgeDelay;
      *arr.PushElem() = edge;
    }
  }

  Array<Edge> result = EndArray(arr);
  
  return result;
}

static void AdvanceUntilValid(EdgeIterator* iter){
  if(iter->currentNode && iter->currentPort){
    return;
  }

  if(iter->currentNode == nullptr){
    Assert(iter->currentPort == nullptr);
    return;
  }
  
  while(iter->currentNode){
    if(iter->currentPort == nullptr){
      iter->currentNode = iter->currentNode->next;

      if(iter->currentNode){
        iter->currentPort = iter->currentNode->allOutputs;
        if(iter->currentPort){
          break;
        }
      }
    }
  }
}

bool Valid(Edge edge){
  return (edge.in.inst != nullptr);
}

bool EdgeIterator::HasNext(){
  if(currentNode == nullptr){
    Assert(currentPort == nullptr);
    return false;
  } else {
    return true;
  }
}

Edge EdgeIterator::Next(){
  if(!HasNext()){
    return {};
  }

  Edge edge = {};
  edge.out.inst = currentNode;
  edge.out.port = currentPort->port;
  edge.in.inst = currentPort->instConnectedTo.inst;
  edge.in.port = currentPort->instConnectedTo.port;
  edge.delay = currentPort->edgeDelay;

  currentPort = currentPort->next;
  AdvanceUntilValid(this);
  
  return edge;
}

EdgeIterator IterateEdges(Accelerator* accel){
  EdgeIterator iter = {};

  if(!accel->allocated){
    return iter;
  }

  iter.currentNode = accel->allocated;
  iter.currentPort = iter.currentNode->allOutputs;
  AdvanceUntilValid(&iter);
  
  return iter;
}

bool EdgeEqualNoDelay(const Edge& e0,const Edge& e1){
  bool res = (e0.in  == e1.in &&
              e0.out == e1.out);
  
  return res;
}

Opt<Edge> FindEdge(PortInstance out,PortInstance in){
  FUDeclaration* inDecl = in.inst->declaration;
  FUDeclaration* outDecl = out.inst->declaration;
  
  Assert(out.inst->accel == in.inst->accel);
  Assert(in.port < inDecl->NumberInputs());
  Assert(out.port < outDecl->NumberOutputs());

  Accelerator* accel = out.inst->accel;

  Edge toCheck = {.out = out,.in = in};
  
  EdgeIterator iter = IterateEdges(accel);
  while(iter.HasNext()){
    Edge edge = iter.Next();

    if(EdgeEqualNoDelay(edge,toCheck)){
      return edge;
    }
  }

  return {};
}

Opt<Edge> FindEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay){
  FUDeclaration* inDecl = in->declaration;
  FUDeclaration* outDecl = out->declaration;

  Assert(out->accel == in->accel);
  Assert(inIndex < inDecl->NumberInputs());
  Assert(outIndex < outDecl->NumberOutputs());

  Accelerator* accel = out->accel;

  EdgeIterator iter = IterateEdges(accel);
  while(iter.HasNext()){
    Edge edgeInst = iter.Next();
    Edge* edge = &edgeInst;

    if(edge->units[0].inst == out &&
       edge->units[0].port == outIndex &&
       edge->units[1].inst == in &&
       edge->units[1].port == inIndex &&
       edge->delay == delay) {
      return *edge;
    }
  }

  return {};
}

void ConnectUnitsGetEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay){
  ConnectUnitsGetEdge(out,outIndex,in,inIndex,delay,nullptr);
}

void ConnectUnits(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay){
  ConnectUnitsGetEdge(out,outIndex,in,inIndex,delay);
}

void ConnectUnitsIfNotConnected(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay){
  Accelerator* accel = out->accel;

  EdgeIterator iter = IterateEdges(accel);
  while(iter.HasNext()){
    Edge edgeInst = iter.Next();
    Edge* edge = &edgeInst;

    if(edge->units[0].inst == out && edge->units[0].port == outIndex &&
       edge->units[1].inst == in  && edge->units[1].port == inIndex &&
       delay == edge->delay){
      return;
    }
  }

  ConnectUnitsGetEdge(out,outIndex,in,inIndex,delay);
}

void ConnectUnits(PortInstance out,PortInstance in,int delay){
  FUDeclaration* inDecl = in.inst->declaration;
  FUDeclaration* outDecl = out.inst->declaration;

  Assert(out.inst->accel == in.inst->accel);
  Assert(in.port < inDecl->NumberInputs());
  Assert(out.port < outDecl->NumberOutputs());

  Assert(!FindEdge(out,in));
  Accelerator* accel = out.inst->accel;

  // Update graph data.
  FUInstance* inputNode = in.inst;
  FUInstance* outputNode = out.inst;

  // Add info to outputNode
  // Update all outputs
  {
    ConnectionNode* con = PushStruct<ConnectionNode>(accel->accelMemory);
    con->edgeDelay = delay;
    con->port = out.port;
    con->instConnectedTo.inst = inputNode;
    con->instConnectedTo.port = in.port;

    outputNode->allOutputs = ListInsert(outputNode->allOutputs,con);
    outputNode->outputs[out.port] = true;
  }

  // Add info to inputNode
  {
    ConnectionNode* con = PushStruct<ConnectionNode>(accel->accelMemory);
    con->edgeDelay = delay;
    con->port = in.port;
    con->instConnectedTo.inst = outputNode;
    con->instConnectedTo.port = out.port;

    inputNode->allInputs = ListInsert(inputNode->allInputs,con);

    if(inputNode->inputs[in.port].inst){
      inputNode->multipleSamePortInputs = true;
    }

    inputNode->inputs[in.port].inst = outputNode;
    inputNode->inputs[in.port].port = out.port;
  }

  CalculateNodeType(inputNode);
  CalculateNodeType(outputNode);
}

void PrintAllInstances(Accelerator* accel){
  FOREACH_LIST(FUInstance*,ptr,accel->allocated){
    printf("%.*s:%d\n",UNPACK_SS(ptr->name),ptr->id);
  }
}

// Connects out -> in
void ConnectUnitsGetEdge(FUInstance* out,int outIndex,FUInstance* in,int inIndex,int delay,Edge* previous){
  FUDeclaration* inDecl = in->declaration;
  FUDeclaration* outDecl = out->declaration;

  Assert(out->accel == in->accel);
  Assert(inIndex < inDecl->NumberInputs());
  Assert(outIndex < outDecl->NumberOutputs());

  Assert(!FindEdge({out,outIndex},{in,inIndex}));
  Accelerator* accel = out->accel;

  // Update graph data.
  FUInstance* inputNode = in;
  FUInstance* outputNode = out;

  // Add info to outputNode
  // Update all outputs
  {
    ConnectionNode* con = PushStruct<ConnectionNode>(accel->accelMemory);
    con->edgeDelay = delay;
    con->port = outIndex;
    con->instConnectedTo.inst = inputNode;
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
    con->instConnectedTo.inst = outputNode;
    con->instConnectedTo.port = outIndex;

    inputNode->allInputs = ListInsert(inputNode->allInputs,con);

    if(inputNode->inputs[inIndex].inst){
      inputNode->multipleSamePortInputs = true;
    }

    inputNode->inputs[inIndex].inst = outputNode;
    inputNode->inputs[inIndex].port = outIndex;
  }

  CalculateNodeType(inputNode);
  CalculateNodeType(outputNode);
}

Array<int> GetNumberOfInputConnections(FUInstance* node,Arena* out){
  Array<int> res = PushArray<int>(out,node->inputs.size);
  Memset(res,0);

  FOREACH_LIST(ConnectionNode*,ptr,node->allInputs){
    int port = ptr->port;
    res[port] += 1;
  }

  return res;
}

Array<Array<PortInstance>> GetAllInputs(FUInstance* node,Arena* out){
  Array<int> connections = GetNumberOfInputConnections(node,out); // Wastes a bit of memory because not deallocated after, its irrelevant

  Array<Array<PortInstance>> res = PushArray<Array<PortInstance>>(out,node->inputs.size);
  Memset(res,{});

  for(int i = 0; i < res.size; i++){
    res[i] = PushArray<PortInstance>(out,connections[i]);
  }

  FOREACH_LIST(ConnectionNode*,ptr,node->allInputs){
    int port = ptr->port;

    Array<PortInstance> array = res[port];
    int index = connections[port] - 1;
    connections[port] -= 1;
    array[index] = ptr->instConnectedTo;
  }

  return res;
}

void RemoveConnection(Accelerator* accel,PortInstance out,PortInstance in){
  RemoveConnection(accel,out.inst,out.port,in.inst,in.port);
}

void RemoveConnection(Accelerator* accel,FUInstance* out,int outPort,FUInstance* in,int inPort){
  out->allOutputs = ListRemoveAll(out->allOutputs,[&](ConnectionNode* n){
    bool res = (n->port == outPort && n->instConnectedTo.inst == in && n->instConnectedTo.port == inPort);
    return res;
  });
  //out->outputs = Size(out->allOutputs);
  FixOutputs(out);

  in->allInputs = ListRemoveAll(in->allInputs,[&](ConnectionNode* n){
    bool res = (n->port == inPort && n->instConnectedTo.inst == out && n->instConnectedTo.port == outPort);
    return res;
  });
  FixInputs(in);
}

void RemoveAllDirectedConnections(FUInstance* out,FUInstance* in){
  out->allOutputs = ListRemoveAll(out->allOutputs,[&](ConnectionNode* n){
    return (n->instConnectedTo.inst == in);
  });
  //out->outputs = Size(out->allOutputs);
  FixOutputs(out);

  in->allInputs = ListRemoveAll(in->allInputs,[&](ConnectionNode* n){
    return (n->instConnectedTo.inst == out);
  });

  FixInputs(in);
}

void RemoveAllConnections(FUInstance* n1,FUInstance* n2){
  RemoveAllDirectedConnections(n1,n2);
  RemoveAllDirectedConnections(n2,n1);
}

FUInstance* RemoveUnit(FUInstance* nodes,FUInstance* unit){
  STACK_ARENA(temp,Kilobyte(32));

  Hashmap<FUInstance*,int>* toRemove = PushHashmap<FUInstance*,int>(&temp,100);

  for(auto* ptr = unit->allInputs; ptr;){
    auto* next = ptr->next;

    toRemove->Insert(ptr->instConnectedTo.inst,0);

    ptr = next;
  }

  for(auto* ptr = unit->allOutputs; ptr;){
    auto* next = ptr->next;

    toRemove->Insert(ptr->instConnectedTo.inst,0);

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

void InsertUnit(Accelerator* accel,PortInstance before,PortInstance after,PortInstance newUnit){
  RemoveConnection(accel,before.inst,before.port,after.inst,after.port);
  ConnectUnits(newUnit,after,0);
  ConnectUnits(before,newUnit,0);
}

void AssertGraphValid(FUInstance* nodes,Arena* arena){
  BLOCK_REGION(arena);

  int size = Size(nodes);
  Hashmap<FUInstance*,int>* seen = PushHashmap<FUInstance*,int>(arena,size);
  Set<Pair<String,int>>* nameIdSeen = PushSet<Pair<String,int>>(arena,size);
  
  FOREACH_LIST(FUInstance*,ptr,nodes){
    seen->Insert(ptr,0);

    Assert(!nameIdSeen->ExistsOrInsert({ptr->name,ptr->id}));
  }

  FOREACH_LIST(FUInstance*,ptr,nodes){
    FOREACH_LIST(ConnectionNode*,con,ptr->allInputs){
      seen->GetOrFail(con->instConnectedTo.inst);
    }

    FOREACH_LIST(ConnectionNode*,con,ptr->allOutputs){
      seen->GetOrFail(con->instConnectedTo.inst);
    }

    for(PortInstance& n : ptr->inputs){
      if(n.inst){
        seen->GetOrFail(n.inst);
      }
    }
  }
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
      a = decl->configInfo[nodePart].outputLatencies[info->port];
    } else {
      a = inst->declaration->baseConfig.outputLatencies[info->port];
    }
    
    int e = info->edgeDelay;

    FOREACH_LIST(ConnectionNode*,otherInfo,other->allInputs){
      int c = 0;
      if(HasMultipleConfigs(other->declaration)){
        int otherPart = nodeToPart->GetOrFail(other);
        
        c = other->declaration->configInfo[otherPart].inputDelays[info->instConnectedTo.port];
      } else {
        c = other->declaration->baseConfig.inputDelays[info->instConnectedTo.port];
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

ConnectionNode* GetConnectionNode(ConnectionNode* head,int port,PortInstance other){
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

void MappingCheck(AcceleratorMapping* map){
  int idToCheckFirst = map->firstId;
  int idToCheckSecond = map->secondId;
  for(auto p : map->inputMap){
    PortInstance f = p.first;
    PortInstance s = p.second;

    Assert(idToCheckFirst == f.inst->accel->id);
    Assert(idToCheckSecond == s.inst->accel->id);
  }

  for(auto p : map->outputMap){
    PortInstance f = p.first;
    PortInstance s = p.second;

    Assert(idToCheckFirst == f.inst->accel->id);
    Assert(idToCheckSecond == s.inst->accel->id);
  }
}

void MappingCheck(AcceleratorMapping* map,Accelerator* first,Accelerator* second){
  int firstId = first->id;
  int secondId = second->id;

  Assert(map->firstId == firstId);
  Assert(map->secondId == secondId);
}

// TODO: Not a good approach
static Arena mappingArenaInst = {};
static Arena* mappingArena = nullptr;

AcceleratorMapping* MappingSimple(Accelerator* first,Accelerator* second,int size,Arena* out){
  if(!mappingArena){
    mappingArenaInst = InitArena(Megabyte(64));
    mappingArena = &mappingArenaInst; 
  }
  
  Assert(size > 0);
  
  AcceleratorMapping* mapping = PushStruct<AcceleratorMapping>(out);
  mapping->inputMap = PushTrieMap<PortInstance,PortInstance>(mappingArena);
  mapping->outputMap = PushTrieMap<PortInstance,PortInstance>(mappingArena);
  mapping->firstId = first->id;   
  mapping->secondId = second->id;

  return mapping;
}

AcceleratorMapping* MappingSimple(Accelerator* first,Accelerator* second,Arena* out){
  int size = Size(first->allocated);

  return MappingSimple(first,second,size,out);
}

AcceleratorMapping* MappingInvert(AcceleratorMapping* toReverse,Arena* out){
  MappingCheck(toReverse);

  AcceleratorMapping* result = PushStruct<AcceleratorMapping>(out);
  result->inputMap = PushTrieMap<PortInstance,PortInstance>(mappingArena);
  result->outputMap = PushTrieMap<PortInstance,PortInstance>(mappingArena);

  for(auto p : toReverse->inputMap){
    result->inputMap->Insert(p.second,p.first);
  }
  for(auto p : toReverse->outputMap){
    result->outputMap->Insert(p.second,p.first);
  }
  
  result->firstId = toReverse->secondId;
  result->secondId = toReverse->firstId;

  MappingCheck(result);
  
  return result;
}

AcceleratorMapping* MappingCombine(AcceleratorMapping* first,AcceleratorMapping* second,Arena* out){
  MappingCheck(first);
  MappingCheck(second);

  Assert(first->secondId == second->firstId);
  
  AcceleratorMapping* result = PushStruct<AcceleratorMapping>(out);
  result->inputMap = PushTrieMap<PortInstance,PortInstance>(mappingArena);
  result->outputMap = PushTrieMap<PortInstance,PortInstance>(mappingArena);

  for(auto p : first->inputMap){
    PortInstance start = p.first;
    PortInstance* end = second->inputMap->Get(p.second);

    if(end){
      result->inputMap->Insert(start,*end);
    }
  }

  for(auto p : first->outputMap){
    PortInstance start = p.first;
    PortInstance* end = second->outputMap->Get(p.second);

    if(end){
      result->outputMap->Insert(start,*end);
    }
  }

  result->firstId = first->firstId;
  result->secondId = second->secondId;
  
  return result;
}

PortInstance MappingMapInput(AcceleratorMapping* mapping,PortInstance toMap){
  int id = toMap.inst->accel->id;

  Assert(mapping->firstId == id);

  PortInstance* ptr = mapping->inputMap->Get(toMap);
  if(!ptr){
    return {nullptr,-1};
  }
  
  return *ptr;
}


PortInstance MappingMapOutput(AcceleratorMapping* mapping,PortInstance toMap){
  int id = toMap.inst->accel->id;

  Assert(mapping->firstId == id);

  PortInstance* ptr = mapping->outputMap->Get(toMap);
  if(!ptr){
    return {nullptr,-1};
  }
  
  return *ptr;
}

void MappingInsertEqualNode(AcceleratorMapping* mapping,FUInstance* first,FUInstance* second){
  Assert(mapping->firstId == first->accel->id);
  Assert(mapping->secondId == second->accel->id);

  Assert(first->declaration == second->declaration);
  FUDeclaration* decl = first->declaration;
  int nInputs = decl->NumberInputs();
  int nOutputs = decl->NumberOutputs();
  
  for(int i = 0; i < nInputs; i++){
    mapping->inputMap->Insert({first,i},{second,i});
  }

  for(int i = 0; i < nOutputs; i++){
    mapping->outputMap->Insert({first,i},{second,i});
  }
}

void MappingInsertInput(AcceleratorMapping* mapping,PortInstance first,PortInstance second){
  Assert(mapping->firstId == first.inst->accel->id);
  Assert(mapping->secondId == second.inst->accel->id);

  mapping->inputMap->Insert(first,second);
}

void MappingInsertOutput(AcceleratorMapping* mapping,PortInstance first,PortInstance second){
  Assert(mapping->firstId == first.inst->accel->id);
  Assert(mapping->secondId == second.inst->accel->id);

  mapping->outputMap->Insert(first,second);
}

void MappingPrintInfo(AcceleratorMapping* map){
  printf("%d -> %d",map->firstId,map->secondId);
}

void MappingPrintAll(AcceleratorMapping* map){
  MappingPrintInfo(map);
  printf("\n");
  
  printf("Input map:\n");
  for(Pair<PortInstance,PortInstance> p : map->inputMap){
    printf("%.*s:%d(%d) -> %.*s:%d(%d)\n",UNPACK_SS(p.first.inst->name),p.first.port,p.first.inst->id,
                                         UNPACK_SS(p.second.inst->name),p.second.port,p.second.inst->id);
  }

  printf("Output map:\n");
  for(Pair<PortInstance,PortInstance> p : map->outputMap){
    printf("%.*s:%d(%d) -> %.*s:%d(%d)\n",UNPACK_SS(p.first.inst->name),p.first.port,p.first.inst->id,
                                         UNPACK_SS(p.second.inst->name),p.second.port,p.second.inst->id);
  }
}

FUInstance* MappingMapNode(AcceleratorMapping* mapping,FUInstance* inst){
  FUDeclaration* decl = inst->declaration;
  int nInputs = decl->NumberInputs();
  int nOutputs = decl->NumberOutputs();

  FUInstance* toCheck = nullptr;
  for(int i = 0; i < nInputs; i++){
    PortInstance* optPort = mapping->inputMap->Get({inst,i});

    if(optPort == nullptr){
      continue;
    }
    
    if(toCheck == nullptr){
      toCheck = optPort->inst;
    } else {
      Assert(toCheck == optPort->inst);
    }
  }

  for(int i = 0; i < nOutputs; i++){
    PortInstance* optPort = mapping->outputMap->Get({inst,i});

    if(optPort == nullptr){
      continue;
    }
    
    if(toCheck == nullptr){
      toCheck = optPort->inst;
    } else {
      Assert(toCheck == optPort->inst);
    }
  }
    
  return toCheck;
}

Set<PortInstance>* MappingMapInput(AcceleratorMapping* map,Set<PortInstance>* set,Arena* out){
  Set<PortInstance>* result = PushSet<PortInstance>(out,set->map->nodesUsed);

  for(PortInstance portInst : set){
    result->Insert(MappingMapInput(map,portInst));
  }

  return result;
}

Set<PortInstance>* MappingMapOutput(AcceleratorMapping* map,Set<PortInstance>* set,Arena* out){
  Set<PortInstance>* result = PushSet<PortInstance>(out,set->map->nodesUsed);

  for(PortInstance portInst : set){
    result->Insert(MappingMapOutput(map,portInst));
  }

  return result;
}

Pair<Accelerator*,SubMap*> Flatten2(Accelerator* accel,int times,Arena* temp){
  Arena* perm = globalPermanent;
  BLOCK_REGION(temp);

  Pair<Accelerator*,AcceleratorMapping*> pair = CopyAcceleratorWithMapping(accel,AcceleratorPurpose_FLATTEN,true,temp);
  Accelerator* newAccel = pair.first;
  
  Pool<FUInstance*> compositeInstances = {};
  Pool<FUInstance*> toRemove = {};
  std::unordered_map<StaticId,int> staticToIndex;
  SubMap* subMappingDone = PushTrieMap<SubMappingInfo,PortInstance>(globalPermanent);
  
  for(int i = 0; i < times; i++){
    FOREACH_LIST(FUInstance*,instPtr,newAccel->allocated){
      FUInstance* inst = instPtr;

      bool containsStatic = false;
      containsStatic = inst->isStatic || inst->declaration->staticUnits != nullptr;
      // TODO: For now we do not flatten units that are static or contain statics. It messes merge configurations and still do not know how to procceed. Need to beef merge up a bit before handling more complex cases. The problem was that after flattening statics would become shared (unions) and as such would appear in places where they where not supposed to appear.

      // TODO: Maybe static and shared units configurations could be stored in a separated structure.
      if(inst->declaration->baseCircuit && !containsStatic){
        FUInstance** ptr = compositeInstances.Alloc();

        *ptr = instPtr;
      }
    }

    if(compositeInstances.Size() == 0){
      break;
    }

    std::unordered_map<int,int> sharedToFirstChildIndex;

    int freeSharedIndex = GetFreeShareIndex(newAccel); //(maxSharedIndex != -1 ? maxSharedIndex + 1 : 0);
    int count = 0;
    for(FUInstance** instPtr : compositeInstances){
      FUInstance* inst = (*instPtr);

      Assert(inst->declaration->baseCircuit);

      count += 1;
      Accelerator* circuit = inst->declaration->baseCircuit;
      FUInstance* outputInstance = GetOutputInstance(circuit->allocated);

      int savedSharedIndex = freeSharedIndex;
      int instSharedIndex = freeSharedIndex;
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
      AcceleratorMapping* map = MappingSimple(circuit,newAccel,temp); // TODO: Leaking

      FOREACH_LIST(FUInstance*,ptr,circuit->allocated){
        FUInstance* circuitInst = ptr;
        if(circuitInst->declaration->type == FUDeclarationType_SPECIAL){
          continue;
        }

        String newName = PushString(perm,"%.*s_%.*s",UNPACK_SS(inst->name),UNPACK_SS(circuitInst->name));
        FUInstance* newInst = CopyInstance(newAccel,circuitInst,true,newName);
        MappingInsertEqualNode(map,circuitInst,newInst);

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
            shareIndex = GetFreeShareIndex(newAccel);

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
            int newIndex = GetFreeShareIndex(newAccel);

            sharedToShared.insert({circuitInst->sharedIndex,newIndex});

            ShareInstanceConfig(newInst,newIndex);
          }
        } else if(inst->sharedEnable){ // Currently flattening instance is shared
          ShareInstanceConfig(newInst,instSharedIndex); // Always use the same inst index
          //freeSharedIndex = GetFreeShareIndex(newAccel);
        } else if(circuitInst->sharedEnable){
          auto ptr = sharedToShared.find(circuitInst->sharedIndex);

          if(ptr != sharedToShared.end()){
            ShareInstanceConfig(newInst,ptr->second);
          } else {
            int newIndex = freeSharedIndex;
            
            sharedToShared.insert({circuitInst->sharedIndex,newIndex});

            ShareInstanceConfig(newInst,newIndex);
            freeSharedIndex = GetFreeShareIndex(newAccel);
          }
        }
      }

      if(inst->sharedEnable && savedSharedIndex > freeSharedIndex){
        freeSharedIndex = savedSharedIndex;
      }

      // TODO: All these iterations could be converted to a single loop where inside we check if its input,output or inernal edge

      EdgeIterator iter = IterateEdges(newAccel);
      // Add accel edges to output instances
      while(iter.HasNext()){
        Edge edgeInst = iter.Next();
        Edge* edge = &edgeInst;
        if(edge->units[0].inst == inst){
          EdgeIterator iter2 = IterateEdges(circuit);
          while(iter2.HasNext()){
            Edge edge2Inst = iter2.Next();
            Edge* circuitEdge = &edge2Inst;

            if(circuitEdge->units[1].inst == outputInstance && circuitEdge->units[1].port == edge->units[0].port){
              FUInstance* other = MappingMapOutput(map,circuitEdge->units[0]).inst;

              if(other == nullptr){
                continue;
              }

              FUInstance* out = other;
              FUInstance* in = edge->units[1].inst;
              int outPort = circuitEdge->units[0].port;
              int inPort = edge->units[1].port;
              int delay = edge->delay + circuitEdge->delay;

              SubMappingInfo t = {};

              t.subDeclaration = inst->declaration;
              t.higherName = inst->name;
              t.isInput = false;
              t.subPort = circuitEdge->units[1].port;

              subMappingDone->Insert(t,{in,inPort});
              
              ConnectUnits(out,outPort,in,inPort,delay);
            }
          }
        }
      }

      // Add accel edges to input instances
      iter = IterateEdges(newAccel);
      while(iter.HasNext()){
        Edge edgeInst = iter.Next();
        Edge* edge = &edgeInst;
        if(edge->units[1].inst == inst){
          FUInstance* circuitInst = GetInputInstance(circuit->allocated,edge->units[1].port);

          EdgeIterator iter2 = IterateEdges(circuit);
          while(iter2.HasNext()){
            Edge edge2Inst = iter2.Next();
            Edge* circuitEdge = &edge2Inst;
          
            if(circuitEdge->units[0].inst == circuitInst){
              FUInstance* other = MappingMapInput(map,circuitEdge->units[1]).inst;

              if(other == nullptr){
                continue;
              }

              FUInstance* out = edge->units[0].inst;
              FUInstance* in = other;
              int outPort = edge->units[0].port;
              int inPort = circuitEdge->units[1].port;
              int delay = edge->delay + circuitEdge->delay;

              SubMappingInfo t = {};

              t.subDeclaration = inst->declaration;
              t.higherName = inst->name;
              t.isInput = true;
              t.subPort = edge->units[1].port;
              
              subMappingDone->Insert(t,{out,outPort});
              
              ConnectUnits(out,outPort,in,inPort,delay);
            }
          }
        }
      }

      // Add circuit specific edges
      iter = IterateEdges(circuit);
      while(iter.HasNext()){
        Edge edgeInst = iter.Next();
        Edge* circuitEdge = &edgeInst;

        FUInstance* otherInput = MappingMapOutput(map,circuitEdge->units[0]).inst;
        FUInstance* otherOutput = MappingMapInput(map,circuitEdge->units[1]).inst;

        if(otherInput == nullptr || otherOutput == nullptr){
          continue;
        }

        FUInstance* out = otherInput;
        FUInstance* in = otherOutput;
        int outPort = circuitEdge->units[0].port;
        int inPort = circuitEdge->units[1].port;
        int delay = circuitEdge->delay;

        ConnectUnits(out,outPort,in,inPort,delay);
      }

      // Add input to output specific edges
      iter = IterateEdges(newAccel);
      while(iter.HasNext()){
        Edge edge1Inst = iter.Next();
        Edge* edge1 = &edge1Inst;
        if(edge1->units[1].inst == inst){
          PortInstance input = edge1->units[0];
          FUInstance* circuitInput = GetInputInstance(circuit->allocated,edge1->units[1].port);

          EdgeIterator iter2 = IterateEdges(newAccel);
          while(iter2.HasNext()){
            Edge edge2Inst = iter2.Next();
            Edge* edge2 = &edge2Inst;
            if(edge2->units[0].inst == inst){
              PortInstance output = edge2->units[1];
              int outputPort = edge2->units[0].port;

              EdgeIterator iter3 = IterateEdges(circuit);
              while(iter3.HasNext()){
                Edge edge3Inst = iter3.Next();
                Edge* circuitEdge = &edge3Inst;

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
    }

    toRemove.Clear();
    compositeInstances.Clear();
  }

  OutputDebugDotGraph(newAccel,STRING("flatten.dot"),temp);
  
  toRemove.Clear(true);
  compositeInstances.Clear(true);

  FUDeclaration base = {};
  newAccel->name = accel->name;
  
  return {newAccel,subMappingDone};
}

CalculateDelayResult CalculateDelay(Accelerator* accel,DAGOrderNodes order,Array<Partition> partitions,Arena* out){
  // TODO: We are currently using the delay pointer inside the ConnectionNode structure. When correct, eventually change to just use the hashmap or an array.
  static int functionCalls = 0;
  
  int nodes = Size(accel->allocated);
  int edges = 9999; // Size(accel->edges);
  EdgeDelay* edgeToDelay = PushHashmap<Edge,DelayInfo>(out,edges);
  NodeDelay* nodeDelay = PushHashmap<FUInstance*,DelayInfo>(out,nodes);
  PortDelay* portDelay = PushHashmap<PortInstance,DelayInfo>(out,edges);
  
  CalculateDelayResult res = {};
  res.edgesDelay = edgeToDelay;
  res.nodeDelay = nodeDelay;
  res.portDelay = portDelay;
  
  FOREACH_LIST(FUInstance*,ptr,accel->allocated){
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
      if(HasMultipleConfigs(node->declaration)){
        part = partitions[partitionIndex++].value;
      }

      nodeToPart->Insert(node,part);
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
            portDelay->Insert(node,{0,false});
          }
        }
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
    if(HasMultipleConfigs(node->declaration)){
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
      String result = GenerateDotGraph(accel,content,out,debugArena);
      OutputContentToFile(filePath,result);
    }
  }

  index = 0;
  partitionIndex = 0;
  // Continue up the tree
  for(; index < order.instances.size; index++){
    FUInstance* node = order.instances[index];

    int part = 0;
    if(HasMultipleConfigs(node->declaration)){
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
    String fileName = PushString(out,"1_%d_out3_%d.dot",functionCalls,graphs++);
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
    String fileName = PushString(out,"1_%d_out4_%d.dot",functionCalls,graphs++);
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
          a = decl->configInfo[nodePart].outputLatencies[info->port];
        } else {
          a = inst->declaration->baseConfig.outputLatencies[info->port];
        }
    
        int e = info->edgeDelay;

        FOREACH_LIST(ConnectionNode*,otherInfo,other->allInputs){
          int c = 0;
          if(HasMultipleConfigs(other->declaration)){
            int otherPart = nodeToPart->GetOrFail(other);
        
            c = other->declaration->configInfo[otherPart].inputDelays[info->instConnectedTo.port];
          } else {
            c = other->declaration->baseConfig.inputDelays[info->instConnectedTo.port];
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
    String result = GenerateDotGraph(accel,content,out,debugArena);
    OutputContentToFile(filepath,result);
  }
  
  functionCalls += 1;
  
  return res;
}

void PrintSubMappingInfo(SubMap* info){
  for(Pair<SubMappingInfo,PortInstance> f : info){
    SubMappingInfo map = f.first;
    PortInstance inst = f.second;
    printf("%.*s",UNPACK_SS(map.subDeclaration->name));
    printf("::%.*s:%d %s\n",UNPACK_SS(map.higherName),map.subPort,map.isInput ? "in" : "out");
    printf("%.*s:%d\n",UNPACK_SS(inst.inst->name),inst.port);
    printf("\n");
  }
}

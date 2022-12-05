#include "versatPrivate.hpp"

#include <unordered_map>
#include <set>
#include <queue>

#define TAG_TEMPORARY_MARK 1
#define TAG_PERMANENT_MARK 2

typedef std::unordered_map<ComplexFUInstance*,ComplexFUInstance*> InstanceMap;

Accelerator* CreateAccelerator(Versat* versat){
   Accelerator* accel = versat->accelerators.Alloc();
   accel->versat = versat;

   return accel;
}

// Forward declare
void DoPopulate(Accelerator* accel,FUDeclaration* accelType,FUInstanceInterfaces& in,Pool<StaticInfo>& staticsAllocated);

void PopulateAcceleratorRecursive(FUDeclaration* accelType,ComplexFUInstance* inst,FUInstanceInterfaces& in,Pool<StaticInfo>& staticsAllocated){
   FUDeclaration* type = inst->declaration;
   PushPtr<int> saved = in.config;

   UnitValues val = CalculateIndividualUnitValues(inst);

   bool foundStatic = false;
   if(inst->isStatic){
      //Assert(accelType);

      for(StaticInfo* info : staticsAllocated){
         if(info->module == accelType && CompareString(info->name,inst->name)){
            in.config.Init(info->ptr,info->nConfigs);
            foundStatic = true;
            break;
         }
      }

      if(!foundStatic){
         StaticInfo* allocation = staticsAllocated.Alloc();
         allocation->module = accelType;
         allocation->name = inst->name;
         allocation->nConfigs = inst->declaration->nConfigs;
         allocation->ptr = in.statics.Push(0);
         allocation->wires = inst->declaration->configWires;

         in.config = in.statics;
      }
   }

   // Accelerator instance doesn't allocate memory, it shares memory with sub units
   if(inst->compositeAccel || val.configs){
      inst->config = in.config.Push(val.configs);
   }
   if(inst->compositeAccel || val.states){
      inst->state = in.state.Push(val.states);
   }
   if(inst->compositeAccel || val.delays){
      inst->delay = in.delay.Push(val.delays);
   }

   // Except for outputs, each unit has it's own output memory
   if(val.outputs){
      inst->outputs = in.outputs.Push(val.outputs);
      inst->storedOutputs = in.storedOutputs.Push(val.outputs);
   }
   if(val.extraData){
      inst->extraData = in.extraData.Push(val.extraData);
   }

   #if 1
   if(!inst->initialized && type->initializeFunction){
      type->initializeFunction(inst);
      inst->initialized = true;
   }
   #endif

   if(inst->compositeAccel){
      FUDeclaration* newAccelType = inst->declaration;

      DoPopulate(inst->compositeAccel,newAccelType,in,staticsAllocated);
   }

   if(inst->declarationInstance && ((ComplexFUInstance*) inst->declarationInstance)->savedConfiguration){
      memcpy(inst->config,inst->declarationInstance->config,inst->declaration->nConfigs * sizeof(int));
   }

   if(inst->isStatic){
      if(!foundStatic){
         in.statics = in.config;
      }
      in.config = saved;
   }
}

void DoPopulate(Accelerator* accel,FUDeclaration* accelType,FUInstanceInterfaces& in,Pool<StaticInfo>& staticsAllocated){
   std::vector<SharingInfo> sharingInfo;
   for(ComplexFUInstance* inst : accel->instances){
      SharingInfo* info = nullptr;
      PushPtr<int> savedConfig = in.config;

      if(inst->sharedEnable){
         int index = inst->sharedIndex;

         if(index >= (int) sharingInfo.size()){
            sharingInfo.resize(index + 1);
         }

         info = &sharingInfo[index];

         if(info->init){ // Already exists, replace config with ptr
            in.config.Init(info->ptr,inst->declaration->nConfigs);
         } else {
            info->ptr = in.config.Push(0);
         }
      }

      PopulateAcceleratorRecursive(accelType,inst,in,staticsAllocated);

      if(inst->sharedEnable){
         if(info->init){
            in.config = savedConfig;
         }
         info->init = true;
      }
   }
}

void PopulateAccelerator(Versat* versat,Accelerator* accel){
   #if 0
   if(accel->type == Accelerator::CIRCUIT){
      return;
   }
   #endif

   // Accel should be top level
   UnitValues val = CalculateAcceleratorValues(versat,accel);

   #if 1
   ZeroOutRealloc(&accel->configAlloc,val.configs);
   ZeroOutRealloc(&accel->stateAlloc,val.states);
   ZeroOutRealloc(&accel->delayAlloc,val.delays);
   ZeroOutRealloc(&accel->outputAlloc,val.totalOutputs);
   ZeroOutRealloc(&accel->storedOutputAlloc,val.totalOutputs);
   ZeroOutRealloc(&accel->extraDataAlloc,val.extraData);
   ZeroOutRealloc(&accel->staticAlloc,val.statics);
   #endif

   FUInstanceInterfaces inter = {};

   #if 1
   inter.config.Init(accel->configAlloc);
   inter.state.Init(accel->stateAlloc);
   inter.delay.Init(accel->delayAlloc);
   inter.outputs.Init(accel->outputAlloc);
   inter.storedOutputs.Init(accel->storedOutputAlloc);
   inter.extraData.Init(accel->extraDataAlloc);
   inter.statics.Init(accel->staticAlloc);
   #endif

   accel->staticInfo.Clear();

   // Assuming no static units on top, for now
   DoPopulate(accel,accel->subtype,inter,accel->staticInfo);

#if 1
   Assert(inter.config.Empty());
   Assert(inter.state.Empty());
   Assert(inter.delay.Empty());
   Assert(inter.outputs.Empty());
   Assert(inter.storedOutputs.Empty());
   Assert(inter.extraData.Empty());
   Assert(inter.statics.Empty());
#endif
}

FUInstance* CreateFUInstance(Accelerator* accel,FUDeclaration* type,SizedString name,bool flat,bool isStatic){
   Assert(CheckValidName(name));

   ComplexFUInstance* ptr = accel->instances.Alloc();

   ptr->name = name;
   ptr->id = accel->entityId++;
   ptr->accel = accel;
   ptr->declaration = type;
   ptr->namedAccess = true;
   ptr->isStatic = isStatic;

   if(type->type == FUDeclaration::COMPOSITE){
      ptr->compositeAccel = CopyAccelerator(accel->versat,type->fixedDelayCircuit,nullptr,true);
   }
   if(type->type == FUDeclaration::ITERATIVE){
      ptr->compositeAccel = CopyAccelerator(accel->versat,type->forLoop,nullptr,true);

      ptr->iterative = CopyAccelerator(accel->versat,type->initial,nullptr,true);
      PopulateAccelerator(accel->versat,ptr->iterative);
      //LockAccelerator(ptr->iterative,Accelerator::Locked::GRAPH);
   }

   #if 1
   if(!flat){
      PopulateAccelerator(accel->versat,accel);
   }
   #endif

   return ptr;
}


Accelerator* CopyAccelerator(Versat* versat,Accelerator* accel,InstanceMap* map,bool flat){
   Accelerator* newAccel = CreateAccelerator(versat);
   InstanceMap nullCaseMap;

   if(map == nullptr){
      map = &nullCaseMap;
   }

   // Copy of instances
   for(ComplexFUInstance* inst : accel->instances){
      ComplexFUInstance* newInst = CopyInstance(newAccel,inst,inst->name,flat);
      newInst->declarationInstance = inst;

      map->insert({inst,newInst});
   }

   // Flat copy of edges
   for(Edge* edge : accel->edges){
      Edge* newEdge = newAccel->edges.Alloc();

      *newEdge = *edge;
      newEdge->units[0].inst = (ComplexFUInstance*) map->at(edge->units[0].inst);
      newEdge->units[1].inst = map->at(edge->units[1].inst);
   }

   // Copy of input instance pointers
   for(ComplexFUInstance** instPtr : accel->inputInstancePointers){
      ComplexFUInstance** newInstPtr = newAccel->inputInstancePointers.Alloc();

      *newInstPtr = map->at(*instPtr);
   }

   if(accel->outputInstance){
      newAccel->outputInstance = map->at(accel->outputInstance);
   }

   return newAccel;
}

ComplexFUInstance* CopyInstance(Accelerator* newAccel,ComplexFUInstance* oldInstance,SizedString newName,bool flat){
   ComplexFUInstance* newInst = (ComplexFUInstance*) CreateFUInstance(newAccel,oldInstance->declaration,newName,flat);

   newInst->parameters = oldInstance->parameters;
   newInst->baseDelay = oldInstance->baseDelay;
   newInst->isStatic = oldInstance->isStatic;
   newInst->sharedEnable = oldInstance->sharedEnable;
   newInst->sharedIndex = oldInstance->sharedIndex;

   return newInst;
}

void InitializeFUInstances(Accelerator* accel,bool force){
   #if 1
   AcceleratorIterator iter = {};
   for(ComplexFUInstance* inst = iter.Start(accel); inst; inst = iter.Next()){
      FUDeclaration* type = inst->declaration;
      if(type->initializeFunction && (force || !inst->initialized)){
         type->initializeFunction(inst);
         inst->initialized = true;
      }
   }
   #endif
}

void RemoveFUInstance(Accelerator* accel,ComplexFUInstance* inst){
   //TODO: Remove instance doesn't update the config / state / memMapped / delay pointers
   for(Edge* edge : accel->edges){
      if(edge->units[0].inst == inst){
         accel->edges.Remove(edge);
      } else if(edge->units[1].inst == inst){
         accel->edges.Remove(edge);
      }
   }

   accel->instances.Remove(inst);
}

void CompressAcceleratorMemory(Accelerator* accel){
   InstanceMap oldPosToNew;

   PoolIterator<ComplexFUInstance> iter = accel->instances.beginNonValid();

   for(ComplexFUInstance* inst : accel->instances){
      ComplexFUInstance* pos = *iter;

      iter.Advance();

      if(pos == inst){
         oldPosToNew.insert({inst,pos});
         continue;
      } else {
         ComplexFUInstance* newPos = accel->instances.Alloc();

         *newPos = *inst;
         oldPosToNew.insert({inst,newPos});

         accel->instances.Remove(inst);
      }
   }

   for(Edge* edge : accel->edges){
      auto unit1 = oldPosToNew.find(edge->units[0].inst);
      auto unit2 = oldPosToNew.find(edge->units[1].inst);

      if(unit1 != oldPosToNew.end()){
         edge->units[0].inst = unit1->second;
      }
      if(unit2 != oldPosToNew.end()){
         edge->units[1].inst = unit2->second;
      }
   }
}

Accelerator* Flatten(Versat* versat,Accelerator* accel,int times){
   Arena* arena = &versat->temp;

   InstanceMap map;
   Accelerator* newAccel = CopyAccelerator(versat,accel,&map,true);
   map.clear();

   Pool<ComplexFUInstance*> compositeInstances = {};
   Pool<ComplexFUInstance*> toRemove = {};
   std::unordered_map<StaticId,int> staticToIndex;

   for(int i = 0; i < times; i++){
      int maxSharedIndex = -1;
      for(ComplexFUInstance* inst : newAccel->instances){
         if(inst->declaration->type == FUDeclaration::COMPOSITE){
            ComplexFUInstance** ptr = compositeInstances.Alloc();

            *ptr = inst;
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
      for(ComplexFUInstance** instPtr : compositeInstances){
         ComplexFUInstance* inst = *instPtr;

         Assert(inst->declaration->type == FUDeclaration::COMPOSITE);

         count += 1;
         Accelerator* circuit = inst->compositeAccel;

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
         for(ComplexFUInstance* circuitInst : circuit->instances){
            if(circuitInst->declaration->type == FUDeclaration::SPECIAL){
               continue;
            }

            SizedString newName = PushString(&versat->permanent,"%.*s.%.*s",UNPACK_SS(inst->name),UNPACK_SS(circuitInst->name));
            ComplexFUInstance* newInst = CopyInstance(newAccel,circuitInst,newName,true);

            if(newInst->isStatic){
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
               newInst->isStatic = false;
            } else if(newInst->sharedEnable && inst->sharedEnable){
               auto ptr = sharedToShared.find(newInst->sharedIndex);

               if(ptr != sharedToShared.end()){
                  newInst->sharedIndex = ptr->second;
               } else {
                  int newIndex = freeSharedIndex++;

                  sharedToShared.insert({newInst->sharedIndex,newIndex});

                  newInst->sharedIndex = newIndex;
               }
            } else if(inst->sharedEnable){ // Currently flattening instance is shared
               ShareInstanceConfig(newInst,freeSharedIndex++);
            } else if(newInst->sharedEnable){
               auto ptr = sharedToShared.find(newInst->sharedIndex);

               if(ptr != sharedToShared.end()){
                  newInst->sharedIndex = ptr->second;
               } else {
                  int newIndex = freeSharedIndex++;

                  sharedToShared.insert({newInst->sharedIndex,newIndex});

                  newInst->sharedIndex = newIndex;
               }
            }

            map.insert({circuitInst,newInst});
         }

         if(inst->sharedEnable && savedSharedIndex > freeSharedIndex){
            freeSharedIndex = savedSharedIndex;
         }

         // Add accel edges to output instances
         for(Edge* edge : newAccel->edges){
            if(edge->units[0].inst == inst){
               for(Edge* circuitEdge: circuit->edges){
                  if(circuitEdge->units[1].inst == circuit->outputInstance && circuitEdge->units[1].port == edge->units[0].port){
                     auto iter = map.find(circuitEdge->units[0].inst);

                     if(iter == map.end()){
                        continue;
                     }

                     Edge* newEdge = newAccel->edges.Alloc();

                     ComplexFUInstance* mappedInst = iter->second;

                     newEdge->delay = edge->delay + circuitEdge->delay;
                     newEdge->units[0].inst = mappedInst;
                     newEdge->units[0].port = circuitEdge->units[0].port;
                     newEdge->units[1] = edge->units[1];
                  }
               }
            }
         }

         // Add accel edges to input instances
         for(Edge* edge : newAccel->edges){
            if(edge->units[1].inst == inst){
               ComplexFUInstance* circuitInst = *circuit->inputInstancePointers.Get(edge->units[1].port);

               for(Edge* circuitEdge : circuit->edges){
                  if(circuitEdge->units[0].inst == circuitInst){
                     auto iter = map.find(circuitEdge->units[1].inst);

                     if(iter == map.end()){
                        continue;
                     }

                     Edge* newEdge = newAccel->edges.Alloc();

                     ComplexFUInstance* mappedInst = iter->second;

                     newEdge->delay = edge->delay + circuitEdge->delay;
                     newEdge->units[0] = edge->units[0];
                     newEdge->units[1].inst = mappedInst;
                     newEdge->units[1].port = circuitEdge->units[1].port;
                  }
               }
            }
         }

         // Add circuit specific edges
         for(Edge* circuitEdge : circuit->edges){
            auto input = map.find(circuitEdge->units[0].inst);
            auto output = map.find(circuitEdge->units[1].inst);

            if(input == map.end() || output == map.end()){
               continue;
            }

            Edge* newEdge = newAccel->edges.Alloc();

            newEdge->delay = circuitEdge->delay;
            newEdge->units[0].inst = input->second;
            newEdge->units[0].port = circuitEdge->units[0].port;
            newEdge->units[1].inst = output->second;
            newEdge->units[1].port = circuitEdge->units[1].port;
         }

         // Add input to output specific edges
         for(Edge* edge1 : newAccel->edges){
            if(edge1->units[1].inst == inst){
               PortInstance input = edge1->units[0];
               ComplexFUInstance* circuitInput = *circuit->inputInstancePointers.Get(edge1->units[1].port);

               for(Edge* edge2 : newAccel->edges){
                  if(edge2->units[0].inst == inst){
                     PortInstance output = edge2->units[1];
                     int outputPort = edge2->units[0].port;

                     for(Edge* circuitEdge : circuit->edges){
                        if(circuitEdge->units[0].inst == circuitInput
                        && circuitEdge->units[1].inst == circuit->outputInstance
                        && circuitEdge->units[1].port == outputPort){

                           Edge* newEdge = newAccel->edges.Alloc();

                           newEdge->delay = edge1->delay + circuitEdge->delay + edge2->delay;
                           newEdge->units[0] = input;
                           newEdge->units[1] = output;
                        }
                     }
                  }
               }
            }
         }

         *toRemove.Alloc() = inst;

         map.clear();
      }

      for(ComplexFUInstance** instPtr : toRemove){
         ComplexFUInstance* inst = *instPtr;

         RemoveFUInstance(newAccel,inst);
      }

      {
         ArenaMarker marker(arena);
         AcceleratorView view = CreateAcceleratorView(newAccel,arena);
         view.CalculateGraphData(arena);
         IsGraphValid(view);
         OutputGraphDotFile(versat,view,true,"./debug/flatten.dot");
      }

      CompressAcceleratorMemory(newAccel);

      {
         ArenaMarker marker(arena);
         AcceleratorView view = CreateAcceleratorView(newAccel,arena);
         view.CalculateGraphData(arena);
         IsGraphValid(view);
      }

      toRemove.Clear();
      compositeInstances.Clear();
   }

   toRemove.Clear(true);
   compositeInstances.Clear(true);

   {
      ArenaMarker marker(arena);
      AcceleratorView view = CreateAcceleratorView(newAccel,arena);
      OutputGraphDotFile(versat,view,true,"./debug/flatten.dot");
   }

   #if 1
   PopulateAccelerator(versat,newAccel);
   InitializeFUInstances(newAccel,true);

   AcceleratorView view = CreateAcceleratorView(newAccel,arena);
   view.CalculateDelay(arena);
   FixDelays(versat,newAccel,view);

   UnitValues val1 = CalculateAcceleratorValues(versat,accel);
   UnitValues val2 = CalculateAcceleratorValues(versat,newAccel);

   if(times == 99){
      Assert((val1.configs + val1.statics) == val2.configs); // New accel shouldn't have statics, unless they are from delay units added (which should also be shared configs)
      Assert(val1.states == val2.states);
      Assert(val1.delays == val2.delays);
   }
   #endif

   newAccel->staticInfo.Clear();

   return newAccel;
}


ComplexFUInstance* AcceleratorIterator::Start(Accelerator* accel){
   index = 0;
   calledStart = true;

   stack[0] = accel->instances.begin();
   ComplexFUInstance* inst = *stack[index];

   return inst;
}

ComplexFUInstance* AcceleratorIterator::Next(){
   Assert(calledStart);

   ComplexFUInstance* inst = *stack[index];

   if(inst->compositeAccel){
      stack[++index] = inst->compositeAccel->instances.begin();
   } else {
      while(1){
         ++stack[index];

         if(!stack[index].HasNext()){
            if(index > 0){
               index -= 1;
               continue;
            }
         }

         break;
      }
   }

   if(!stack[index].HasNext()){
      return nullptr;
   }

   inst = *stack[index];

   return inst;
}

ComplexFUInstance* AcceleratorIterator::CurrentAcceleratorInstance(){
   int i = std::max(0,index - 1);

   ComplexFUInstance* inst = *stack[i];
   return inst;
}

void AcceleratorView::CalculateGraphData(Arena* arena){
   if(graphData){
      return;
   }

   graphData = true;
   int memoryNeeded = sizeof(GraphComputedData) * numberNodes + 2 * numberEdges * sizeof(ConnectionInfo);
   Byte* memory = PushBytes(arena,memoryNeeded);

   PushPtr<Byte> data;
   data.Init(memory,memoryNeeded);

   GraphComputedData* computedData = (GraphComputedData*) data.Push(numberNodes * sizeof(GraphComputedData));
   ConnectionInfo* inputBuffer = (ConnectionInfo*) data.Push(numberEdges * sizeof(ConnectionInfo));
   ConnectionInfo* outputBuffer = (ConnectionInfo*) data.Push(numberEdges * sizeof(ConnectionInfo));

   Assert(data.Empty());

   // Associate computed data to each instance
   for(int i = 0; i < numberNodes; i++){
      ComplexFUInstance* inst = nodes[i];

      inst->graphData = &computedData[i];
   }

   // Set inputs and outputs
   for(int i = 0; i < numberNodes; i++){
      ComplexFUInstance* inst = nodes[i];

      inst->graphData->inputs = inputBuffer;
      inst->graphData->outputs = outputBuffer;

      for(int i = 0; i < numberEdges; i++){
         Edge* edge = &edges[i];

         if(edge->units[0].inst == inst){
            outputBuffer->instConnectedTo = edge->units[1];
            outputBuffer->port = edge->units[0].port;
            outputBuffer += 1;

            inst->graphData->outputPortsUsed = std::max(inst->graphData->outputPortsUsed,edge->units[0].port + 1);
            inst->graphData->numberOutputs += 1;
         }
         if(edge->units[1].inst == inst){
            inputBuffer->instConnectedTo = edge->units[0];
            inputBuffer->port = edge->units[1].port;
            inputBuffer->delay = edge->delay;
            inputBuffer += 1;

            Assert(abs(inputBuffer->delay) < 100);

            inst->graphData->inputPortsUsed = std::max(inst->graphData->inputPortsUsed,edge->units[1].port + 1);
            inst->graphData->numberInputs += 1;
         }
      }
   }

   for(int i = 0; i < numberNodes; i++){
      ComplexFUInstance* inst = nodes[i];

      inst->graphData->nodeType = GraphComputedData::TAG_UNCONNECTED;

      bool hasInput = (inst->graphData->numberInputs > 0);
      bool hasOutput = (inst->graphData->numberOutputs > 0);

      // If the unit is both capable of acting as a sink or as a source of data
      if(hasInput && hasOutput){
         if(CHECK_DELAY(inst,DELAY_TYPE_SINK_DELAY) || CHECK_DELAY(inst,DELAY_TYPE_SOURCE_DELAY)){
            inst->graphData->nodeType = GraphComputedData::TAG_SOURCE_AND_SINK;
         }  else {
            inst->graphData->nodeType = GraphComputedData::TAG_COMPUTE;
         }
      } else if(hasInput){
         inst->graphData->nodeType = GraphComputedData::TAG_SINK;
      } else if(hasOutput){
         inst->graphData->nodeType = GraphComputedData::TAG_SOURCE;
      } else {
         // Unconnected
      }
   }
}

static void SendLatencyUpwards(ComplexFUInstance* inst){
   int b = inst->graphData->inputDelay;

   int minimum = 0;
   for(int i = 0; i < inst->graphData->numberInputs; i++){
      ConnectionInfo* info = &inst->graphData->inputs[i];

      int a = inst->declaration->inputDelays[info->port];
      int e = info->delay;

      ComplexFUInstance* other = inst->graphData->inputs[i].instConnectedTo.inst;
      for(int ii = 0; ii < other->graphData->numberOutputs; ii++){
         ConnectionInfo* otherInfo = &other->graphData->outputs[ii];

         int c = other->declaration->latencies[info->instConnectedTo.port];

         if(info->instConnectedTo.inst == other && info->instConnectedTo.port == otherInfo->port &&
            otherInfo->instConnectedTo.inst == inst && otherInfo->instConnectedTo.port == info->port){
            otherInfo->delay = b + a + e - c;

            minimum = std::min(minimum,otherInfo->delay);

            //Assert(abs(otherInfo->delay) < 100);
         }
      }
   }
}

void AcceleratorView::CalculateDelay(Arena* arena){
   CalculateDAGOrdering(arena);

   // Clear everything, just in case
   for(int i = 0; i < numberNodes; i++){
      ComplexFUInstance* inst = nodes[i];
      inst->graphData->inputDelay = 0;

      for(int i = 0; i < inst->graphData->numberOutputs; i++){
         inst->graphData->outputs[i].delay = 0;
      }
   }

   #if 0
   if(versat->debug.outputGraphs){
      char buffer[1024];
      sprintf(buffer,"debug/%.*s/",UNPACK_SS(accel->subtype->name));
      MakeDirectory(buffer);
   }
   #endif

   for(int i = 0; i < order.numberSinks; i++){
      ComplexFUInstance* inst = order.sinks[i];

      inst->graphData->inputDelay = -10000; // As long as it is a constant greater than max latency, should work fine
      inst->baseDelay = abs(inst->graphData->inputDelay);

      SendLatencyUpwards(inst);

      #if 0
      if(versat->debug.outputGraphs){
         OutputGraphDotFile(versat,view,false,"debug/%.*s/out1_%d.dot",UNPACK_SS(view.accel->subtype->name),graphs++);
      }
      #endif
   }

   for(int i = numberNodes - order.numberSinks - 1; i >= 0; i--){
      ComplexFUInstance* inst = order.instances[i];

      if(inst->graphData->nodeType == GraphComputedData::TAG_UNCONNECTED){
         continue;
      }

      int minimum = (1 << 30);
      for(int ii = 0; ii < inst->graphData->numberOutputs; ii++){
         minimum = std::min(minimum,inst->graphData->outputs[ii].delay);
      }

      for(int ii = 0; ii < inst->graphData->numberOutputs; ii++){
         inst->graphData->outputs[ii].delay -= minimum;
      }

      inst->graphData->inputDelay = minimum;
      inst->baseDelay = abs(inst->graphData->inputDelay);

      SendLatencyUpwards(inst);

      #if 0
      if(versat->debug.outputGraphs){
         OutputGraphDotFile(versat,view,false,"debug/%.*s/out2_%d.dot",UNPACK_SS(view.accel->subtype->name),graphs++);
      }
      #endif
   }

   int minimum = 0;
   for(int i = 0; i < numberNodes; i++){
      ComplexFUInstance* inst = nodes[i];
      minimum = std::min(minimum,inst->graphData->inputDelay);
   }

   minimum = abs(minimum);
   for(int i = 0; i < numberNodes; i++){
      ComplexFUInstance* inst = nodes[i];
      inst->graphData->inputDelay = minimum - abs(inst->graphData->inputDelay);
   }

    for(int i = 0; i < numberNodes; i++){
      ComplexFUInstance* inst = nodes[i];
     if(inst->graphData->nodeType == GraphComputedData::TAG_UNCONNECTED){
         inst->graphData->inputDelay = 0;
      } else if(inst->graphData->nodeType == GraphComputedData::TAG_SOURCE_AND_SINK){
         for(int ii = 0; ii < inst->graphData->numberOutputs; ii++){
            inst->graphData->outputs[ii].delay = 0;
         }
      }
      inst->baseDelay = inst->graphData->inputDelay;
   }
}

static int Visit(ComplexFUInstance*** ordering,ComplexFUInstance* inst){
   if(inst->tag == TAG_PERMANENT_MARK){
      return 0;
   }
   if(inst->tag == TAG_TEMPORARY_MARK){
      Assert(0);
   }

   if(inst->graphData->nodeType == GraphComputedData::TAG_SINK ||
     (inst->graphData->nodeType == GraphComputedData::TAG_SOURCE_AND_SINK && CHECK_DELAY(inst,DelayType::DELAY_TYPE_SINK_DELAY))){
      return 0;
   }

   inst->tag = TAG_TEMPORARY_MARK;

   int count = 0;
   if(inst->graphData->nodeType == GraphComputedData::TAG_COMPUTE){
      for(int i = 0; i < inst->graphData->numberInputs; i++){
         count += Visit(ordering,inst->graphData->inputs[i].instConnectedTo.inst);
      }
   }

   inst->tag = TAG_PERMANENT_MARK;

   if(inst->graphData->nodeType == GraphComputedData::TAG_COMPUTE){
      *(*ordering) = inst;
      (*ordering) += 1;
      count += 1;
   }

   return count;
}

DAGOrder AcceleratorView::CalculateDAGOrdering(Arena* arena){
   if(dagOrder){
      return order;
   }

   dagOrder = true;

   CalculateGraphData(arena); // Need graph data for dag ordering

   order.numberComputeUnits = 0;
   order.numberSinks = 0;
   order.numberSources = 0;
   order.instances= PushArray(arena,numberNodes,ComplexFUInstance*);

   for(int i = 0; i < numberNodes; i++){
      ComplexFUInstance* inst = nodes[i];
      inst->tag = 0;
   }

   ComplexFUInstance** sourceUnits = order.instances;
   order.sources = sourceUnits;
   // Add source units, guaranteed to come first
   for(int i = 0; i < numberNodes; i++){
      ComplexFUInstance* inst = nodes[i];
      if(inst->graphData->nodeType == GraphComputedData::TAG_SOURCE || (inst->graphData->nodeType == GraphComputedData::TAG_SOURCE_AND_SINK && CHECK_DELAY(inst,DelayType::DELAY_TYPE_SOURCE_DELAY))){
         *(sourceUnits++) = inst;
         order.numberSources += 1;
         inst->tag = TAG_PERMANENT_MARK;
      }
   }

   // Add compute units
   ComplexFUInstance** computeUnits = sourceUnits;
   order.computeUnits = computeUnits;

   for(int i = 0; i < numberNodes; i++){
      ComplexFUInstance* inst = nodes[i];
      if(inst->graphData->nodeType == GraphComputedData::TAG_UNCONNECTED){
         *(computeUnits++) = inst;
         order.numberComputeUnits += 1;
         inst->tag = TAG_PERMANENT_MARK;
      } else if(inst->tag == 0 && inst->graphData->nodeType == GraphComputedData::TAG_COMPUTE){
         order.numberComputeUnits += Visit(&computeUnits,inst);
      }
   }

   // Add sink units
   ComplexFUInstance** sinkUnits = computeUnits;
   order.sinks = sinkUnits;

   for(int i = 0; i < numberNodes; i++){
      ComplexFUInstance* inst = nodes[i];
      if(inst->graphData->nodeType == GraphComputedData::TAG_SINK || (inst->graphData->nodeType == GraphComputedData::TAG_SOURCE_AND_SINK && CHECK_DELAY(inst,DelayType::DELAY_TYPE_SINK_DELAY))){
         *(sinkUnits++) = inst;
         order.numberSinks += 1;
         Assert(inst->tag == 0);
         inst->tag = TAG_PERMANENT_MARK;
      }
   }


   for(int i = 0; i < numberNodes; i++){
      ComplexFUInstance* inst = nodes[i];
      Assert(inst->tag == TAG_PERMANENT_MARK);
   }

   Assert(order.numberSources + order.numberComputeUnits + order.numberSinks == numberNodes);

   // Calculate order
   for(int i = 0; i < order.numberSources; i++){
      ComplexFUInstance* inst = order.sources[i];

      inst->graphData->order = 0;
   }

   for(int i = order.numberSources; i < numberNodes; i++){
      ComplexFUInstance* inst = order.instances[i];

      int max = 0;
      for(int ii = 0; ii < inst->graphData->numberInputs; ii++){
         max = std::max(max,inst->graphData->inputs[ii].instConnectedTo.inst->graphData->order);
      }

      inst->graphData->order = (max + 1);
   }

   dagOrder = true;
   return order;
}

SubgraphData SubGraphAroundInstance(Versat* versat,Accelerator* accel,ComplexFUInstance* instance,int layers){
   NOT_IMPLEMENTED; // Use AcceleratorView to implement
   return SubgraphData{};
   #if 0
   Accelerator* newAccel = CreateAccelerator(versat);
   //newAccel->type = Accelerator::CIRCUIT;

   std::set<ComplexFUInstance*> subgraphUnits;
   std::set<ComplexFUInstance*> tempSubgraphUnits;
   subgraphUnits.insert(instance);
   for(int i = 0; i < layers; i++){
      for(ComplexFUInstance* inst : subgraphUnits){
         for(Edge* edge : accel->edges){
            for(int i = 0; i < 2; i++){
               if(edge->units[i].inst == inst){
                  tempSubgraphUnits.insert(edge->units[1 - i].inst);
                  break;
               }
            }
         }
      }

      subgraphUnits.insert(tempSubgraphUnits.begin(),tempSubgraphUnits.end());
      tempSubgraphUnits.clear();
   }

   InstanceMap map;
   for(ComplexFUInstance* nonMapped : subgraphUnits){
      ComplexFUInstance* mapped = (ComplexFUInstance*) CreateFUInstance(newAccel,nonMapped->declaration,nonMapped->name);
      map.insert({nonMapped,mapped});
   }

   for(Edge* edge : accel->edges){
      auto iter0 = map.find(edge->units[0].inst);
      auto iter1 = map.find(edge->units[1].inst);

      if(iter0 == map.end() || iter1 == map.end()){
         continue;
      }

      Edge* newEdge = newAccel->edges.Alloc();

      *newEdge = *edge;
      newEdge->units[0].inst = iter0->second;
      newEdge->units[1].inst = iter1->second;
   }

   SubgraphData data = {};

   data.accel = newAccel;
   data.instanceMapped = map.at(instance);

   return data;
   #endif
}

struct HuffmanBlock{
   int bits;
   ComplexFUInstance* instance; // TODO: Maybe add the instance index (on the list) so we can push to the left instances that appear first and make it easier to see the mapping taking place
   HuffmanBlock* left;
   HuffmanBlock* right;
   enum {LEAF,NODE} type;
};

static void SaveMemoryMappingInfo(char* buffer,int size,HuffmanBlock* block){
   if(block->type == HuffmanBlock::LEAF){
      ComplexFUInstance* inst = block->instance;

      memcpy(inst->versatData->memoryMask,buffer,size);
      inst->versatData->memoryMask[size] = '\0';
      inst->versatData->memoryMaskSize = size;
   } else {
      buffer[size] = '1';
      SaveMemoryMappingInfo(buffer,size + 1,block->left);
      buffer[size] = '0';
      SaveMemoryMappingInfo(buffer,size + 1,block->right);
   }
}

void AcceleratorView::CalculateVersatData(Arena* arena){
   if(versatData){
      return;
   }

   versatData = true;
   VersatComputedData* mem = PushArray(arena,numberNodes,VersatComputedData);

   CalculateGraphData(arena);

   for(int i = 0; i < numberNodes; i++){
      ComplexFUInstance* inst = nodes[i];

      if(inst->declaration->type == FUDeclaration::COMPOSITE && inst->compositeAccel){
         AcceleratorView view = CreateAcceleratorView(inst->compositeAccel,arena);
         view.CalculateVersatData(arena);
      }
   }

   auto Compare = [](const HuffmanBlock* a, const HuffmanBlock* b) {
      return a->bits > b->bits;
   };

   Pool<HuffmanBlock> blocks = {};
   std::priority_queue<HuffmanBlock*,std::vector<HuffmanBlock*>,decltype(Compare)> huffQueue(Compare);

   for(int i = 0; i < numberNodes; i++){
      ComplexFUInstance* inst = nodes[i];
      inst->versatData = mem++;

      if(inst->declaration->isMemoryMapped){
         HuffmanBlock* block = blocks.Alloc();

         block->bits = inst->declaration->memoryMapBits;
         block->instance = inst;
         block->type = HuffmanBlock::LEAF;

         huffQueue.push(block);
      }
   }

   if(huffQueue.size() == 0){
      return;
   }

   while(huffQueue.size() > 1){
      HuffmanBlock* first = huffQueue.top();
      huffQueue.pop();
      HuffmanBlock* second = huffQueue.top();
      huffQueue.pop();

      HuffmanBlock* block = blocks.Alloc();
      block->type = HuffmanBlock::NODE;
      block->left = first;
      block->right = second;
      block->bits = std::max(first->bits,second->bits) + 1;

      huffQueue.push(block);
   }

   char bitMapping[33];
   memset(bitMapping,0,33 * sizeof(char));
   HuffmanBlock* root = huffQueue.top();

   SaveMemoryMappingInfo(bitMapping,0,root);

   int maxMask = 0;
   for(int i = 0; i < numberNodes; i++){
      ComplexFUInstance* inst = nodes[i];
      if(inst->declaration->isMemoryMapped){
         maxMask = std::max(maxMask,inst->versatData->memoryMaskSize + inst->declaration->memoryMapBits);
      }
   }

   for(int i = 0; i < numberNodes; i++){
      ComplexFUInstance* inst = nodes[i];
      if(inst->declaration->isMemoryMapped){
         int memoryAddressOffset = 0;
         for(int i = 0; i < inst->versatData->memoryMaskSize; i++){
            int bit = inst->versatData->memoryMask[i] - '0';

            memoryAddressOffset += bit * (1 << (maxMask - i - 1));
         }
         inst->versatData->memoryAddressOffset = memoryAddressOffset;

         // TODO: Not a good way of dealing with calculating memory map to lower units, but should suffice for now
         Accelerator* subAccel = inst->compositeAccel;
         if(inst->declaration->type == FUDeclaration::COMPOSITE && subAccel){
            AcceleratorIterator iter = {};
            for(ComplexFUInstance* inst = iter.Start(subAccel); inst; inst = iter.Next()){
               if(inst->declaration->isMemoryMapped){
                  inst->versatData->memoryAddressOffset += memoryAddressOffset;
               }
            };
         }
      }
   }

   blocks.Clear(true);
}

void ClearFUInstanceTempData(Accelerator* accel){
   AcceleratorIterator iter = {};
   for(ComplexFUInstance* inst = iter.Start(accel); inst; inst = iter.Next()){
      inst->graphData = nullptr;
      inst->versatData = nullptr;
   }
}

AcceleratorView CreateAcceleratorView(Accelerator* accel,Arena* arena){
   AcceleratorView view = {};

   view.nodes = PushArray(arena,accel->instances.Size(),ComplexFUInstance*);
   view.edges = PushArray(arena,accel->edges.Size(),Edge);
   view.validNodes.Init(arena,accel->instances.Size());

   std::unordered_map<ComplexFUInstance*,int> map;
   for(ComplexFUInstance* inst : accel->instances){
      map.insert({inst,view.numberNodes});
      view.nodes[view.numberNodes] = inst;

      view.numberNodes += 1;
   }
   for(Edge* edge : accel->edges){
      Edge newEdge = {};

      ComplexFUInstance* inst0 = edge->units[0].inst;
      ComplexFUInstance* inst1 = edge->units[1].inst;

      newEdge = *edge;
      newEdge.units[0].inst = view.nodes[map.find(inst0)->second];
      newEdge.units[1].inst = view.nodes[map.find(inst1)->second];

      view.edges[view.numberEdges++] = newEdge;
   }

   return view;
}

int CalculateTotalOutputs(Accelerator* accel){
   int total = 0;
   AcceleratorIterator iter = {};
   for(FUInstance* inst = iter.Start(accel); inst; inst = iter.Next()){
      total += inst->declaration->nOutputs;
   }
   return total;
}

int CalculateTotalOutputs(FUInstance* inst){
   int total = 0;
   if(inst->compositeAccel){
      total += CalculateTotalOutputs(inst->compositeAccel);
   }
   total += inst->declaration->nOutputs;

   return total;
}

bool IsUnitCombinatorial(FUInstance* instance){
   FUDeclaration* type = instance->declaration;

   if(type->nOutputs && type->latencies[0] == 0){
      return true;
   } else {
      return false;
   }
}

static int CalculateLatency_(PortInstance portInst, std::unordered_map<PortInstance,int>* memoization,bool seenSourceAndSink){
   ComplexFUInstance* inst = portInst.inst;
   int port = portInst.port;

   if(inst->graphData->nodeType == GraphComputedData::TAG_SOURCE_AND_SINK && seenSourceAndSink){
      return inst->declaration->latencies[port];
   } else if(inst->graphData->nodeType == GraphComputedData::TAG_SOURCE){
      return inst->declaration->latencies[port];
   } else if(inst->graphData->nodeType == GraphComputedData::TAG_UNCONNECTED){
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

int CalculateLatency(ComplexFUInstance* inst){
   std::unordered_map<PortInstance,int> map;

   Assert(inst->graphData->nodeType == GraphComputedData::TAG_SOURCE_AND_SINK || inst->graphData->nodeType == GraphComputedData::TAG_SINK);

   int maxLatency = 0;
   for(int i = 0; i < inst->graphData->numberInputs; i++){
      ConnectionInfo* info = &inst->graphData->inputs[i];
      int latency = CalculateLatency_(info->instConnectedTo,&map,false);
      maxLatency = std::max(maxLatency,latency);
   }

   return maxLatency;
}


void FixMultipleInputs(Versat* versat,Accelerator* accel){
   Arena* arena = &versat->temp;
   ArenaMarker marker(arena);
   AcceleratorView view = CreateAcceleratorView(accel,arena);
   view.CalculateGraphData(arena);

   Assert(view.graphData);

   static int multiplexersInstantiated = 0;

   bool isComb = true;
   int portUsedCount[99];
   for(int i = 0; i < view.numberNodes; i++){
      ComplexFUInstance* inst = view.nodes[i];
      if(inst->declaration == versat->multiplexer || inst->declaration == versat->combMultiplexer){ // Not good, but works for now (otherwise newly added muxes would break the algorithm)
         continue;
      }

      Memset(portUsedCount,0,99);

      if(!IsUnitCombinatorial(inst)){
         isComb = false;
      }

      for(int i = 0; i < inst->graphData->numberInputs; i++){
         ConnectionInfo* info = &inst->graphData->inputs[i];

         Assert(info->port < 99 && info->port >= 0);
         portUsedCount[info->port] += 1;

         if(!IsUnitCombinatorial(info->instConnectedTo.inst)){
            isComb = false;
         }
      }

      for(int port = 0; port < 99; port++){
         if(portUsedCount[port] > 1){ // Edge has more that one connection
            FUDeclaration* muxType = versat->multiplexer;
            const char* format = "mux%d";

            if(isComb){
               muxType = versat->combMultiplexer;
               format = "comb_mux%d";
            }

            SizedString name = PushString(&versat->permanent,format,multiplexersInstantiated++);
            ComplexFUInstance* multiplexer = (ComplexFUInstance*) CreateFUInstance(accel,muxType,name);

            // Connect edges to multiplexer
            int portsConnected = 0;
            for(int i = 0; i < view.numberEdges; i++){
               Edge* edge = &view.edges[i];
               if(edge->units[1] == PortInstance{inst,port}){
                  edge->units[1].inst = multiplexer;
                  edge->units[1].port = portsConnected;
                  portsConnected += 1;
               }
            }
            Assert(portsConnected <= 2);

            // Connect multiplexer to intended input
            ConnectUnits(multiplexer,0,inst,port);

            // Ports connected can be used to parameterize the multiplexer connection size
         }
      }
   }

   #if 0
   if(versat->debug.outputGraphs){
      char buffer[1024];
      sprintf(buffer,"debug/%.*s/",UNPACK_SS(accel->subtype->name));
      MakeDirectory(buffer);
      OutputGraphDotFile(versat,view,false,"debug/%.*s/mux.dot",UNPACK_SS(accel->subtype->name));
   }
   #endif
}

// Fixes edges such that unit before connected to after, is reconnected to new unit
void InsertUnit(Accelerator* accel, PortInstance before, PortInstance after, PortInstance newUnit){
   for(Edge* edge : accel->edges){
      if(edge->units[0] == before && edge->units[1] == after){
         ConnectUnits(newUnit,after);

         edge->units[1] = newUnit;

         return;
      }
   }
}

void FixDelays(Versat* versat,Accelerator* accel,AcceleratorView view){
   Assert(view.graphData && view.dagOrder);

   DAGOrder order = view.order;

   int buffersInserted = 0;
   static int maxDelay = 0;
   for(int i = accel->instances.Size() - 1; i >= 0; i--){
      ComplexFUInstance* inst = order.instances[i];

      if(inst->declaration == versat->buffer || inst->declaration == versat->fixedBuffer){
         continue;
      }

      for(int ii = 0; ii < inst->graphData->numberOutputs; ii++){
         ConnectionInfo* info = &inst->graphData->outputs[ii];
         ComplexFUInstance* other = info->instConnectedTo.inst;

         if(other->declaration == versat->buffer){
            other->baseDelay = info->delay;
         } else if(other->declaration == versat->fixedBuffer){
            //NOT_IMPLEMENTED;
         } else if(info->delay > 0){

            //Assert(!(inst->declaration == versat->buffer || inst->declaration == versat->fixedBuffer));

            if(versat->debug.useFixedBuffers){
               SizedString bufferName = PushString(&versat->permanent,"fixedBuffer%d",buffersInserted++);

               ComplexFUInstance* delay = (ComplexFUInstance*) CreateFUInstance(accel,versat->fixedBuffer,bufferName,false,false);
               delay->parameters = PushString(&versat->permanent,"#(.AMOUNT(%d))",info->delay - versat->fixedBuffer->latencies[0]);

               InsertUnit(accel,PortInstance{inst,info->port},PortInstance{info->instConnectedTo.inst,info->instConnectedTo.port},PortInstance{delay,0});

               delay->baseDelay = info->delay - versat->fixedBuffer->latencies[0];

               maxDelay = std::max(maxDelay,delay->baseDelay);
            } else {
               SizedString bufferName = PushString(&versat->permanent,"buffer%d",buffersInserted++);

               ComplexFUInstance* delay = (ComplexFUInstance*) CreateFUInstance(accel,versat->buffer,bufferName,false,true);
               InsertUnit(accel,PortInstance{inst,info->port},PortInstance{info->instConnectedTo.inst,info->instConnectedTo.port},PortInstance{delay,0});

               delay->baseDelay = info->delay - versat->buffer->latencies[0];
               Assert(delay->baseDelay >= 0);

               maxDelay = std::max(maxDelay,delay->baseDelay);

               if(delay->config){
                  delay->config[0] = delay->baseDelay;
               }
            }
         } else {
            Assert(info->delay >= 0);
         }
      }
   }

   view = CreateAcceleratorView(accel,&versat->temp);
   view.CalculateDAGOrdering(&versat->temp);

   for(ComplexFUInstance* inst : accel->instances){
      inst->graphData->inputDelay = inst->baseDelay;

      Assert(inst->baseDelay >= 0);
      Assert(inst->baseDelay < 9999); // Unless dealing with really long accelerators, should catch some calculating bugs

      if(inst->declaration == versat->buffer && inst->config){
         inst->config[0] = inst->baseDelay;
      }
   }

   if(versat->debug.outputGraphs){
      //OutputGraphDotFile(versat,accel,false,"debug/%.*s/out3_%d.dot",UNPACK_SS(accel->subtype->name),graphs++);
   }
}

void ActivateMergedAcceleratorRecursive(Versat* versat,ComplexFUInstance* inst, int index){
   if(inst->compositeAccel){
      AcceleratorIterator iter = {};

      for(ComplexFUInstance* subInst = iter.Start(inst->compositeAccel); subInst; subInst = iter.Next()){
         if(versat->multiplexer == subInst->declaration){
            subInst->config[0] = index;
         }
      }
   } else {
      if(versat->multiplexer == inst->declaration){
         inst->config[0] = index;
      }
   }
}

void ActivateMergedAccelerator(Versat* versat,Accelerator* accel,FUDeclaration* type){
   AcceleratorIterator iter = {};

   for(ComplexFUInstance* inst = iter.Start(accel); inst; inst = iter.Next()){
      for(int i = 0; i < 2; i++){
         if(inst->declaration->mergedType[i] == type){
            ActivateMergedAcceleratorRecursive(versat,inst,i);
         }
      }
   }
}


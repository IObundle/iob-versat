#include "versatPrivate.hpp"
#include "debug.hpp"

#include <unordered_map>
#include <set>
#include <queue>

#define TAG_TEMPORARY_MARK 1
#define TAG_PERMANENT_MARK 2

typedef std::unordered_map<ComplexFUInstance*,ComplexFUInstance*> InstanceMap;

Accelerator* CreateAccelerator(Versat* versat){
   Accelerator* accel = versat->accelerators.Alloc();
   accel->versat = versat;

   Reserve(&accel->configAlloc,Kilobyte(1));
   Reserve(&accel->stateAlloc,Kilobyte(1));
   Reserve(&accel->delayAlloc,Kilobyte(1));
   Reserve(&accel->staticAlloc,Kilobyte(1));
   Reserve(&accel->outputAlloc,Megabyte(1));
   Reserve(&accel->storedOutputAlloc,Megabyte(1));
   Reserve(&accel->extraDataAlloc,Megabyte(1));

   return accel;
}

void RepopulateAccelerator(Accelerator* topLevel){
   STACK_ARENA(temp,Kilobyte(64));

   AcceleratorIterator iter = {};
   iter.Start(topLevel,&temp,true); // This will populate top level units

   for(ComplexFUInstance* inst : topLevel->subInstances){
      ComplexFUInstance* other = (ComplexFUInstance*) GetInstanceByName(topLevel,inst->name);

      *inst = *other;
   }
}

void InitializeSubaccelerator(AcceleratorIterator iter){
   int staticIndex = 0;
   ComplexFUInstance* parent = iter.CurrentAcceleratorInstance();
   FUDeclaration* decl = parent->declaration;

   for(ComplexFUInstance* inst = iter.Current(); inst; inst = iter.Skip()){
      #if 01
      if(inst->savedConfiguration){
         if(inst->isStatic){
            Assert(inst->declaration->configs.size <= decl->defaultStatic.size - staticIndex);
            Memcpy(inst->config,&decl->defaultStatic.data[staticIndex],inst->declaration->configs.size);
            staticIndex += inst->declaration->configs.size;
         } else {
            int configIndex = inst->config - parent->config;
            Assert(inst->declaration->configs.size <= decl->defaultConfig.size - configIndex);
            Memcpy(inst->config,&decl->defaultConfig.data[configIndex],inst->declaration->configs.size);
         }
      }
      #endif

      if(inst->declaration->initializeFunction){
         inst->declaration->initializeFunction(inst);
      }

      if(inst->declaration->type == FUDeclaration::COMPOSITE){
         AcceleratorIterator it = iter.LevelBelowIterator();
         InitializeSubaccelerator(it);
      }
   }
}

FUInstance* CreateFUInstance(Accelerator* accel,FUDeclaration* type,SizedString name,bool flat,bool isStatic){
   ArenaMarker marker(&accel->versat->temp);

   Assert(CheckValidName(name));

   ComplexFUInstance* ptr = accel->instances.Alloc();

   ptr->name = name;
   ptr->id = accel->entityId++;
   ptr->accel = accel;
   ptr->declaration = type;
   ptr->namedAccess = true;
   ptr->isStatic = isStatic;

   for(auto pair : type->staticUnits){
      StaticInfo* info = accel->staticInfo.Alloc();

      info->id = pair.first;
      info->data = pair.second;
   }

   if(isStatic){
      int offset = accel->staticInfo.Size();
      StaticInfo* info = accel->staticInfo.Alloc();

      info->id.name = name;
      info->id.parent = nullptr;
      info->data.offset = offset;
   }

   #if 0
   if(CompareString(name,"Test"))
      DebugTerminal(MakeValue(accel,"Accelerator"));
   #endif

   if(true){
   //if(!flat){
      // The position of top level newly created units is always at the end of the current allocated configurations
      int configOffset = accel->configAlloc.size;
      if(isStatic){
         configOffset = accel->staticAlloc.size;
      }

      int stateOffset = accel->stateAlloc.size;
      int delayOffset = accel->delayAlloc.size;
      int outputOffset = accel->outputAlloc.size;
      int extraDataOffset = accel->extraDataAlloc.size;

      UnitValues val = CalculateAcceleratorValues(accel->versat,accel);
      Assert(!ZeroOutRealloc(&accel->configAlloc,val.configs));
      Assert(!ZeroOutRealloc(&accel->stateAlloc,val.states));
      Assert(!ZeroOutRealloc(&accel->delayAlloc,val.delays));
      Assert(!ZeroOutRealloc(&accel->outputAlloc,val.totalOutputs));
      Assert(!ZeroOutRealloc(&accel->storedOutputAlloc,val.totalOutputs));
      Assert(!ZeroOutRealloc(&accel->extraDataAlloc,val.extraData));
      Assert(!ZeroOutRealloc(&accel->staticAlloc,val.statics));

      if(isStatic){
         ptr->config = &accel->staticAlloc.ptr[configOffset];
      } else {
         ptr->config = &accel->configAlloc.ptr[configOffset];
      }

      ptr->state = &accel->stateAlloc.ptr[stateOffset];
      ptr->delay = &accel->delayAlloc.ptr[delayOffset];
      ptr->outputs = &accel->outputAlloc.ptr[outputOffset];
      ptr->storedOutputs = &accel->storedOutputAlloc.ptr[outputOffset];
      ptr->extraData = &accel->extraDataAlloc.ptr[extraDataOffset];

      // Initialize sub units
      if(type->type == FUDeclaration::COMPOSITE){ // TODO: Iterative units
         // Fix static info
         for(Pair<StaticId,StaticData> pair : type->staticUnits){
            bool found = false;
            for(StaticInfo* info : accel->staticInfo){
               if(pair.first == info->id){
                  found = true;
                  break;
               }
            }
            if(found){
               continue;
            }

            StaticInfo* info = accel->staticInfo.Alloc();

            info->id = pair.first;
            info->data = pair.second;
         }

         AcceleratorIterator iter = {};
         for(ComplexFUInstance* inst = iter.Start(accel,&accel->versat->temp,true); inst; inst = iter.Skip()){
            // Have to iterate the top units until reaching the one we just created
            if(inst == ptr){
               AcceleratorIterator it = iter.LevelBelowIterator();
               InitializeSubaccelerator(it);
            }
         }
      } else if(type->initializeFunction){
         type->initializeFunction(ptr);
      }
   }

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

      Memcpy(newInst->config,inst->config,inst->declaration->configs.size);
      newInst->savedConfiguration = inst->savedConfiguration;

      map->insert({inst,newInst});
   }

   // Flat copy of edges
   for(Edge* edge : accel->edges){
      Edge* newEdge = newAccel->edges.Alloc();

      *newEdge = *edge;
      newEdge->units[0].inst = (ComplexFUInstance*) map->at(edge->units[0].inst);
      newEdge->units[1].inst = map->at(edge->units[1].inst);
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

#if 0
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
#endif

void RemoveFUInstance(Accelerator* accel,ComplexFUInstance* inst){
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

int CountNonOperationChilds(Accelerator* accel){
   if(accel == nullptr){
      return 0;
   }

   int count = 0;
   for(ComplexFUInstance* inst : accel->instances){
      if(inst->declaration->type == FUDeclaration::COMPOSITE){
         count += CountNonOperationChilds(inst->declaration->fixedDelayCircuit);
      }

      if(!inst->declaration->isOperation && inst->declaration->type != FUDeclaration::SPECIAL){
         count += 1;
      }
   }

   return count;
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
         Accelerator* circuit = inst->declaration->fixedDelayCircuit;
         ComplexFUInstance* outputInstance = GetOutputInstance(circuit);

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
                  if(circuitEdge->units[1].inst == outputInstance && circuitEdge->units[1].port == edge->units[0].port){
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
               ComplexFUInstance* circuitInst = GetInputInstance(circuit,edge->units[1].port);

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
               ComplexFUInstance* circuitInput = GetInputInstance(circuit,edge1->units[1].port);

               for(Edge* edge2 : newAccel->edges){
                  if(edge2->units[0].inst == inst){
                     PortInstance output = edge2->units[1];
                     int outputPort = edge2->units[0].port;

                     for(Edge* circuitEdge : circuit->edges){
                        if(circuitEdge->units[0].inst == circuitInput
                        && circuitEdge->units[1].inst == outputInstance
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
      view.CalculateGraphData(arena);
      OutputGraphDotFile(versat,view,true,"./debug/flatten.dot");
   }

   #if 0
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

ComplexFUInstance* AcceleratorIterator::Start(Accelerator* topLevel,ComplexFUInstance* compositeInst,Arena* arena,bool populate){
   this->topLevel = topLevel;
   this->level = 0;
   this->calledStart = true;
   this->populate = populate;
   FUDeclaration* decl = compositeInst->declaration;
   Accelerator* accel = compositeInst->declaration->fixedDelayCircuit;

   if(populate){
      staticUnits = PushStruct<Hashmap<StaticId,StaticData>>(arena);
      staticUnits->Init(arena,topLevel->staticInfo.Size());
      for(StaticInfo* info : topLevel->staticInfo){
         staticUnits->Insert(info->id,info->data);
      }

      staticUnits = &decl->staticUnits;

      FUInstanceInterfaces inter = {};
      inter.config.Init(compositeInst->config,decl->configs.size);
      inter.state.Init(compositeInst->state,decl->states.size);
      inter.delay.Init(compositeInst->delay,decl->nDelays);
      inter.outputs.Init(compositeInst->outputs,decl->totalOutputs);
      inter.storedOutputs.Init(compositeInst->storedOutputs,decl->totalOutputs);
      inter.extraData.Init(compositeInst->extraData,decl->extraDataSize);
      inter.statics.Init(topLevel->staticAlloc);

      PopulateAccelerator(topLevel,accel,decl,inter,*staticUnits);

      #if 0
      Assert(inter.config.Empty());
      Assert(inter.state.Empty());
      Assert(inter.delay.Empty());
      Assert(inter.outputs.Empty());
      Assert(inter.storedOutputs.Empty());
      Assert(inter.extraData.Empty());
      #endif
   }

   stack = PushArray<PoolIterator<ComplexFUInstance>>(arena,99);
   stack[0] = compositeInst->declaration->fixedDelayCircuit->instances.begin();
   ComplexFUInstance* inst = *stack[level];

   return inst;
}

ComplexFUInstance* AcceleratorIterator::Start(Accelerator* topLevel,Arena* arena,bool populate){
   this->level = 0;
   this->calledStart = true;
   this->topLevel = topLevel;
   this->populate = populate;

   if(populate){
      staticUnits = PushStruct<Hashmap<StaticId,StaticData>>(arena);
      staticUnits->Init(arena,topLevel->staticInfo.Size());
      for(StaticInfo* info : topLevel->staticInfo){
         staticUnits->Insert(info->id,info->data);
      }

      FUInstanceInterfaces inter = {};
      inter.config.Init(topLevel->configAlloc);
      inter.state.Init(topLevel->stateAlloc);
      inter.delay.Init(topLevel->delayAlloc);
      inter.outputs.Init(topLevel->outputAlloc);
      inter.storedOutputs.Init(topLevel->storedOutputAlloc);
      inter.extraData.Init(topLevel->extraDataAlloc);
      inter.statics.Init(topLevel->staticAlloc);

      PopulateAccelerator2(topLevel,nullptr,inter,*staticUnits);

      #if 1
      Assert(inter.config.Empty());
      Assert(inter.state.Empty());
      Assert(inter.delay.Empty());
      Assert(inter.outputs.Empty());
      Assert(inter.storedOutputs.Empty());
      Assert(inter.extraData.Empty());
      #endif
   }

   stack = PushArray<PoolIterator<ComplexFUInstance>>(arena,99);
   stack[0] = topLevel->instances.begin();
   ComplexFUInstance* inst = *stack[level];

   return inst;
}

ComplexFUInstance* AcceleratorIterator::Descend(){
   ComplexFUInstance* inst = *stack[level];

   if(populate){
      FUDeclaration* decl = inst->declaration;
      Accelerator* accel = decl->fixedDelayCircuit;
      FUInstanceInterfaces inter = {};

      inter.config.Init(inst->config,decl->configs.size);
      inter.state.Init(inst->state,decl->states.size);
      inter.delay.Init(inst->delay,decl->nDelays);
      inter.outputs.Init(inst->outputs,decl->totalOutputs);
      inter.storedOutputs.Init(inst->storedOutputs,decl->totalOutputs);
      inter.extraData.Init(inst->extraData,decl->extraDataSize);
      inter.statics.Init(topLevel->staticAlloc);

      PopulateAccelerator(topLevel,accel,decl,inter,*staticUnits);

      // Don't make sense when using PopulateAccelerator(1). The function does not push
      #if 0
      Assert(inter.config.Empty());
      Assert(inter.state.Empty());
      Assert(inter.delay.Empty());
      Assert(inter.outputs.Empty());
      Assert(inter.storedOutputs.Empty());
      Assert(inter.extraData.Empty());
      #endif
   }

   stack[++level] = inst->declaration->fixedDelayCircuit->instances.begin();

   return Current();
}

ComplexFUInstance* AcceleratorIterator::Current(){
   if(!stack[level].HasNext()){
      return nullptr;
   }

   ComplexFUInstance* inst = *stack[level];

   return inst;
}

ComplexFUInstance* AcceleratorIterator::CurrentAcceleratorInstance(){
   #if 0 // Only because of LevelBelowIterator. Ideally should have a flag to signal this situation and let the function access stack[-1] for iterators that we know they have a bigger stack
   if(level == 0){
      return nullptr;
   }
   #endif

   ComplexFUInstance* inst = *stack[level - 1];
   return inst;
}

ComplexFUInstance* AcceleratorIterator::Next(){
   Assert(calledStart);

   ComplexFUInstance* inst = *stack[level];

   if(inst->declaration->fixedDelayCircuit){
      return Descend();
   } else {
      while(1){
         ++stack[level];

         if(!stack[level].HasNext()){
            if(level > 0){
               level -= 1;
               continue;
            }
         }

         break;
      }
   }

   return Current();
}

ComplexFUInstance* AcceleratorIterator::Skip(){
   Assert(calledStart);

   ++stack[level];

   return Current();
}

AcceleratorIterator AcceleratorIterator::LevelBelowIterator(Arena* arena){
   ComplexFUInstance* inst = Current();

   Assert(inst && inst->declaration->type == FUDeclaration::COMPOSITE);

   AcceleratorIterator iter = {};

   iter.Start(topLevel,inst,arena,populate);

   return iter;
}

AcceleratorIterator AcceleratorIterator::LevelBelowIterator(){
   Descend();

   AcceleratorIterator iter = {};
   iter.stack.data = &this->stack.data[level];
   iter.stack.size = this->stack.size - level;
   iter.staticUnits = this->staticUnits;
   iter.topLevel = this->topLevel;
   iter.level = 0;
   iter.calledStart = true;
   iter.populate = this->populate;

   this->level -= 1;

   return iter;
}

void AcceleratorView::SetGraphData(){
   Assert(graphData.size);

   GraphComputedData* computedData = (GraphComputedData*) graphData.data;

   // Associate computed data to each instance
   int index = 0;
   for(ComplexFUInstance** instPtr : nodes){
      ComplexFUInstance* inst = *instPtr;
      inst->graphData = &computedData[index++];
   }
}

void AcceleratorView::CalculateGraphData(Arena* arena){
   if(graphData.size){
      return;
   }

   int nNodes = nodes.Size();
   int nEdges = edges.Size();

   int memoryNeeded = sizeof(GraphComputedData) * nNodes + 2 * nEdges * sizeof(ConnectionInfo);
   graphData = PushArray<Byte>(arena,memoryNeeded);

   PushPtr<Byte> data;
   data.Init(graphData);

   /* GraphComputedData* computedData = (GraphComputedData*) */ data.Push(nNodes * sizeof(GraphComputedData));
   ConnectionInfo* inputBuffer = (ConnectionInfo*) data.Push(nEdges * sizeof(ConnectionInfo));
   ConnectionInfo* outputBuffer = (ConnectionInfo*) data.Push(nEdges * sizeof(ConnectionInfo));
   Assert(data.Empty());

   SetGraphData();

   // Set inputs and outputs
   for(ComplexFUInstance** instPtr : nodes){
      ComplexFUInstance* inst = *instPtr;
      GraphComputedData* graphData = inst->graphData;

      graphData->allInputs.data = inputBuffer;
      graphData->allOutputs.data = outputBuffer;

      // TODO: for circuit inputs, max inputs don't need to be equal to the declared value. They can depend on the graph
      int maxInputs = inst->declaration->inputDelays.size;
      graphData->outputs = inst->declaration->outputLatencies.size;

      for(EdgeView* edgeView : edges){
         Edge* edge = edgeView->edge;

         if(edge->units[0].inst == inst){
            outputBuffer->instConnectedTo = edge->units[1];
            outputBuffer->port = edge->units[0].port;
            outputBuffer->edgeDelay = edge->delay;
            outputBuffer->delay = &edgeView->delay;

            outputBuffer += 1;

            graphData->allOutputs.size += 1;
         }
         if(edge->units[1].inst == inst){
            inputBuffer->instConnectedTo = edge->units[0];
            inputBuffer->port = edge->units[1].port;
            inputBuffer->edgeDelay = edge->delay;
            inputBuffer->delay = &edgeView->delay;
            inputBuffer += 1;

            graphData->allInputs.size += 1;
         }
      }

      graphData->singleInputs = PushArray<PortInstance>(arena,maxInputs);
      for(ConnectionInfo& info : graphData->allInputs){
         int index = info.port;

         if(graphData->singleInputs[index].inst){
            graphData->multipleSamePortInputs = true;
         } else {
            graphData->singleInputs[index] = info.instConnectedTo;
         }
      }

      graphData->nodeType = GraphComputedData::TAG_UNCONNECTED;

      bool hasInput = (graphData->allInputs.size > 0);
      bool hasOutput = (graphData->allOutputs.size > 0);

      // If the unit is both capable of acting as a sink or as a source of data
      if(hasInput && hasOutput){
         if(CHECK_DELAY(inst,DELAY_TYPE_SINK_DELAY) || CHECK_DELAY(inst,DELAY_TYPE_SOURCE_DELAY)){
            graphData->nodeType = GraphComputedData::TAG_SOURCE_AND_SINK;
         }  else {
            graphData->nodeType = GraphComputedData::TAG_COMPUTE;
         }
      } else if(hasInput){
         graphData->nodeType = GraphComputedData::TAG_SINK;
      } else if(hasOutput){
         graphData->nodeType = GraphComputedData::TAG_SOURCE;
      } else {
         // Unconnected
      }
   }
}

#if 1
static void SendLatencyUpwards(ComplexFUInstance* inst){
   int b = inst->graphData->inputDelay;

   for(ConnectionInfo& info : inst->graphData->allOutputs){
      ComplexFUInstance* other = info.instConnectedTo.inst;

      // Do not set delay for source and sink units. Source units cannot be found in this, otherwise they wouldn't be source
      Assert(other->graphData->nodeType != GraphComputedData::TAG_SOURCE);

      int a = inst->declaration->outputLatencies[info.port];
      int e = info.edgeDelay;

      for(ConnectionInfo& otherInfo : other->graphData->allInputs){
         int c = other->declaration->inputDelays[info.instConnectedTo.port];

         if(info.instConnectedTo.port == otherInfo.port &&
            otherInfo.instConnectedTo.inst == inst && otherInfo.instConnectedTo.port == info.port){

            *otherInfo.delay = b + a + e - c;
         }
      }
   }
}
#endif

#if 1
void AcceleratorView::CalculateDelay(Arena* arena,bool outputDebugGraph){
   CalculateDAGOrdering(arena);

   // Clear everything, just in case
   for(ComplexFUInstance** instPtr : nodes){
      ComplexFUInstance* inst = *instPtr;

      inst->graphData->inputDelay = 0;

      for(ConnectionInfo& info : inst->graphData->allOutputs){
         *info.delay = 0;
      }
      for(ConnectionInfo& info : inst->graphData->allInputs){
         *info.delay = 0;
      }
   }

   if(outputDebugGraph && versat->debug.outputGraphs){
      char buffer[1024];
      sprintf(buffer,"debug/%.*s/",UNPACK_SS(accel->subtype->name));
      MakeDirectory(buffer);
   }

   int graphs = 0;
   for(int i = 0; i < order.numberSources; i++){
      ComplexFUInstance* inst = order.sources[i];

      inst->graphData->inputDelay = 0;
      inst->baseDelay = 0;

      SendLatencyUpwards(inst);

      #if 1
      if(outputDebugGraph && versat->debug.outputGraphs){
         OutputGraphDotFile(versat,*this,true,inst,"debug/%.*s/out1_%d.dot",UNPACK_SS(accel->subtype->name),graphs++);
      }
      #endif
   }

   for(int i = order.numberSources; i < nodes.Size(); i++){
      ComplexFUInstance* inst = order.instances[i];

      if(inst->graphData->nodeType == GraphComputedData::TAG_UNCONNECTED){
         continue;
      }

      int maximum = -(1 << 30);
      for(ConnectionInfo& info : inst->graphData->allInputs){
         maximum = std::max(maximum,*info.delay);
      }

      #if 1
      for(ConnectionInfo& info : inst->graphData->allInputs){
         *info.delay = maximum - *info.delay;
      }
      #endif

      inst->graphData->inputDelay = maximum;
      inst->baseDelay = maximum;

      if(inst->graphData->nodeType != GraphComputedData::TAG_SOURCE_AND_SINK){
         SendLatencyUpwards(inst);
      }

      #if 1
      if(outputDebugGraph && versat->debug.outputGraphs){
         OutputGraphDotFile(versat,*this,true,inst,"debug/%.*s/out2_%d.dot",UNPACK_SS(accel->subtype->name),graphs++);
      }
      #endif
   }

   #if 1
   for(int i = 0; i < order.numberSinks; i++){
      ComplexFUInstance* inst = order.sinks[i];

      if(inst->graphData->nodeType != GraphComputedData::TAG_SOURCE_AND_SINK){
         continue;
      }

      // Source and sink units never have output delay. They can't
      for(ConnectionInfo& info : inst->graphData->allOutputs){
         *info.delay = 0;
      }
   }
   #endif

   int minimum = 0;
   for(ComplexFUInstance** instPtr : nodes){
      ComplexFUInstance* inst = *instPtr;
      minimum = std::min(minimum,inst->graphData->inputDelay);
   }
   for(ComplexFUInstance** instPtr : nodes){
      ComplexFUInstance* inst = *instPtr;
      inst->graphData->inputDelay = inst->graphData->inputDelay - minimum;
   }

   if(outputDebugGraph && versat->debug.outputGraphs){
      OutputGraphDotFile(versat,*this,true,"debug/%.*s/out3.dot",UNPACK_SS(accel->subtype->name));
   }

   #if 1
   for(int i = 0; i < order.numberSources; i++){
      ComplexFUInstance* inst = order.sources[i];

      int minimum = 1 << 30;
      for(ConnectionInfo& info : inst->graphData->allOutputs){
         minimum = std::min(minimum,*info.delay);
      }

      // Does not take into account unit latency
      inst->graphData->inputDelay = minimum;
      inst->baseDelay = minimum;

      for(ConnectionInfo& info : inst->graphData->allOutputs){
         *info.delay -= minimum;
      }
   }
   #endif

   if(outputDebugGraph && versat->debug.outputGraphs){
      OutputGraphDotFile(versat,*this,true,"debug/%.*s/out4.dot",UNPACK_SS(accel->subtype->name));
   }

   for(ComplexFUInstance** instPtr : nodes){
      ComplexFUInstance* inst = *instPtr;
      if(inst->graphData->nodeType == GraphComputedData::TAG_UNCONNECTED){
         inst->graphData->inputDelay = 0;
      } else if(inst->graphData->nodeType == GraphComputedData::TAG_SOURCE_AND_SINK){
         #if 0
         for(ConnectionInfo& info : inst->graphData->allInputs){
            *info.delay = 0;
         }
         #endif
      }
      inst->baseDelay = inst->graphData->inputDelay;
   }
}
#endif

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
      for(ConnectionInfo& info : inst->graphData->allInputs){
         count += Visit(ordering,info.instConnectedTo.inst);
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
   order.instances = PushArray<ComplexFUInstance*>(arena,nodes.Size()).data;

   for(ComplexFUInstance** instPtr : nodes){
      ComplexFUInstance* inst = *instPtr;
      inst->tag = 0;
   }

   ComplexFUInstance** sourceUnits = order.instances;
   order.sources = sourceUnits;
   // Add source units, guaranteed to come first
   for(ComplexFUInstance** instPtr : nodes){
      ComplexFUInstance* inst = *instPtr;
      if(inst->graphData->nodeType == GraphComputedData::TAG_SOURCE || (inst->graphData->nodeType == GraphComputedData::TAG_SOURCE_AND_SINK && CHECK_DELAY(inst,DelayType::DELAY_TYPE_SOURCE_DELAY))){
         *(sourceUnits++) = inst;
         order.numberSources += 1;
         inst->tag = TAG_PERMANENT_MARK;
      }
   }

   // Add compute units
   ComplexFUInstance** computeUnits = sourceUnits;
   order.computeUnits = computeUnits;

   for(ComplexFUInstance** instPtr : nodes){
      ComplexFUInstance* inst = *instPtr;
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

   for(ComplexFUInstance** instPtr : nodes){
      ComplexFUInstance* inst = *instPtr;
      if(inst->graphData->nodeType == GraphComputedData::TAG_SINK || (inst->graphData->nodeType == GraphComputedData::TAG_SOURCE_AND_SINK && CHECK_DELAY(inst,DelayType::DELAY_TYPE_SINK_DELAY))){
         *(sinkUnits++) = inst;
         order.numberSinks += 1;
         Assert(inst->tag == 0);
         inst->tag = TAG_PERMANENT_MARK;
      }
   }


   for(ComplexFUInstance** instPtr : nodes){
      ComplexFUInstance* inst = *instPtr;
      Assert(inst->tag == TAG_PERMANENT_MARK);
   }

   Assert(order.numberSources + order.numberComputeUnits + order.numberSinks == nodes.Size());

   // Calculate order
   for(int i = 0; i < order.numberSources; i++){
      ComplexFUInstance* inst = order.sources[i];

      inst->graphData->order = 0;
   }

   for(int i = order.numberSources; i < nodes.Size(); i++){
      ComplexFUInstance* inst = order.instances[i];

      int max = 0;
      for(ConnectionInfo& info : inst->graphData->allInputs){
         max = std::max(max,info.instConnectedTo.inst->graphData->order);
      }

      inst->graphData->order = (max + 1);
   }

   dagOrder = true;
   return order;
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
   if(versatData.size){
      return;
   }

   VersatComputedData* mem = PushArray<VersatComputedData>(arena,nodes.Size()).data;

   CalculateGraphData(arena);

   for(ComplexFUInstance** instPtr : nodes){
      ComplexFUInstance* inst = *instPtr;
      if(inst->declaration->type == FUDeclaration::COMPOSITE && inst->declaration->fixedDelayCircuit){
         AcceleratorView view = CreateAcceleratorView(inst->declaration->fixedDelayCircuit,arena);
         view.CalculateVersatData(arena);
      }
   }

   auto Compare = [](const HuffmanBlock* a, const HuffmanBlock* b) {
      return a->bits > b->bits;
   };

   Pool<HuffmanBlock> blocks = {};
   std::priority_queue<HuffmanBlock*,std::vector<HuffmanBlock*>,decltype(Compare)> huffQueue(Compare);

   for(ComplexFUInstance** instPtr : nodes){
      ComplexFUInstance* inst = *instPtr;
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
   for(ComplexFUInstance** instPtr : nodes){
      ComplexFUInstance* inst = *instPtr;
      if(inst->declaration->isMemoryMapped){
         maxMask = std::max(maxMask,inst->versatData->memoryMaskSize + inst->declaration->memoryMapBits);
      }
   }

   for(ComplexFUInstance** instPtr : nodes){
      ComplexFUInstance* inst = *instPtr;
      if(inst->declaration->isMemoryMapped){
         int memoryAddressOffset = 0;
         for(int i = 0; i < inst->versatData->memoryMaskSize; i++){
            int bit = inst->versatData->memoryMask[i] - '0';

            memoryAddressOffset += bit * (1 << (maxMask - i - 1));
         }
         inst->versatData->memoryAddressOffset = memoryAddressOffset;

         // TODO: Not a good way of dealing with calculating memory map to lower units, but should suffice for now
         Accelerator* subAccel = inst->declaration->fixedDelayCircuit;
         if(inst->declaration->type == FUDeclaration::COMPOSITE && subAccel){
            AcceleratorIterator iter = {};
            for(ComplexFUInstance* inst = iter.Start(subAccel,arena,false); inst; inst = iter.Next()){
               if(inst->declaration->isMemoryMapped){
                  inst->versatData->memoryAddressOffset += memoryAddressOffset;
               }
            };
         }
      }
   }

   blocks.Clear(true);
}

AcceleratorView CreateAcceleratorView(Accelerator* accel,Arena* arena){
   AcceleratorView view = {};

   view.versat = accel->versat;
   view.accel = accel;

   std::unordered_map<ComplexFUInstance*,ComplexFUInstance**> map;
   for(ComplexFUInstance* inst : accel->instances){
      ComplexFUInstance** space = view.nodes.Alloc();
      map.insert({inst,space});
      *space = inst;
   }

   for(Edge* edge : accel->edges){
      EdgeView newEdge = {};

      ComplexFUInstance* inst0 = edge->units[0].inst;
      ComplexFUInstance* inst1 = edge->units[1].inst;

      newEdge.edge = edge;
      newEdge.nodes[0] = map.find(inst0)->second;
      newEdge.nodes[1] = map.find(inst1)->second;

      *view.edges.Alloc() = newEdge;
   }

   return view;
}

AcceleratorView CreateAcceleratorView(Accelerator* accel,std::vector<Edge*>& edgeMappings,Arena* arena){
   AcceleratorView view = {};

   view.versat = accel->versat;
   view.accel = accel;

   // Allocate edges first
   for(Edge* edge : edgeMappings){
      EdgeView* newEdge = view.edges.Alloc();

      newEdge->edge = edge;
   }

   std::unordered_map<ComplexFUInstance*,int> map;

   for(EdgeView* edgeView : view.edges){
      Edge* edge = edgeView->edge;

      for(int ii = 0; ii < 2; ii++){
         ComplexFUInstance* node = edge->units[ii].inst;

         auto iter = map.find(node);
         ComplexFUInstance** mapping = nullptr;
         if(iter == map.end()){
            ComplexFUInstance** newNode = view.nodes.Alloc();
            map.insert({node,view.nodes.Size()});
            *newNode = node;

            mapping = newNode;
         } else {
            mapping = view.nodes.Get(iter->second);
         }

         edgeView->nodes[ii] = mapping;
      }
   }

   return view;
}

AcceleratorView SubGraphAroundInstance(Versat* versat,Accelerator* accel,ComplexFUInstance* instance,int layers,Arena* arena){
   AcceleratorView view = {};

   // Allocate edges first
   for(Edge* edge : accel->edges){
      if(!(edge->units[0].inst == instance || edge->units[1].inst == instance)){
         continue;
      }

      EdgeView* newEdge = view.edges.Alloc();

      newEdge->edge = edge;
   }

   std::unordered_map<ComplexFUInstance*,int> map;

   for(EdgeView* edgeView : view.edges){
      Edge* edge = edgeView->edge;

      for(int ii = 0; ii < 2; ii++){
         ComplexFUInstance* node = edge->units[ii].inst;

         auto iter = map.find(node);
         ComplexFUInstance** mapping = nullptr;
         if(iter == map.end()){
            ComplexFUInstance** newNode = view.nodes.Alloc();
            map.insert({node,view.nodes.Size()});
            *newNode = node;

            mapping = newNode;
         } else {
            mapping = view.nodes.Get(iter->second);
         }

         edgeView->nodes[ii] = mapping;
      }
   }

   return view;
}

int CalculateTotalOutputs(Accelerator* accel){
   int total = 0;
   for(ComplexFUInstance* inst : accel->instances){
      total += inst->declaration->totalOutputs;
   }

   return total;
}

int CalculateTotalOutputs(FUInstance* inst){
   int total = 0;
   if(inst->declaration->fixedDelayCircuit){
      total += CalculateTotalOutputs(inst->declaration->fixedDelayCircuit);
   }
   total += inst->declaration->outputLatencies.size;

   return total;
}

bool IsUnitCombinatorial(FUInstance* instance){
   FUDeclaration* type = instance->declaration;

   if(type->outputLatencies.size && type->outputLatencies[0] == 0){
      return true;
   } else {
      return false;
   }
}

#if 0
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
#endif

void FixMultipleInputs(Versat* versat,Accelerator* accel){
   Arena* arena = &versat->temp;
   ArenaMarker marker(arena);
   AcceleratorView view = CreateAcceleratorView(accel,arena);
   view.CalculateGraphData(arena);

   Assert(view.graphData.size);

   static int multiplexersInstantiated = 0;

   bool isComb = true;
   int portUsedCount[99];
   for(ComplexFUInstance** instPtr : view.nodes){
      ComplexFUInstance* inst = *instPtr;
      if(inst->declaration == BasicDeclaration::multiplexer || inst->declaration == BasicDeclaration::combMultiplexer){ // Not good, but works for now (otherwise newly added muxes would break the algorithm)
         continue;
      }

      Memset(portUsedCount,0,99);

      if(!IsUnitCombinatorial(inst)){
         isComb = false;
      }

      for(ConnectionInfo& info : inst->graphData->allInputs){
         Assert(info.port < 99 && info.port >= 0);
         portUsedCount[info.port] += 1;

         if(!IsUnitCombinatorial(info.instConnectedTo.inst)){
            isComb = false;
         }
      }

      for(int port = 0; port < 99; port++){
         if(portUsedCount[port] > 1){ // Edge has more that one connection
            FUDeclaration* muxType = BasicDeclaration::multiplexer;
            const char* format = "mux%d";

            if(isComb){
               muxType = BasicDeclaration::combMultiplexer;
               format = "comb_mux%d";
            }

            SizedString name = PushString(&versat->permanent,format,multiplexersInstantiated++);
            ComplexFUInstance* multiplexer = (ComplexFUInstance*) CreateFUInstance(accel,muxType,name);

            // Connect edges to multiplexer
            int portsConnected = 0;
            for(EdgeView* edgeView : view.edges){
               Edge* edge = edgeView->edge;
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

// Fixes edges such that unit before connected to after, are reconnected to new unit
void InsertUnit(Accelerator* accel, PortInstance before, PortInstance after, PortInstance newUnit){
   for(Edge* edge : accel->edges){
      if(edge->units[0] == before && edge->units[1] == after){
         ConnectUnits(newUnit,after);

         edge->units[1] = newUnit;

         return;
      }
   }

   Assert(false);
}

void InsertUnit(AcceleratorView& view, PortInstance before, PortInstance after, PortInstance newUnit,ComplexFUInstance** newNode){
   for(EdgeView* edgeView : view.edges){
      Edge* edge = edgeView->edge;
      if(edge->units[0] == before && edge->units[1] == after){
         Edge* newEdge = ConnectUnits(newUnit,after);

         EdgeView* newEdgeView = view.edges.Alloc();

         newEdgeView->edge = newEdge;
         newEdgeView->nodes[0] = newNode;
         newEdgeView->nodes[1] = edgeView->nodes[1];

         edge->units[1] = newUnit;
         edgeView->nodes[1] = newNode;

         return;
      }
   }

   Assert(false);
}

void FixDelays(Versat* versat,Accelerator* accel,AcceleratorView& view){
   Assert(view.graphData.size && view.dagOrder);

   DAGOrder order = view.order;

   int buffersInserted = 0;
   int fixed = 0;
   for(int i = accel->instances.Size() - 1; i >= 0; i--){
      ComplexFUInstance* inst = order.instances[i];

      if(inst->declaration == BasicDeclaration::buffer || inst->declaration == BasicDeclaration::fixedBuffer){
         continue;
      }

      for(ConnectionInfo& info : inst->graphData->allInputs){
         ComplexFUInstance* other = info.instConnectedTo.inst;
         PortInstance start = info.instConnectedTo;
         PortInstance end = PortInstance{inst,info.port};

         int delay = *info.delay;

         if(other->declaration == BasicDeclaration::buffer){
            other->baseDelay = delay;
         } else if(other->declaration == BasicDeclaration::fixedBuffer){
            //NOT_IMPLEMENTED;
         } else if(delay > 0){
            if(versat->debug.useFixedBuffers){
               SizedString bufferName = PushString(&versat->permanent,"fixedBuffer%d",buffersInserted++);

               ComplexFUInstance* unit = (ComplexFUInstance*) CreateFUInstance(accel,BasicDeclaration::fixedBuffer,bufferName,false,false);
               unit->parameters = PushString(&versat->permanent,"#(.AMOUNT(%d))",delay - BasicDeclaration::fixedBuffer->outputLatencies[0]);

               ComplexFUInstance** viewNode = view.nodes.Alloc();
               *viewNode = unit;

               InsertUnit(view,start,end,PortInstance{unit,0},viewNode);

               unit->bufferAmount = delay - BasicDeclaration::fixedBuffer->outputLatencies[0];

               if(versat->debug.outputGraphs){
                  OutputGraphDotFile(versat,view,false,unit,"debug/%.*s/fix_%d.dot",UNPACK_SS(accel->subtype->name),++fixed);
               }
            } else {
               SizedString bufferName = PushString(&versat->permanent,"buffer%d",buffersInserted++);

               ComplexFUInstance* unit = (ComplexFUInstance*) CreateFUInstance(accel,BasicDeclaration::buffer,bufferName,false,true);
               ComplexFUInstance** viewNode = view.nodes.Alloc();
               *viewNode = unit;

               InsertUnit(view,start,end,PortInstance{unit,0},viewNode);

               unit->bufferAmount = delay - BasicDeclaration::buffer->outputLatencies[0];
               Assert(unit->bufferAmount >= 0);

               if(unit->config){
                  unit->config[0] = unit->bufferAmount;
               }
               if(versat->debug.outputGraphs){
                  OutputGraphDotFile(versat,view,false,unit,"debug/%.*s/fix_%d.dot",UNPACK_SS(accel->subtype->name),++fixed);
               }
            }
         } else {
            if(versat->debug.outputGraphs){
               OutputGraphDotFile(versat,view,false,"debug/%.*s/fix_%d_nothing.dot",UNPACK_SS(accel->subtype->name),fixed);
            }
            Assert(delay >= 0);
         }
      }
   }

   for(ComplexFUInstance* inst : accel->instances){
      if(inst->graphData){
         inst->graphData->inputDelay = inst->baseDelay;

         Assert(inst->baseDelay >= 0);
         Assert(inst->baseDelay < 9999); // Unless dealing with really long accelerators, should catch some calculating bugs

         if(inst->declaration == BasicDeclaration::buffer && inst->config){
            inst->config[0] = inst->baseDelay;
         }
      }
   }

   if(versat->debug.outputGraphs){
      AcceleratorView newView = CreateAcceleratorView(accel,&versat->temp);
      newView.CalculateDAGOrdering(&versat->temp);
      newView.CalculateGraphData(&versat->temp);
      OutputGraphDotFile(versat,newView,true,"debug/%.*s/fixDelay.dot",UNPACK_SS(accel->subtype->name));
   }
}

void ActivateMergedAcceleratorRecursive(Versat* versat,ComplexFUInstance* inst, int index){
   // Accelerator iterator inside another accelerator iterator?

   #if 0
   if(inst->declaration->fixedDelayCircuit){
      AcceleratorIterator iter = {};

      for(ComplexFUInstance* subInst = iter.Start(inst->declaration->fixedDelayCircuit,&versat->temp,true); subInst; subInst = iter.Next()){
         ActivateMergedAcceleratorRecursive(versat,subInst,index);
      }
   } else {
      if(BasicDeclaration::multiplexer == inst->declaration || BasicDeclaration::combMultiplexer == inst->declaration){
         inst->config[0] = index;
      }
   }
   #endif
}

void ActivateMergedAccelerator(Versat* versat,Accelerator* accel,FUDeclaration* type){
   // Accelerator iterator inside another accelerator iterator?

   #if 0
   AcceleratorIterator iter = {};

   for(ComplexFUInstance* inst = iter.Start(accel,&versat->temp,true); inst; inst = iter.Next()){
      for(int i = 0; i < 2; i++){
         if(inst->declaration->mergedType[i] == type){
            ActivateMergedAcceleratorRecursive(versat,inst,i);
         }
      }
   }
   #endif
}

ComplexFUInstance* GetInputInstance(Accelerator* accel,int inputIndex){
   for(ComplexFUInstance* inst : accel->instances){
      if(inst->declaration == BasicDeclaration::input && inst->id == inputIndex){
         return inst;
      }
   }
   return nullptr;
}

ComplexFUInstance* GetOutputInstance(Accelerator* accel){
   for(ComplexFUInstance* inst : accel->instances){
      if(inst->declaration == BasicDeclaration::output){
         return inst;
      }
   }

   return nullptr;
}


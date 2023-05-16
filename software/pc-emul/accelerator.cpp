#include "versatPrivate.hpp"

#include <unordered_map>
#include <queue>

#include "debug.hpp"
#include "debugGUI.hpp"
#include "configurations.hpp"
#include "graph.hpp"

#define TAG_TEMPORARY_MARK 1
#define TAG_PERMANENT_MARK 2

typedef std::unordered_map<ComplexFUInstance*,ComplexFUInstance*> InstanceMap;

Accelerator* CreateAccelerator(Versat* versat){
   Accelerator* accel = versat->accelerators.Alloc();
   accel->versat = versat;

   accel->accelMemory = CreateDynamicArena(1);

   return accel;
}

void DestroyAccelerator(Versat* versat,Accelerator* accel){
   Free(&accel->configAlloc);
   Free(&accel->stateAlloc);
   Free(&accel->delayAlloc);
   Free(&accel->staticAlloc);
   Free(&accel->outputAlloc);
   Free(&accel->storedOutputAlloc);
   Free(&accel->extraDataAlloc);
   Free(&accel->externalMemoryAlloc);

   //Free(&accel->instancesMemory);
   //Free(&accel->temp);

   versat->accelerators.Remove(accel);
}

void InitializeSubaccelerator(AcceleratorIterator iter){
   int staticIndex = 0;
   ComplexFUInstance* parent = iter.CurrentAcceleratorInstance()->inst;
   FUDeclaration* decl = parent->declaration;

   for(InstanceNode* node = iter.Current(); node; node = iter.Skip()){
      ComplexFUInstance* inst = node->inst;
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

      if(IsTypeHierarchical(inst->declaration)){
         AcceleratorIterator it = iter.LevelBelowIterator();
         InitializeSubaccelerator(it);
      }
   }
}

InstanceNode* CreateFlatFUInstance(Accelerator* accel,FUDeclaration* type,String name){
   Assert(CheckValidName(name));

   accel->ordered = nullptr; // TODO: Will leak

   FOREACH_LIST(ptr,accel->allocated){
      if(ptr->inst->name == name){
         //Assert(false);
         break;
      }
   }

   ComplexFUInstance* inst = accel->instances.Alloc();
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
   inst->id = accel->entityId++;
   inst->accel = accel;
   inst->declaration = type;
   inst->namedAccess = true;

   for(Pair<StaticId,StaticData>& pair : type->staticUnits){
      StaticId first = pair.first;
      StaticData second = pair.second;
      accel->staticUnits.insert({first,second});
   }

   return ptr;
}

InstanceNode* CreateAndConfigureFUInstance(Accelerator* accel,FUDeclaration* type,String name){
   //SetDebuggingValue(MakeValue(accel,"Accelerator"));

   accel->ordered = nullptr; // Added recently, might cause bugs

   Arena* arena = &accel->versat->temp;
   BLOCK_REGION(arena);

   InstanceNode* node = CreateFlatFUInstance(accel,type,name);
   ComplexFUInstance* inst = node->inst;
   inst->declarationInstance = inst;

   //if(!flat){ // TODO: before this was true
   //if(false){
   if(true){
      // The position of top level newly created units is always at the end of the current allocated configurations
      int configOffset = accel->configAlloc.size;
      int stateOffset = accel->stateAlloc.size;
      int delayOffset = accel->delayAlloc.size;
      int outputOffset = accel->outputAlloc.size;
      int extraDataOffset = accel->extraDataAlloc.size;
      int externalDataOffset = accel->externalMemoryAlloc.size;
      //int debugData = accel->debugDataAlloc.size;

      iptr memMappedStart = -1;
      iptr memMappedEnd = 0;
      FOREACH_LIST(ptr,accel->allocated){
         if(ptr != node && ptr->inst->declaration->isMemoryMapped){
            memMappedStart = std::max(memMappedStart,(iptr) ptr->inst->memMapped);
            memMappedEnd = memMappedStart + (1 << ptr->inst->declaration->memoryMapBits);
         }
      }

      UnitValues val = CalculateAcceleratorValues(accel->versat,accel);
      int numberUnits = CalculateNumberOfUnits(accel->allocated);

      AssertAndDo(!ZeroOutRealloc(&accel->configAlloc,val.configs));
      AssertAndDo(!ZeroOutRealloc(&accel->stateAlloc,val.states));
      AssertAndDo(!ZeroOutRealloc(&accel->delayAlloc,val.delays));
      AssertAndDo(!ZeroOutRealloc(&accel->outputAlloc,val.totalOutputs));
      AssertAndDo(!ZeroOutRealloc(&accel->storedOutputAlloc,val.totalOutputs));
      AssertAndDo(!ZeroOutRealloc(&accel->extraDataAlloc,val.extraData));
      AssertAndDo(!ZeroOutRealloc(&accel->staticAlloc,val.statics));
      AssertAndDo(!ZeroOutRealloc(&accel->externalMemoryAlloc,val.externalMemorySize));
      AssertAndDo(!ZeroOutRealloc(&accel->debugDataAlloc,numberUnits));

      #if 1
      if(type->configs.size) inst->config = &accel->configAlloc.ptr[configOffset];
      if(type->states.size) inst->state = &accel->stateAlloc.ptr[stateOffset];
      if(type->nDelays) inst->delay = &accel->delayAlloc.ptr[delayOffset];
      if(type->outputLatencies.size || type->totalOutputs) inst->outputs = &accel->outputAlloc.ptr[outputOffset];
      if(type->outputLatencies.size || type->totalOutputs) inst->storedOutputs = &accel->storedOutputAlloc.ptr[outputOffset];
      if(type->extraDataSize) inst->extraData = &accel->extraDataAlloc.ptr[extraDataOffset];
      if(type->externalMemory.size) inst->externalMemory = &accel->externalMemoryAlloc.ptr[externalDataOffset];

      if(type->memoryMapBits){
         if(memMappedStart == -1){
            inst->memMapped = nullptr;
         } else {
            inst->memMapped = (int*) memMappedEnd;
         }
      }
      #endif

      // Initialize sub units
      if(IsTypeHierarchical(type)){
         // Fix static info
         for(Pair<StaticId,StaticData>& pair : type->staticUnits){
            #if 0
            bool found = false;
            for(auto& info : accel->staticInfo){
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
            #endif // 0

            StaticId first = pair.first;
            StaticData second = pair.second;
            accel->staticUnits.insert({first,second});
         }

         PopulateTopLevelAccelerator(accel);
         AcceleratorIterator iter = {};
         iter.Start(accel,&accel->versat->temp,true);

         for(InstanceNode* node = iter.Start(accel,&accel->versat->temp,true); node; node = iter.Skip()){
            ComplexFUInstance* other = node->inst;
            // Have to iterate the top units until reaching the one we just created
            if(other == inst){
               AcceleratorIterator it = iter.LevelBelowIterator();
               InitializeSubaccelerator(it);
            }
         }
      } else if(type->initializeFunction){
         #if 1 // Force population of top accelerator
         {
         PopulateTopLevelAccelerator(accel);
         AcceleratorIterator iter = {};
         iter.Start(accel,&accel->versat->temp,true);
         }
         #endif

         type->initializeFunction(inst);
      }
   }

   return node;
}

FUInstance* CreateFUInstance(Accelerator* accel,FUDeclaration* type,String name){
   FUInstance* res = CreateAndConfigureFUInstance(accel,type,name)->inst;

   return res;
}

Accelerator* CopyAccelerator(Versat* versat,Accelerator* accel,InstanceMap* map){
   Accelerator* newAccel = CreateAccelerator(versat);
   InstanceMap nullCaseMap;

   if(map == nullptr){
      map = &nullCaseMap;
   }

   // Copy of instances
   FOREACH_LIST(ptr,accel->allocated){
      ComplexFUInstance* inst = ptr->inst;
      ComplexFUInstance* newInst = CopyInstance(newAccel,inst,inst->name);
      newInst->declarationInstance = inst;

      if(inst->config){ // Added
         Memcpy(newInst->config,inst->config,inst->declaration->configs.size);
      }
      newInst->savedConfiguration = inst->savedConfiguration;
      newInst->literal = inst->literal;

      map->insert({inst,newInst});
   }

   // Flat copy of edges
   FOREACH_LIST(edge,accel->edges){
      ComplexFUInstance* out = (ComplexFUInstance*) map->at(edge->units[0].inst);
      int outPort = edge->units[0].port;
      ComplexFUInstance* in = (ComplexFUInstance*) map->at(edge->units[1].inst);
      int inPort = edge->units[1].port;

      ConnectUnits(out,outPort,in,inPort,edge->delay);
   }

   return newAccel;
}

ComplexFUInstance* CopyInstance(Accelerator* newAccel,ComplexFUInstance* oldInstance,String newName){
   ComplexFUInstance* newInst = CreateAndConfigureFUInstance(newAccel,oldInstance->declaration,newName)->inst;

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
   InstanceNode* newNode = CreateAndConfigureFUInstance(newAccel,oldInstance->inst->declaration,newName);
   ComplexFUInstance* newInst = newNode->inst;

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

ComplexFUInstance* CopyFlatInstance(Accelerator* newAccel,ComplexFUInstance* oldInstance,String newName){
   ComplexFUInstance* newInst = CreateFlatFUInstance(newAccel,oldInstance->declaration,newName)->inst;

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
   Accelerator* newAccel = CreateAccelerator(versat);
   InstanceMap nullCaseMap;

   if(map == nullptr){
      map = &nullCaseMap;
   }

   // Copy of instances
   FOREACH_LIST(ptr,accel->allocated){
      ComplexFUInstance* inst = ptr->inst;
      ComplexFUInstance* newInst = CopyFlatInstance(newAccel,inst,inst->name);
      newInst->declarationInstance = inst;

      newInst->savedConfiguration = inst->savedConfiguration;
      newInst->literal = inst->literal;

      map->insert({inst,newInst});
   }

   // Flat copy of edges
   FOREACH_LIST(edge,accel->edges){
      ComplexFUInstance* out = (ComplexFUInstance*) map->at(edge->units[0].inst);
      int outPort = edge->units[0].port;
      ComplexFUInstance* in = (ComplexFUInstance*) map->at(edge->units[1].inst);
      int inPort = edge->units[1].port;

      ConnectUnits(out,outPort,in,inPort,edge->delay);
   }

   return newAccel;
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

// This function works, but not for the flatten algorithm, as we hope that the configs match but they don't match for the outputs after removing a composite instance
// We could, on the other hand, push the outputs of a composite instance to after the outputs of the subinstances, at which point it should work
void RemoveFUInstance(Accelerator* accel,InstanceNode* node){
   //UNHANDLED_ERROR; // Need to update graph data.

   ComplexFUInstance* inst = node->inst;

   FOREACH_LIST(edge,accel->edges){
      if(edge->units[0].inst == inst){
         accel->edges = ListRemove(accel->edges,edge);
      } else if(edge->units[1].inst == inst){
         accel->edges = ListRemove(accel->edges,edge);
      }
   }

   #if 0
   // Change allocations for all the other units
   int nConfigs = inst->declaration->configs.size;
   int nStates = inst->declaration->states.size;
   int nDelays = inst->declaration->nDelays;
   int nOutputs = inst->declaration->totalOutputs;
   int nExtraData = inst->declaration->extraDataSize;

   int* oldConfig = inst->config;
   int* oldState = inst->state;
   int* oldDelay = inst->delay;
   int* oldOutputs = inst->outputs;
   int* oldStoredOutputs = inst->storedOutputs;
   unsigned char* oldExtraData = inst->extraData;

   if(inst->config) RemoveChunkAndCompress(&accel->configAlloc,inst->config,nConfigs);
   if(inst->state) RemoveChunkAndCompress(&accel->stateAlloc,inst->state,nStates);
   if(inst->delay) RemoveChunkAndCompress(&accel->delayAlloc,inst->delay,nDelays);
   if(inst->outputs) RemoveChunkAndCompress(&accel->outputAlloc,inst->outputs,nOutputs);
   if(inst->storedOutputs) RemoveChunkAndCompress(&accel->storedOutputAlloc,inst->storedOutputs,nOutputs);
   if(inst->extraData) RemoveChunkAndCompress(&accel->extraDataAlloc,inst->extraData,nExtraData);
   #endif

   #if 0
   FOREACH_LIST(ptr,node->allInputs){
      RemoveConnection(ptr->instConnectedTo.node,ptr->instConnectedTo.port,node,ptr->port);
   }
   FOREACH_LIST(ptr,node->allOutputs){
      RemoveConnection(node,ptr->port,ptr->instConnectedTo.node,ptr->instConnectedTo.port);
   }
   FixInputs(node);

   // Remove instance
   int oldSize = Size(accel->allocated);
   accel->allocated = ListRemove(accel->allocated,node);
   int newSize = Size(accel->allocated);
   Assert(oldSize == newSize + 1);
   #endif

   accel->allocated = RemoveUnit(accel->allocated,node);

   #if 0
   // Fix config pointers
   FOREACH_LIST(in,accel->instances){
      if(!IsConfigStatic(accel,in) && in->config > oldConfig){
         in->config -= nConfigs;
      }
      if(in->state > oldState){
         in->state -= nStates;
      }
      if(in->delay > oldDelay){
         in->delay -= nDelays;
      }
      #if 1
      if(in->outputs > oldOutputs){
         in->outputs -= nOutputs;
      }
      if(in->storedOutputs > oldStoredOutputs){
         in->outputs -= nOutputs;
      }
      #endif
      if(in->extraData > oldExtraData){
         in->extraData -= nExtraData;
      }
   }
   #endif
}

ComplexFUInstance GetNodeByName(Accelerator* accel,String string,Arena* arena){
   AcceleratorIterator iter = {};
   for(InstanceNode* node = iter.Start(accel,arena,true); node; node = iter.Next()){
      BLOCK_REGION(arena);
      String name = iter.GetFullName(arena);

      if(CompareString(name,string)){
         return *node->inst;
      }
   }
   Assert(false);
   return (ComplexFUInstance){};
}

Accelerator* Flatten(Versat* versat,Accelerator* accel,int times){
   Arena* arena = &versat->temp;
   BLOCK_REGION(arena);

   InstanceMap map;
   Accelerator* newAccel = CopyFlatAccelerator(versat,accel,&map);
   map.clear();

   Pool<InstanceNode*> compositeInstances = {};
   Pool<InstanceNode*> toRemove = {};
   std::unordered_map<StaticId,int> staticToIndex;

   for(int i = 0; i < times; i++){
      int maxSharedIndex = -1;
      FOREACH_LIST(instPtr,newAccel->allocated){
         ComplexFUInstance* inst = instPtr->inst;
         if(inst->declaration->type == FUDeclaration::COMPOSITE){
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
         ComplexFUInstance* inst = (*instPtr)->inst;

         Assert(inst->declaration->type == FUDeclaration::COMPOSITE);

         count += 1;
         Accelerator* circuit = inst->declaration->baseCircuit; // TODO: we replaced fixedDelay with base circuit. Future care
         ComplexFUInstance* outputInstance = GetOutputInstance(circuit->allocated);

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
         FOREACH_LIST(ptr,circuit->allocated){
            ComplexFUInstance* circuitInst = ptr->inst;
            if(circuitInst->declaration->type == FUDeclaration::SPECIAL){
               continue;
            }

            String newName = PushString(&versat->permanent,"%.*s.%.*s",UNPACK_SS(inst->name),UNPACK_SS(circuitInst->name));
            ComplexFUInstance* newInst = CopyFlatInstance(newAccel,circuitInst,newName);

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
         FOREACH_LIST(edge,newAccel->edges){
            if(edge->units[0].inst == inst){
               FOREACH_LIST(circuitEdge,circuit->edges){
                  if(circuitEdge->units[1].inst == outputInstance && circuitEdge->units[1].port == edge->units[0].port){
                     auto iter = map.find(circuitEdge->units[0].inst);

                     if(iter == map.end()){
                        continue;
                     }

                     ComplexFUInstance* out = iter->second;
                     ComplexFUInstance* in = edge->units[1].inst;
                     int outPort = circuitEdge->units[0].port;
                     int inPort = edge->units[1].port;
                     int delay = edge->delay + circuitEdge->delay;

                     ConnectUnits(out,outPort,in,inPort,delay);
                  }
               }
            }
         }

         // Add accel edges to input instances
         FOREACH_LIST(edge,newAccel->edges){
            if(edge->units[1].inst == inst){
               ComplexFUInstance* circuitInst = GetInputInstance(circuit->allocated,edge->units[1].port);

               FOREACH_LIST(circuitEdge,circuit->edges){
                  if(circuitEdge->units[0].inst == circuitInst){
                     auto iter = map.find(circuitEdge->units[1].inst);

                     if(iter == map.end()){
                        continue;
                     }

                     ComplexFUInstance* out = edge->units[0].inst;
                     ComplexFUInstance* in = iter->second;
                     int outPort = edge->units[0].port;
                     int inPort = circuitEdge->units[1].port;
                     int delay = edge->delay + circuitEdge->delay;

                     ConnectUnits(out,outPort,in,inPort,delay);
                  }
               }
            }
         }

         // Add circuit specific edges
         FOREACH_LIST(circuitEdge,circuit->edges){
            auto input = map.find(circuitEdge->units[0].inst);
            auto output = map.find(circuitEdge->units[1].inst);

            if(input == map.end() || output == map.end()){
               continue;
            }

            ComplexFUInstance* out = input->second;
            ComplexFUInstance* in = output->second;
            int outPort = circuitEdge->units[0].port;
            int inPort = circuitEdge->units[1].port;
            int delay = circuitEdge->delay;

            ConnectUnits(out,outPort,in,inPort,delay);
         }

         // Add input to output specific edges
         FOREACH_LIST(edge1,newAccel->edges){
            if(edge1->units[1].inst == inst){
               PortInstance input = edge1->units[0];
               ComplexFUInstance* circuitInput = GetInputInstance(circuit->allocated,edge1->units[1].port);

               FOREACH_LIST(edge2,newAccel->edges){
                  if(edge2->units[0].inst == inst){
                     PortInstance output = edge2->units[1];
                     int outputPort = edge2->units[0].port;

                     FOREACH_LIST(circuitEdge,circuit->edges){
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
         AssertGraphValid(newAccel->allocated,arena);

         map.clear();
      }

      #if 0
      for(ComplexFUInstance** instPtr : toRemove){
         ComplexFUInstance* inst = *instPtr;

         FlattenRemoveFUInstance(newAccel,inst);
      }
      #endif

      #if 0
      for(InstanceNode** instPtr : toRemove){
         RemoveFUInstance(newAccel,*instPtr);
      }
      AssertGraphValid(newAccel->allocated,arena);
      #endif

      toRemove.Clear();
      compositeInstances.Clear();
   }

   {
      ArenaMarker marker(arena);
      OutputGraphDotFile(versat,newAccel,true,"./debug/flatten.dot");
   }

   toRemove.Clear(true);
   compositeInstances.Clear(true);

   newAccel->staticUnits.clear();

   FUDeclaration base = {};
   base.name = STRING("Top");
   newAccel->subtype = &base;

   newAccel->ordered = nullptr;
   ReorganizeAccelerator(newAccel,arena);

   Hashmap<EdgeNode,int>* delay = CalculateDelay(versat,newAccel,arena);
   FixDelays(versat,newAccel,delay);

   newAccel->ordered = nullptr;
   ReorganizeAccelerator(newAccel,arena);

   UnitValues val = CalculateAcceleratorValues(versat,newAccel);
   AssertAndDo(!ZeroOutRealloc(&newAccel->configAlloc,val.configs));
   AssertAndDo(!ZeroOutRealloc(&newAccel->stateAlloc,val.states));
   AssertAndDo(!ZeroOutRealloc(&newAccel->delayAlloc,val.delays));
   AssertAndDo(!ZeroOutRealloc(&newAccel->outputAlloc,val.totalOutputs));
   AssertAndDo(!ZeroOutRealloc(&newAccel->storedOutputAlloc,val.totalOutputs));
   AssertAndDo(!ZeroOutRealloc(&newAccel->extraDataAlloc,val.extraData));
   AssertAndDo(!ZeroOutRealloc(&newAccel->staticAlloc,val.statics));
   AssertAndDo(!ZeroOutRealloc(&newAccel->externalMemoryAlloc,val.externalMemorySize));

   PopulateTopLevelAccelerator(newAccel);
   AcceleratorIterator iter = {};
   for(InstanceNode* node = iter.Start(newAccel,arena,true); node; node = iter.Next()){
      ComplexFUInstance* inst = node->inst;
      if(inst->declaration->initializeFunction){
         inst->declaration->initializeFunction(inst);
         inst->initialized = true;
      }
   }

   ReorganizeAccelerator(accel,arena);
   ReorganizeAccelerator(newAccel,arena);

   // Copy configuration
   region(arena){
      Hashmap<String,SizedConfig>* oldConfigs = ExtractNamedSingleConfigs(accel,arena);
      Hashmap<String,SizedConfig>* newConfigs = ExtractNamedSingleConfigs(newAccel,arena);

      for(Pair<String,SizedConfig>& pair : oldConfigs){
         SizedConfig oldConfig = pair.second;
         SizedConfig* possibleConfig = newConfigs->Get(pair.first);

         if(!possibleConfig){
            continue;
         } else {
            SizedConfig newConfig = *possibleConfig;
            Memcpy(newConfig.ptr,oldConfig.ptr,oldConfig.size);
         }
      }
   }

   return newAccel;
}

InstanceNode* AcceleratorIterator::Start(Accelerator* topLevel,ComplexFUInstance* compositeInst,Arena* arena,bool populate){
   this->topLevel = topLevel;
   this->level = 0;
   this->calledStart = true;
   this->populate = populate;
   FUDeclaration* decl = compositeInst->declaration;
   Accelerator* accel = compositeInst->declaration->fixedDelayCircuit;

   if(populate){
      FUInstanceInterfaces inter = {};
      inter.Init(topLevel,compositeInst);

      PopulateAccelerator(topLevel,accel,decl,inter,&topLevel->staticUnits);

      #if 1
      inter.AssertEmpty(false);
      #endif
   } else { // Detects bugs if we clear everything for the non populate case
      FOREACH_LIST(ptr,accel->allocated){
         ptr->inst->config = nullptr;
         ptr->inst->state = nullptr;
         ptr->inst->delay = nullptr;
         ptr->inst->outputs = nullptr;
         ptr->inst->storedOutputs = nullptr;
         ptr->inst->extraData = nullptr;
      }
   }

   stack = PushArray<AcceleratorIterator::Type>(arena,99);
   stack[0].node = compositeInst->declaration->fixedDelayCircuit->allocated;
   InstanceNode* inst = GetInstance(level);

   return inst;
}

InstanceNode* AcceleratorIterator::Start(Accelerator* topLevel,Arena* arena,bool populate){
   this->level = 0;
   this->calledStart = true;
   this->topLevel = topLevel;
   this->populate = populate;

   #if 0
   if(populate){
      PopulateTopLevelAccelerator(topLevel);
      inter.AssertEmpty(false);
   } else { // Detects bugs if we clear everything for the non populate case
      FOREACH_LIST(ptr,topLevel->allocated){
         ptr->inst->config = nullptr;
         ptr->inst->state = nullptr;
         ptr->inst->delay = nullptr;
         ptr->inst->outputs = nullptr;
         ptr->inst->storedOutputs = nullptr;
         ptr->inst->extraData = nullptr;
      }
   }
   #endif

   stack = PushArray<AcceleratorIterator::Type>(arena,99);
   stack[0].node = topLevel->allocated;
   InstanceNode* inst = GetInstance(level);

   return inst;
}

InstanceNode* AcceleratorIterator::StartOrdered(Accelerator* topLevel,Arena* arena,bool populate){
   this->level = 0;
   this->calledStart = true;
   this->topLevel = topLevel;
   this->populate = populate;
   this->ordered = true;

   #if 0
   if(populate){
      PopulateTopLevelAccelerator(topLevel);
      inter.AssertEmpty(false);
   } else { // Detects bugs if we clear everything for the non populate case
      FOREACH_LIST(ptr,topLevel->allocated){
         ptr->inst->config = nullptr;
         ptr->inst->state = nullptr;
         ptr->inst->delay = nullptr;
         ptr->inst->outputs = nullptr;
         ptr->inst->storedOutputs = nullptr;
         ptr->inst->extraData = nullptr;
      }
   }
   #endif

   stack = PushArray<AcceleratorIterator::Type>(arena,99);
   stack[0].ordered = topLevel->ordered;
   InstanceNode* inst = GetInstance(level);

   return inst;
}

InstanceNode* AcceleratorIterator::GetInstance(int level){
   if(level + upperLevels < 0){
      return nullptr;
   }

   if(ordered){
      return stack[level].ordered->node;
   } else {
      return stack[level].node;
   }
}

InstanceNode* AcceleratorIterator::ParentInstance(){
   InstanceNode* inst = GetInstance(level - 1);
   return inst;
}

String AcceleratorIterator::GetParentInstanceFullName(Arena* out){
   Byte* mark = MarkArena(out);
   for(int i = 0; i < level; i++){
      if(i != 0){
         PushString(out,".");
      }
      PushString(out,"%.*s",UNPACK_SS(GetInstance(i)->inst->name));
   }
   String res = PointArena(out,mark);
   return res;
}

String AcceleratorIterator::GetFullName(Arena* out){
   Byte* mark = MarkArena(out);
   for(int i = 0; i < level + upperLevels + 1; i++){
      if(i != 0){
         PushString(out,".");
      }
      PushString(out,"%.*s",UNPACK_SS(GetInstance(i - upperLevels)->inst->name));
   }
   String res = PointArena(out,mark);
   return res;
}

int AcceleratorIterator::GetFullLevel(){
   int level = this->level + this->upperLevels;
   return level;
}

InstanceNode* AcceleratorIterator::Descend(){
   ComplexFUInstance* inst = GetInstance(level)->inst;

   if(populate){
      FUDeclaration* decl = inst->declaration;
      Accelerator* accel = decl->fixedDelayCircuit;

      FUInstanceInterfaces inter = {};
      inter.Init(topLevel,inst);
      PopulateAccelerator(topLevel,accel,decl,inter,&topLevel->staticUnits);
      #if 1
      inter.AssertEmpty(false);
      #endif
   }

   if(ordered){
      stack[++level].ordered = inst->declaration->fixedDelayCircuit->ordered;
   } else {
      stack[++level].node = inst->declaration->fixedDelayCircuit->allocated;
   }

   return GetInstance(level);
}

InstanceNode* AcceleratorIterator::Current(){
   if(!stack[level].node){
      return nullptr;
   }

   return GetInstance(level);
}

InstanceNode* AcceleratorIterator::CurrentAcceleratorInstance(){
   if(levelBelow && level == 0){
      // Accessing parent iterator stack
      if(ordered){
         return stack[-1].ordered->node;
      } else {
         return stack[-1].node;
      }
   }

   InstanceNode* inst =  GetInstance(level - 1);
   return inst;
}

InstanceNode* AcceleratorIterator::Next(){
   Assert(calledStart);

   ComplexFUInstance* inst = GetInstance(level)->inst;

   if(inst->declaration->fixedDelayCircuit){
      return Descend();
   } else {
      while(1){

         if(ordered){
            stack[level].ordered = stack[level].ordered->next;
         } else {
            stack[level].node = stack[level].node->next;
         }

         if(!stack[level].node){
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

InstanceNode* AcceleratorIterator::Skip(){
   Assert(calledStart);

   if(ordered){
      stack[level].ordered = stack[level].ordered->next;
   } else {
      stack[level].node = stack[level].node->next;
   }

   return Current();
}

AcceleratorIterator AcceleratorIterator::LevelBelowIterator(){
   Descend();

   AcceleratorIterator iter = {};
   iter.stack.data = &this->stack.data[level];
   iter.stack.size = this->stack.size - level;
   iter.topLevel = this->topLevel;
   iter.level = 0;
   iter.upperLevels = this->level + this->upperLevels;
   iter.calledStart = true;
   iter.populate = this->populate;
   iter.levelBelow = true;
   iter.ordered = this->ordered;

   this->level -= 1;

   return iter;
}

#if 1
static void SendLatencyUpwards(InstanceNode* node,Hashmap<EdgeNode,int>* delays){
   int b = node->inputDelay;
   ComplexFUInstance* inst = node->inst;

   FOREACH_LIST(info,node->allOutputs){
      InstanceNode* other = info->instConnectedTo.node;

      // Do not set delay for source and sink units. Source units cannot be found in this, otherwise they wouldn't be source
      Assert(other->type != InstanceNode::TAG_SOURCE);

      int a = inst->declaration->outputLatencies[info->port];
      int e = info->edgeDelay;

      FOREACH_LIST(otherInfo,other->allInputs){
         int c = other->inst->declaration->inputDelays[info->instConnectedTo.port];

         if(info->instConnectedTo.port == otherInfo->port &&
            otherInfo->instConnectedTo.node->inst == inst && otherInfo->instConnectedTo.port == info->port){

            int delay = b + a + e - c;

            #if 0
            EdgeNode edgeNode = {};
            edgeNode.node0 = PortNode{node,info->port};
            edgeNode.node1 = PortNode{other,otherInfo->port};

            delays->Insert(edgeNode,delay);
            #endif

            *otherInfo->delay = delay;
         }
      }
   }
}
#endif

// Instead of an accelerator, it could take a ordered list of instances, and potently the max amount of edges for the hashmap instantiation.
// Therefore abstracting from the accelerator completely and being able to be used for things like subgraphs.
// Change later when needed.
Hashmap<EdgeNode,int>* CalculateDelay(Versat* versat,Accelerator* accel,Arena* out){
   // TODO: We are currently using the delay pointer inside the ConnectionNode structure. When correct, eventually change to just use the hashmap

   int edges = Size(accel->edges);
   Hashmap<EdgeNode,int>* edgeToDelay = PushHashmap<EdgeNode,int>(out,edges);

   // Clear everything, just in case
   FOREACH_LIST(ptr,accel->allocated){
      ptr->inputDelay = 0;

      FOREACH_LIST(con,ptr->allInputs){
         EdgeNode edge = {};

         edge.node0 = con->instConnectedTo;
         edge.node1.node = ptr;
         edge.node1.port = con->port;

         //edgeToDelay->Insert(edge,0);
         con->delay = edgeToDelay->Insert(edge,0);
      }

      FOREACH_LIST(con,ptr->allOutputs){
         EdgeNode edge = {};

         edge.node0.node = ptr;
         edge.node0.port = con->port;
         edge.node1 = con->instConnectedTo;

         //edgeToDelay->Insert(edge,0);
         con->delay = edgeToDelay->Insert(edge,0);
      }
   }

   int graphs = 0;
   OrderedInstance* ptr = accel->ordered;
   for(; ptr; ptr = ptr->next){
      InstanceNode* node = ptr->node;

      if(node->type != InstanceNode::TAG_SOURCE && node->type != InstanceNode::TAG_SOURCE_AND_SINK){
         break;
      }

      node->inputDelay = 0;
      node->inst->baseDelay = 0;

      SendLatencyUpwards(node,edgeToDelay);

      OutputGraphDotFile(versat,accel,true,node->inst,"debug/%.*s/out1_%d.dot",UNPACK_SS(accel->subtype->name),graphs++);
   }

   for(; ptr; ptr = ptr->next){
      InstanceNode* node = ptr->node;
      if(node->type == InstanceNode::TAG_UNCONNECTED){
         continue;
      }

      int maximum = -(1 << 30);
      FOREACH_LIST(info,node->allInputs){
         maximum = std::max(maximum,*info->delay);
      }

      #if 1
      FOREACH_LIST(info,node->allInputs){
         *info->delay = maximum - *info->delay;
      }
      #endif

      node->inputDelay = maximum;
      node->inst->baseDelay = maximum;

      if(node->type != InstanceNode::TAG_SOURCE_AND_SINK){
         SendLatencyUpwards(node,edgeToDelay);
      }

      OutputGraphDotFile(versat,accel,true,node->inst,"debug/%.*s/out2_%d.dot",UNPACK_SS(accel->subtype->name),graphs++);
   }

   #if 1
   FOREACH_LIST(ptr,accel->ordered){
      InstanceNode* node = ptr->node;

      if(node->type != InstanceNode::TAG_SOURCE_AND_SINK){
         continue;
      }

      // Source and sink units never have output delay. They can't
      FOREACH_LIST(con,node->allOutputs){
         *con->delay = 0;
      }
   }
   #endif

   int minimum = 0;
   FOREACH_LIST(ptr,accel->ordered){
      InstanceNode* node = ptr->node;
      minimum = std::min(minimum,node->inputDelay);
   }
   FOREACH_LIST(ptr,accel->ordered){
      InstanceNode* node = ptr->node;
      node->inputDelay = node->inputDelay - minimum;
   }

   OutputGraphDotFile(versat,accel,true,"debug/%.*s/out3.dot",UNPACK_SS(accel->subtype->name));

   #if 1
   FOREACH_LIST(ptr,accel->ordered){
      InstanceNode* node = ptr->node;

      if(node->type != InstanceNode::TAG_SOURCE && node->type != InstanceNode::TAG_SOURCE_AND_SINK){
         break;
      }

      int minimum = 1 << 30;
      FOREACH_LIST(info,node->allOutputs){
         minimum = std::min(minimum,*info->delay);
      }

      // Does not take into account unit latency
      node->inputDelay = minimum;
      node->inst->baseDelay = minimum;

      FOREACH_LIST(info,node->allOutputs){
         *info->delay -= minimum;
      }
   }
   #endif

   OutputGraphDotFile(versat,accel,true,"debug/%.*s/out4.dot",UNPACK_SS(accel->subtype->name));

   FOREACH_LIST(ptr,accel->ordered){
      InstanceNode* node = ptr->node;

      if(node->type == InstanceNode::TAG_UNCONNECTED){
         node->inputDelay = 0;
      } else if(node->type == InstanceNode::TAG_SOURCE_AND_SINK){
      }
      node->inst->baseDelay = node->inputDelay;
   }

   return edgeToDelay;
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

   ComplexFUInstance* inst = node->inst;

   if(node->type == InstanceNode::TAG_SINK ||
     (node->type == InstanceNode::TAG_SOURCE_AND_SINK && CHECK_DELAY(inst,DelayType::DELAY_TYPE_SINK_DELAY))){
      return 0;
   }

   *tag = TAG_TEMPORARY_MARK;

   int count = 0;
   if(node->type == InstanceNode::TAG_COMPUTE){
      FOREACH_LIST(ptr,node->allInputs){
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

DAGOrderNodes CalculateDAGOrder(InstanceNode* instances,Arena* arena){
   int size = Size(instances);

   DAGOrderNodes res = {};

   res.numberComputeUnits = 0;
   res.numberSinks = 0;
   res.numberSources = 0;
   res.size = 0;
   res.instances = PushArray<InstanceNode*>(arena,size).data;
   res.order = PushArray<int>(arena,size).data;

   PushPtr<InstanceNode*> pushPtr = {};
   pushPtr.Init(res.instances,size);

   BLOCK_REGION(arena);

   Hashmap<InstanceNode*,int>* tags = PushHashmap<InstanceNode*,int>(arena,size);

   FOREACH_LIST(ptr,instances){
      res.size += 1;
      tags->Insert(ptr,0);
   }

   res.sources = pushPtr.Push(0);
   // Add source units, guaranteed to come first
   FOREACH_LIST(ptr,instances){
      ComplexFUInstance* inst = ptr->inst;
      if(ptr->type == InstanceNode::TAG_SOURCE || (ptr->type == InstanceNode::TAG_SOURCE_AND_SINK && CHECK_DELAY(inst,DelayType::DELAY_TYPE_SOURCE_DELAY))){
         *pushPtr.Push(1) = ptr;
         res.numberSources += 1;

         tags->Insert(ptr,TAG_PERMANENT_MARK);
      }
   }

   // Add compute units
   res.computeUnits = pushPtr.Push(0);
   FOREACH_LIST(ptr,instances){
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
   FOREACH_LIST(ptr,instances){
      ComplexFUInstance* inst = ptr->inst;
      if(ptr->type == InstanceNode::TAG_SINK || (ptr->type == InstanceNode::TAG_SOURCE_AND_SINK && CHECK_DELAY(inst,DelayType::DELAY_TYPE_SINK_DELAY))){
         *pushPtr.Push(1) = ptr;
         res.numberSinks += 1;

         int* tag = tags->Get(ptr);
         Assert(*tag == 0);

         *tag = TAG_PERMANENT_MARK;
      }
   }

   FOREACH_LIST(ptr,instances){
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
      FOREACH_LIST(ptr,node->allInputs){
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

int CalculateTotalOutputs(Accelerator* accel){
   int total = 0;
   FOREACH_LIST(ptr,accel->allocated){
      ComplexFUInstance* inst = ptr->inst;
      total += inst->declaration->totalOutputs;
   }

   return total;
}

int CalculateTotalOutputs(ComplexFUInstance* inst){
   int total = 0;
   if(inst->declaration->fixedDelayCircuit){
      total += CalculateTotalOutputs(inst->declaration->fixedDelayCircuit);
   }
   total += inst->declaration->outputLatencies.size;

   return total;
}

bool IsUnitCombinatorial(ComplexFUInstance* instance){
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

   FOREACH_LIST(ptr,accel->allocated){
      seen->Insert(ptr,false);
         order->Insert(ptr,size - 1);
   }

   // Start by seeing all inputs
   FOREACH_LIST(ptr,accel->allocated){
      if(ptr->allInputs == nullptr){
         seen->Insert(ptr,true);
         order->Insert(ptr,0);
      }
   }

   FOREACH_LIST(outerPtr,accel->allocated){
      FOREACH_LIST(ptr,accel->allocated){
         bool allInputsSeen = true;
         FOREACH_LIST(input,ptr->allInputs){
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
         FOREACH_LIST(input,ptr->allInputs){
            InstanceNode* node = input->instConnectedTo.node;
            maxOrder = std::max(maxOrder,order->GetOrFail(node));
         }

         order->Insert(ptr,maxOrder + 1);
      }
   }

   Array<InstanceNode*> nodes = PushArray<InstanceNode*>(temp,size);
   int index = 0;
   for(int i = 0; i < size; i++){
      FOREACH_LIST(ptr,accel->allocated){
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

int CalculateNumberOfUnits(InstanceNode* node){
   int res = 0;

   FOREACH_LIST(ptr,node){
      res += 1;

      if(IsTypeHierarchical(ptr->inst->declaration)){
         res += CalculateNumberOfUnits(ptr->inst->declaration->fixedDelayCircuit->allocated);
      }
   }

   return res;
}

#if 0
static int CalculateLatency_(PortInstance portInst, std::unordered_map<PortInstance,int>* memoization,bool seenSourceAndSink){
   ComplexFUInstance* inst = portInst.inst;
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

int CalculateLatency(ComplexFUInstance* inst){
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

void FixMultipleInputs(Versat* versat,Accelerator* accel){
   Arena* arena = &versat->temp;
   ArenaMarker marker(arena);

   static int multiplexersInstantiated = 0;

   bool isComb = true;
   int portUsedCount[99];
   FOREACH_LIST(ptr,accel->allocated){
      ComplexFUInstance* inst = ptr->inst;
      if(inst->declaration == BasicDeclaration::multiplexer || inst->declaration == BasicDeclaration::combMultiplexer){ // Not good, but works for now (otherwise newly added muxes would break the algorithm)
         continue;
      }

      Memset(portUsedCount,0,99);

      if(!IsUnitCombinatorial(inst)){
         isComb = false;
      }

      FOREACH_LIST(con,ptr->allInputs){
         Assert(con->port < 99 && con->port >= 0);
         portUsedCount[con->port] += 1;

         if(!IsUnitCombinatorial(con->instConnectedTo.node->inst)){
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

            String name = PushString(&versat->permanent,format,multiplexersInstantiated++);
            ComplexFUInstance* multiplexer = (ComplexFUInstance*) CreateFUInstance(accel,muxType,name);

            // Connect edges to multiplexer
            int portsConnected = 0;
            FOREACH_LIST(edge,accel->edges){
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
}

void FixMultipleInputs(Versat* versat,Accelerator* accel,Hashmap<ComplexFUInstance*,int>* instanceToInput){
   Arena* arena = &versat->temp;
   ArenaMarker marker(arena);

   Hashmap<PortNode,int>* inputInstances = PushHashmap<PortNode,int>(arena,99);

   static int multiplexersInstantiated = 0;

   bool isComb = true;
   int portUsedCount[99];
   FOREACH_LIST(ptr,accel->allocated){
      ComplexFUInstance* inst = ptr->inst;
      if(inst->declaration == BasicDeclaration::multiplexer || inst->declaration == BasicDeclaration::combMultiplexer){ // Not good, but works for now (otherwise newly added muxes would break the algorithm)
         continue;
      }

      if(!IsUnitCombinatorial(inst)){
         isComb = false;
      }

      Memset(portUsedCount,0,99);
      inputInstances->Clear();

      FOREACH_LIST(con,ptr->allInputs){
         Assert(con->port < 99 && con->port >= 0);
         inputInstances->Insert(con->instConnectedTo,con->port);

         portUsedCount[con->port] += 1;

         if(!IsUnitCombinatorial(con->instConnectedTo.node->inst)){
            isComb = false;
         }
      }

      FUDeclaration* muxType = BasicDeclaration::multiplexer;
      const char* format = "mux%d";

      if(isComb){
         muxType = BasicDeclaration::combMultiplexer;
         format = "comb_mux%d";
      }

      for(int port = 0; port < 99; port++){
         if(portUsedCount[port] > 1){

            String name = PushString(&versat->permanent,format,multiplexersInstantiated++);
            ComplexFUInstance* multiplexer = (ComplexFUInstance*) CreateFUInstance(accel,muxType,name);

            OutputGraphDotFile(versat,accel,true,multiplexer,"debug/%.*s/beforeConnect.dot",UNPACK_SS(accel->subtype->name));

            for(auto& pair : inputInstances){
               if(pair.second != port){
                  continue;
               }

               RemoveConnection(accel,pair.first.node,pair.first.port,ptr,port);
               ConnectUnits(pair.first.node->inst,pair.first.port,multiplexer,instanceToInput->GetOrFail(pair.first.node->inst));
            }

            ConnectUnits(multiplexer,0,inst,port);
            OutputGraphDotFile(versat,accel,true,multiplexer,"debug/%.*s/afterConnect.dot",UNPACK_SS(accel->subtype->name));
         }
      }

      #if 0
         if(portUsedCount[port] > 1){ // Edge has more that one connection
            FUDeclaration* muxType = BasicDeclaration::multiplexer;
            const char* format = "mux%d";

            if(isComb){
               muxType = BasicDeclaration::combMultiplexer;
               format = "comb_mux%d";
            }

            String name = PushString(&versat->permanent,format,multiplexersInstantiated++);
            ComplexFUInstance* multiplexer = (ComplexFUInstance*) CreateFUInstance(accel,muxType,name);

            OutputGraphDotFile(versat,accel,true,multiplexer,"debug/%.*s/beforeConnect.dot",UNPACK_SS(accel->subtype->name));

            // Connect edges to multiplexer


            #if 0
            int portsConnected = 0;
            FOREACH_LIST(edge,accel->edges){
               if(edge->units[1] == PortInstance{inst,port}){
                  RemoveConnection(edge->units[0].inst,edge->units[0].port,edge->units[1].inst,edge->units[1].port);
                  ConnectUnits(edge->units[0].inst,edge->units[0].port,multiplexer,instanceToInput->GetOrFail(edge->units[0].inst));

                  //edge->units[1].inst = multiplexer;
                  //edge->units[1].port = instanceToInput->GetOrFail(edge->units[0].inst); // portsConnected;
                  portsConnected += 1;
               }
            }
            Assert(portsConnected <= 2);
            #endif

            // Connect multiplexer to intended input
            ConnectUnits(multiplexer,0,inst,port);
            OutputGraphDotFile(versat,accel,true,multiplexer,"debug/%.*s/afterConnect.dot",UNPACK_SS(accel->subtype->name));

            // Ports connected can be used to parameterize the multiplexer connection size
         }
      }
      #endif
   }
}

#if 1
void FixDelays(Versat* versat,Accelerator* accel,Hashmap<EdgeNode,int>* edgeDelays){
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

      ComplexFUInstance* buffer = nullptr;
      if(versat->debug.useFixedBuffers){
         String bufferName = PushString(&versat->permanent,"fixedBuffer%d",buffersInserted);

         buffer = (ComplexFUInstance*) CreateFUInstance(accel,BasicDeclaration::fixedBuffer,bufferName);
         buffer->bufferAmount = delay - BasicDeclaration::fixedBuffer->outputLatencies[0];
         buffer->parameters = PushString(&versat->permanent,"#(.AMOUNT(%d))",buffer->bufferAmount);
      } else {
         String bufferName = PushString(&versat->permanent,"buffer%d",buffersInserted);

         buffer = (ComplexFUInstance*) CreateFUInstance(accel,BasicDeclaration::buffer,bufferName);
         buffer->bufferAmount = delay - BasicDeclaration::buffer->outputLatencies[0];
         Assert(buffer->bufferAmount >= 0);
         SetStatic(accel,buffer);

         if(buffer->config){
            buffer->config[0] = buffer->bufferAmount;
         }
      }

      InsertUnit(accel,edge.node0,edge.node1,PortNode{GetInstanceNode(accel,buffer),0});

      OutputGraphDotFile(versat,accel,true,buffer,"debug/%.*s/fixDelay_%d.dot",UNPACK_SS(accel->subtype->name),buffersInserted);
      buffersInserted += 1;
   }
}
#endif

void ActivateMergedAccelerator(Versat* versat,Accelerator* accel,FUDeclaration* type){
   // Accelerator iterator inside another accelerator iterator?

   FUDeclaration* combMux4 = GetTypeByName(versat,STRING("CombMux4"));

   #if 01
   AcceleratorIterator iter = {};

   for(InstanceNode* node = iter.Start(accel,&versat->temp,true); node; node = iter.Skip()){
      ComplexFUInstance* inst = node->inst;
      FUDeclaration* decl = inst->declaration;

      for(int i = 0; i < decl->mergedType.size; i++){
         if(decl->mergedType[i] == type){
            AcceleratorIterator lower = iter.LevelBelowIterator();

            for(InstanceNode* sub = lower.Current(); sub; sub = lower.Next()){
               FUDeclaration* subType = sub->inst->declaration;

               if(BasicDeclaration::multiplexer == subType || BasicDeclaration::combMultiplexer == subType || subType == combMux4){
                  sub->inst->config[0] = i;
               }
            }
         }
      }
   }
   #endif
}

InstanceNode* GetInputNode(InstanceNode* nodes,int inputIndex){
   FOREACH_LIST(ptr,nodes){
      ComplexFUInstance* inst = ptr->inst;
      if(inst->declaration == BasicDeclaration::input && inst->portIndex == inputIndex){
         return ptr;
      }
   }
   return nullptr;
}

ComplexFUInstance* GetInputInstance(InstanceNode* nodes,int inputIndex){
   FOREACH_LIST(ptr,nodes){
      ComplexFUInstance* inst = ptr->inst;
      if(inst->declaration == BasicDeclaration::input && inst->portIndex == inputIndex){
         return inst;
      }
   }
   return nullptr;
}

InstanceNode* GetOutputNode(InstanceNode* nodes){
   FOREACH_LIST(ptr,nodes){
      ComplexFUInstance* inst = ptr->inst;
      if(inst->declaration == BasicDeclaration::output){
         return ptr;
      }
   }

   return nullptr;
}

ComplexFUInstance* GetOutputInstance(InstanceNode* nodes){
   FOREACH_LIST(ptr,nodes){
      ComplexFUInstance* inst = ptr->inst;
      if(inst->declaration == BasicDeclaration::output){
         return inst;
      }
   }

   return nullptr;
}

PortNode GetInputValueInstance(ComplexFUInstance* inst,int index){
   InstanceNode* node = GetInstanceNode(inst->accel,inst);
   if(!node){
      return {};
   }

   PortNode other = node->inputs[index];

   return other;

   #if 0
   PortInstance& other = instance->graphData->singleInputs[index];

   return other.inst;
   #endif
}

ComputedData CalculateVersatComputedData(InstanceNode* instances,VersatComputedValues val,Arena* out){
   Array<ExternalMemoryInterface> external = PushArray<ExternalMemoryInterface>(out,val.externalMemoryInterfaces);
   int index = 0;
   int externalIndex = 0;
   int memoryMapped = 0;
   FOREACH_LIST(ptr,instances){
      for(ExternalMemoryInterface& inter : ptr->inst->declaration->externalMemory){
         external[externalIndex++] = inter;
      }

      if(ptr->inst->declaration->isMemoryMapped){
         memoryMapped += 1;
      }
   }

   Array<VersatComputedData> data = PushArray<VersatComputedData>(out,memoryMapped);

   index = 0;
   FOREACH_LIST(ptr,instances){
      if(ptr->inst->declaration->isMemoryMapped){
         FUDeclaration* decl = ptr->inst->declaration;
         iptr offset = (iptr) ptr->inst->memMapped;
         iptr mask = offset >> decl->memoryMapBits;
         iptr maskSize = val.memoryAddressBits - decl->memoryMapBits;

         //maskSize += 1;

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





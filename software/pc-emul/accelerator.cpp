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

   Reserve(&accel->configAlloc,Kilobyte(1));
   Reserve(&accel->stateAlloc,Kilobyte(1));
   Reserve(&accel->delayAlloc,Kilobyte(1));
   Reserve(&accel->staticAlloc,Kilobyte(1));
   Reserve(&accel->outputAlloc,Megabyte(1));
   Reserve(&accel->storedOutputAlloc,Megabyte(1));
   Reserve(&accel->extraDataAlloc,Megabyte(1));
   Reserve(&accel->externalMemoryAlloc,Megabyte(1));
   Reserve(&accel->debugDataAlloc,Megabyte(1));

   accel->accelMemory = CreateDynamicArena(1); // InitArena(Megabyte(1));
   //accel->temp = InitArena(Megabyte(1));

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
         inst->declaration->initializeFunction(node);
      }

      if(inst->declaration->type == FUDeclaration::COMPOSITE){
         AcceleratorIterator it = iter.LevelBelowIterator();
         InitializeSubaccelerator(it);
      }
   }
}

InstanceNode* CreateFlatFUInstance(Accelerator* accel,FUDeclaration* type,String name){
   Assert(CheckValidName(name));

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

   for(auto pair : type->staticUnits){
      // TODO: Check this, isn't adding static info multiple times? No checking is being done
      StaticInfo* info = accel->staticInfo.Alloc();

      info->id = pair.first;
      info->data = pair.second;
   }

   return ptr;
}

InstanceNode* CreateAndConfigureFUInstance(Accelerator* accel,FUDeclaration* type,String name){
   //SetDebuggingValue(MakeValue(accel,"Accelerator"));

   ArenaMarker marker(&accel->versat->temp);

   InstanceNode* node = CreateFlatFUInstance(accel,type,name);
   ComplexFUInstance* inst = node->inst;
   inst->declarationInstance = inst;

   //if(!flat){ // TODO: before this was true
   //if(false){
   if(true){
      // The position of top level newly created units is always at the end of the current allocated configurations
      int configOffset = accel->configAlloc.size;

      #if 0
      if(isStatic){
         configOffset = accel->staticAlloc.size;
      }
      #endif

      int stateOffset = accel->stateAlloc.size;
      int delayOffset = accel->delayAlloc.size;
      int outputOffset = accel->outputAlloc.size;
      int extraDataOffset = accel->extraDataAlloc.size;
      int externalDataOffset = accel->externalMemoryAlloc.size;
      int debugData = accel->debugDataAlloc.size;

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

      Assert(!ZeroOutRealloc(&accel->configAlloc,val.configs));
      Assert(!ZeroOutRealloc(&accel->stateAlloc,val.states));
      Assert(!ZeroOutRealloc(&accel->delayAlloc,val.delays));
      Assert(!ZeroOutRealloc(&accel->outputAlloc,val.totalOutputs));
      Assert(!ZeroOutRealloc(&accel->storedOutputAlloc,val.totalOutputs));
      Assert(!ZeroOutRealloc(&accel->extraDataAlloc,val.extraData));
      Assert(!ZeroOutRealloc(&accel->staticAlloc,val.statics));
      Assert(!ZeroOutRealloc(&accel->externalMemoryAlloc,val.externalMemorySize));
      Assert(!ZeroOutRealloc(&accel->debugDataAlloc,numberUnits));

      if(type->configs.size){
         #if 0
         if(isStatic){
            inst->config = &accel->staticAlloc.ptr[configOffset];
         } else
         #endif
         {
            inst->config = &accel->configAlloc.ptr[configOffset];
         }
      }

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
         iter.Start(accel,&accel->versat->temp,true);

         //CheckMemory(iter,MemType::EXTRA,&accel->versat->temp);
         //printf("\n");

         for(InstanceNode* node = iter.Start(accel,&accel->versat->temp,true); node; node = iter.Skip()){
            ComplexFUInstance* other = node->inst;
            // Have to iterate the top units until reaching the one we just created
            if(other == inst){
               AcceleratorIterator it = iter.LevelBelowIterator();
               InitializeSubaccelerator(it);
            }
         }
      } else if(type->initializeFunction){
         type->initializeFunction(node);
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
   newInst->isStatic = oldInstance->isStatic;
   newInst->portIndex = oldInstance->portIndex;
   newInst->sharedEnable = oldInstance->sharedEnable;
   newInst->sharedIndex = oldInstance->sharedIndex;

   return newInst;
}

InstanceNode* CopyInstance(Accelerator* newAccel,InstanceNode* oldInstance,String newName){
   InstanceNode* newNode = CreateAndConfigureFUInstance(newAccel,oldInstance->inst->declaration,newName);
   ComplexFUInstance* newInst = newNode->inst;

   newInst->parameters = oldInstance->inst->parameters;
   newInst->baseDelay = oldInstance->inst->baseDelay;
   newInst->isStatic = oldInstance->inst->isStatic;
   newInst->portIndex = oldInstance->inst->portIndex;
   newInst->sharedEnable = oldInstance->inst->sharedEnable;
   newInst->sharedIndex = oldInstance->inst->sharedIndex;

   return newNode;
}

ComplexFUInstance* CopyFlatInstance(Accelerator* newAccel,ComplexFUInstance* oldInstance,String newName){
   ComplexFUInstance* newInst = CreateFlatFUInstance(newAccel,oldInstance->declaration,newName)->inst;

   newInst->parameters = oldInstance->parameters;
   newInst->baseDelay = oldInstance->baseDelay;
   newInst->isStatic = oldInstance->isStatic;
   newInst->portIndex = oldInstance->portIndex;
   newInst->sharedEnable = oldInstance->sharedEnable;
   newInst->sharedIndex = oldInstance->sharedIndex;

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

void FlattenRemoveFUInstance(Accelerator* accel,ComplexFUInstance* inst){
   UNHANDLED_ERROR;
   #if 0
   FOREACH_LIST(edge,accel->edges){
      if(edge->units[0].inst == inst){
         accel->edges = ListRemove(accel->edges,edge);
      } else if(edge->units[1].inst == inst){
         accel->edges = ListRemove(accel->edges,edge);
      }
   }

   #if 01
   // Change allocations for all the other units
   int nConfigs = inst->declaration->configs.size;
   int nStates = inst->declaration->states.size;
   int nDelays = inst->declaration->nDelays;
   int nOutputs = inst->declaration->outputLatencies.size;
   int nTotalOutputs = inst->declaration->totalOutputs;
   int nExtraData = inst->declaration->extraDataSize;

   int* oldConfig = inst->config;
   int* oldState = inst->state;
   int* oldDelay = inst->delay;
   int* oldOutputs = inst->outputs;
   int* oldStoredOutputs = inst->storedOutputs;
   Byte* oldExtraData = inst->extraData;

   if(inst->config) RemoveChunkAndCompress(&accel->configAlloc,inst->config,nConfigs);
   if(inst->state) RemoveChunkAndCompress(&accel->stateAlloc,inst->state,nStates);
   if(inst->delay) RemoveChunkAndCompress(&accel->delayAlloc,inst->delay,nDelays);
   if(inst->outputs) RemoveChunkAndCompress(&accel->outputAlloc,inst->outputs,nOutputs);
   if(inst->storedOutputs) RemoveChunkAndCompress(&accel->storedOutputAlloc,inst->storedOutputs,nOutputs);
   if(inst->extraData) RemoveChunkAndCompress(&accel->extraDataAlloc,inst->extraData,nExtraData);

   if(inst->declaration->type == FUDeclaration::COMPOSITE){
      accel->outputAlloc.size -= (nTotalOutputs - nOutputs); // Total outputs already includes nOutputs, and RemoveChunkAndCompress already removes nOutputs
      accel->storedOutputAlloc.size -= (nTotalOutputs - nOutputs);
      nOutputs = nTotalOutputs;
   }
   #endif

   // Remove instance
   accel->instances = ListRemove(accel->instances,inst);
   accel->numberInstances -= 1;

   #if 01
   // Fix config pointers
   FOREACH_LIST(in,accel->instances){
   //for(ComplexFUInstance* in : accel->instances){
      if(!IsConfigStatic(accel,in) && in->config > oldConfig){
         in->config -= nConfigs;
         Assert(Inside(&accel->configAlloc,in->config));
      }
      if(in->state > oldState){
         in->state -= nStates;
         Assert(Inside(&accel->stateAlloc,in->state));
      }
      if(in->delay > oldDelay){
         in->delay -= nDelays;
         Assert(Inside(&accel->delayAlloc,in->delay));
      }
      #if 1
      if(in->outputs > oldOutputs){
         in->outputs -= nOutputs;
         Assert(Inside(&accel->outputAlloc,in->outputs));
      }
      if(in->storedOutputs > oldStoredOutputs){
         in->storedOutputs -= nOutputs;
         Assert(Inside(&accel->storedOutputAlloc,in->storedOutputs));
      }
      #endif
      if(in->extraData > oldExtraData){
         in->extraData -= nExtraData;
         Assert(Inside(&accel->extraDataAlloc,in->extraData));
      }
   }
   #endif
   #endif
}

Accelerator* Flatten(Versat* versat,Accelerator* accel,int times){
   Arena* arena = &versat->temp;
   BLOCK_REGION(arena);

   InstanceMap map;
   Accelerator* newAccel = CopyFlatAccelerator(versat,accel,&map);
   map.clear();

   // Maybe change to the flatten that does not need to remove units.
   // Then do a simple copy of config values and stuff.

   // Maybe just scrap completely the idea that flatten should preserve configurations.
   // As long as the units have the correct pointers, let the user code have to "reload" the configurations.

   // Ultimately, we need more power in configuration management.

   // Maybe we should have some logic to extract configuration arrays from accelerators and stuff

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

         //*toRemove.Alloc() = *instPtr;

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

   ReorganizeAccelerator(newAccel,arena);

   toRemove.Clear(true);
   compositeInstances.Clear(true);

   #if 0
   //AcceleratorView view = CreateAcceleratorView(newAccel,arena);
   //view.CalculateDelay(arena);
   //FixDelays(versat,newAccel,view);

   UnitValues val1 = CalculateAcceleratorValues(versat,accel);
   UnitValues val2 = CalculateAcceleratorValues(versat,newAccel);

   if(times == 99){
      Assert((val1.configs + val1.statics) == val2.configs); // New accel shouldn't have statics, unless they are from delay units added (which should also be shared configs)
      Assert(val1.states == val2.states);
      Assert(val1.delays == val2.delays);
   }
   #endif

   FUDeclaration base = {};
   base.name = STRING("Top");
   newAccel->subtype = &base;

   Hashmap<EdgeNode,int>* delay = CalculateDelay(versat,newAccel,arena);
   FixDelays(versat,newAccel,delay);

   newAccel->staticInfo.Clear();

   UnitValues val = CalculateAcceleratorValues(versat,newAccel);
   Assert(!ZeroOutRealloc(&newAccel->configAlloc,val.configs));
   Assert(!ZeroOutRealloc(&newAccel->stateAlloc,val.states));
   Assert(!ZeroOutRealloc(&newAccel->delayAlloc,val.delays));
   Assert(!ZeroOutRealloc(&newAccel->outputAlloc,val.totalOutputs));
   Assert(!ZeroOutRealloc(&newAccel->storedOutputAlloc,val.totalOutputs));
   Assert(!ZeroOutRealloc(&newAccel->extraDataAlloc,val.extraData));
   Assert(!ZeroOutRealloc(&newAccel->staticAlloc,val.statics));
   Assert(!ZeroOutRealloc(&newAccel->externalMemoryAlloc,val.externalMemorySize));

   AcceleratorIterator iter = {};
   for(InstanceNode* node = iter.Start(newAccel,arena,true); node; node = iter.Next()){
      ComplexFUInstance* inst = node->inst;
      if(inst->declaration->initializeFunction){
         inst->declaration->initializeFunction(node);
         inst->initialized = true;
      }
   }

   return newAccel;
}

// Returns the instances connected to the output instance.
// The index in the array is the associated port
Array<PortInstance> RecursiveFlattenCreate(Versat* versat,Accelerator* newAccel,Accelerator* accel,Array<PortInstance> inputs,Arena* arena){
   UNHANDLED_ERROR;
   #if 0
   int count = 0;
   FOREACH_LIST(inst,accel->instances){
      count += inst->declaration->outputLatencies.size * 50;
   }

   Array<PortInstance> outputs = PushArray<PortInstance>(arena,BasicDeclaration::output->inputDelays.size);
   // Arena marker here, I think

   // The key is in topLevel space
   // The data is in newAccel space
   Hashmap<PortInstance,PortInstance> mapOutputsToNewInstances = {};
   mapOutputsToNewInstances.Init(arena,count);
   Hashmap<ComplexFUInstance*,ComplexFUInstance*> oldToNew = {};
   oldToNew.Init(arena,accel->numberInstances);

   // In theory, should not work for accelerators with loops
   FOREACH_LIST(inst,accel->instances){
      if(inst->declaration == BasicDeclaration::input){
         int port = inst->portIndex;
         PortInstance mapped = inputs[port];

         mapOutputsToNewInstances.Insert((PortInstance){inst,port},mapped);
      } else if(inst->declaration->type == FUDeclaration::COMPOSITE){
         Array<PortInstance> inputs = PushArray<PortInstance>(arena,inst->graphData->singleInputs.size);
         int i = 0;
         for(PortInstance portInst : inst->graphData->singleInputs){ // Map top level space to new accel space
            if(portInst.inst){
               PortInstance* possible = mapOutputsToNewInstances.Get(portInst);

               if(possible){
                  inputs[i] = *possible;
               }
            }
            i += 1;
         }

         Array<PortInstance> outputs = RecursiveFlattenCreate(versat,newAccel,inst->declaration->fixedDelayCircuit,inputs,arena);
         i = 0;
         for(PortInstance portInst : outputs){
            PortInstance topLevelSpace = (PortInstance){inst,i};

            mapOutputsToNewInstances.Insert(topLevelSpace,portInst);
            i += 1;
         }
      } else if(inst->declaration->type == FUDeclaration::SINGLE){
         ComplexFUInstance* newUnit = (ComplexFUInstance*) CreateFUInstance(newAccel,inst->declaration,inst->name);

         if(inst->isStatic){
            SetStatic(newAccel,newUnit);
         }

         oldToNew.Insert(inst,newUnit);

         int i = 0;
         for(PortInstance portInst : inst->graphData->singleInputs){
            if(portInst.inst){
               PortInstance correct = {};
               if(portInst.inst->declaration == BasicDeclaration::input){
                  int port = portInst.inst->portIndex;
                  correct = inputs[port];
               } else {
                  PortInstance* possible = mapOutputsToNewInstances.Get(portInst);
                  if(possible){
                     correct = *possible;
                  } else {
                     correct.inst = oldToNew.GetOrFail(portInst.inst);
                     correct.port = portInst.port;
                  }
               }

               if(correct.inst){
                  ConnectUnits(correct,(PortInstance){newUnit,i},0);
               }
            }
            i += 1;
         }
      }
   }

   ComplexFUInstance* output = GetOutputInstance(accel);

   int i = 0;
   for(PortInstance portInst : output->graphData->singleInputs){
      if(portInst.inst){
         PortInstance correct = {};
         if(portInst.inst->declaration == BasicDeclaration::input){
            int port = portInst.inst->portIndex;
            correct = inputs[port];
         } else {
            PortInstance* possible = mapOutputsToNewInstances.Get(portInst);
            if(possible){
               correct = *possible;
            } else {
               correct.inst = oldToNew.GetOrFail(portInst.inst);
               correct.port = portInst.port;
            }
         }

         outputs[i] = correct;
      }
      i += 1;
   }

   return outputs;
   #endif
   return {};
}

Accelerator* RecursiveFlatten(Versat* versat,Accelerator* topLevel){
   Arena* arena = &versat->temp;
   ArenaMarker marker(arena);

   UNHANDLED_ERROR;
   #if 0
   Accelerator* newAccel = CreateAccelerator(versat);

   AcceleratorView view = CreateAcceleratorView(topLevel,arena);
   view.CalculateDAGOrdering(arena);

   int count = 0;
   FOREACH_LIST(inst,topLevel->instances){
      count += inst->declaration->outputLatencies.size * 50;
   }

   // The key is in topLevel space
   // The data is in newAccel space
   Hashmap<PortInstance,PortInstance> mapOutputsToNewInstances = {};
   mapOutputsToNewInstances.Init(arena,count);
   Hashmap<ComplexFUInstance*,ComplexFUInstance*> oldToNew = {};
   oldToNew.Init(arena,topLevel->numberInstances);

   // In theory, should not work for accelerators with loops
   for(int index = 0; index < view.nodes.Size(); index++){
      ComplexFUInstance* inst = view.order.instances[index];

      if(inst->declaration->type == FUDeclaration::COMPOSITE){
         Array<PortInstance> inputs = PushArray<PortInstance>(arena,inst->graphData->singleInputs.size);
         int i = 0;
         for(PortInstance portInst : inst->graphData->singleInputs){ // Map top level space to new topLevel space
            if(portInst.inst){
               PortInstance* possible = mapOutputsToNewInstances.Get(portInst);

               if(possible){
                  inputs[i] = *possible;
               } else {
                  inputs[i].inst = oldToNew.GetOrFail(portInst.inst);
                  inputs[i].port = portInst.port;
               }
            }
            i += 1;
         }

         Array<PortInstance> outputs = RecursiveFlattenCreate(versat,newAccel,inst->declaration->fixedDelayCircuit,inputs,arena);
         for(PortInstance portInst : outputs){
            PortInstance topLevelSpace = (PortInstance){inst,i};

            mapOutputsToNewInstances.Insert(topLevelSpace,portInst);
            i += 1;
         }
      } else {
         ComplexFUInstance* newUnit = (ComplexFUInstance*) CreateFUInstance(newAccel,inst->declaration,inst->name);

         if(inst->isStatic){
            SetStatic(newAccel,newUnit);
         }

         oldToNew.Insert(inst,newUnit);

         int i = 0;
         for(PortInstance portInst : inst->graphData->singleInputs){
            if(portInst.inst){
               PortInstance correct = {};
               PortInstance* possible = mapOutputsToNewInstances.Get(portInst);
               if(possible){
                  correct = *possible;
               } else {
                  correct.inst = oldToNew.GetOrFail(portInst.inst);
                  correct.port = portInst.port;
               }

               ConnectUnits(correct,(PortInstance){newUnit,i},0);
            }
            i += 1;
         }
      }
   }

   view = CreateAcceleratorView(newAccel,arena);
   view.CalculateDAGOrdering(arena);
   OutputGraphDotFile(versat,view,true,nullptr,"debug/flatten.dot");

   #if 0
   Assert(topLevel->configAlloc.size == newAccel->configAlloc.size);
   Assert(topLevel->stateAlloc.size == newAccel->stateAlloc.size);
   Assert(topLevel->delayAlloc.size == newAccel->delayAlloc.size);
   Assert(topLevel->extraDataAlloc.size == newAccel->extraDataAlloc.size);
   #endif

   return newAccel;
   #endif
   return nullptr;
}

InstanceNode* AcceleratorIterator::Start(Accelerator* topLevel,ComplexFUInstance* compositeInst,Arena* arena,bool populate){
   this->topLevel = topLevel;
   this->level = 0;
   this->calledStart = true;
   this->populate = populate;
   FUDeclaration* decl = compositeInst->declaration;
   Accelerator* accel = compositeInst->declaration->fixedDelayCircuit;

   if(populate){
      #if 0
      staticUnits = PushHashmap<StaticId,StaticData>(arena,topLevel->staticInfo.Size());
      for(StaticInfo* info : topLevel->staticInfo){
         staticUnits->Insert(info->id,info->data);
      }
      #endif

      staticUnits = decl->staticUnits;

      FUInstanceInterfaces inter = {};
      inter.Init(topLevel,compositeInst);

      PopulateAccelerator(topLevel,accel,decl,inter,staticUnits);

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

   if(populate){
      staticUnits = PushHashmap<StaticId,StaticData>(arena,topLevel->staticInfo.Size());
      for(StaticInfo* info : topLevel->staticInfo){
         staticUnits->Insert(info->id,info->data);
      }

      FUInstanceInterfaces inter = {};
      inter.Init(topLevel);

      PopulateAccelerator2(topLevel,nullptr,inter,staticUnits);
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

   if(populate){
      staticUnits = PushHashmap<StaticId,StaticData>(arena,topLevel->staticInfo.Size());
      for(StaticInfo* info : topLevel->staticInfo){
         staticUnits->Insert(info->id,info->data);
      }

      FUInstanceInterfaces inter = {};
      inter.Init(topLevel);

      PopulateAccelerator2(topLevel,nullptr,inter,staticUnits);
      inter.AssertEmpty(false);
   }

   stack = PushArray<AcceleratorIterator::Type>(arena,99);
   stack[0].ordered = topLevel->ordered;
   InstanceNode* inst = GetInstance(level);

   return inst;
}

InstanceNode* AcceleratorIterator::GetInstance(int level){
   if(level + upperLevels < 0){
      return nullptr;
   }

   long signed int long a = 0;

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

InstanceNode* AcceleratorIterator::Descend(){
   ComplexFUInstance* inst = GetInstance(level)->inst;

   if(populate){
      FUDeclaration* decl = inst->declaration;
      Accelerator* accel = decl->fixedDelayCircuit;

      FUInstanceInterfaces inter = {};
      inter.Init(topLevel,inst);
      PopulateAccelerator(topLevel,accel,decl,inter,staticUnits);
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
   #if 0 // Only because of LevelBelowIterator. Ideally should have a flag to signal this situation and let the function access stack[-1] for iterators that we know they have a bigger stack
   if(level == 0){
      return nullptr;
   }
   #endif

   if(levelBelow && level == 0){
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

AcceleratorIterator AcceleratorIterator::LevelBelowIterator(Arena* arena){
   ComplexFUInstance* inst = Current()->inst;

   Assert(inst && inst->declaration->type == FUDeclaration::COMPOSITE);

   AcceleratorIterator iter = {};

   Assert(!ordered); // If needed, implement a Start for ordered that takes an instance
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
   iter.upperLevels = this->level + this->upperLevels;
   iter.calledStart = true;
   iter.populate = this->populate;
   iter.levelBelow = true;
   iter.ordered = this->ordered;

   this->level -= 1;

   return iter;
}

#if 0
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
   Memset<Byte>(graphData,0);

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
      Memset(graphData->singleInputs,(PortInstance){});
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
#endif

#if 0
static void SendLatencyUpwards(ComplexFUInstance* inst){
   int b = inst->graphData->inputDelay;

   for(ConnectionInfo& info : inst->graphData->allOutputs){
      ComplexFUInstance* other = info.instConnectedTo.inst;

      // Do not set delay for source and sink units. Source units cannot be found in this, otherwise they wouldn't be source
      Assert(other->graphData->nodeType != InstanceNode::TAG_SOURCE);

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

#if 0
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

      if(inst->graphData->nodeType == InstanceNode::TAG_UNCONNECTED){
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

      if(inst->graphData->nodeType != InstanceNode::TAG_SOURCE_AND_SINK){
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

      if(inst->graphData->nodeType != InstanceNode::TAG_SOURCE_AND_SINK){
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
      if(inst->graphData->nodeType == InstanceNode::TAG_UNCONNECTED){
         inst->graphData->inputDelay = 0;
      } else if(inst->graphData->nodeType == InstanceNode::TAG_SOURCE_AND_SINK){
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

#if 0
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

      if(inst->graphData->nodeType == InstanceNode::TAG_UNCONNECTED){
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

      if(inst->graphData->nodeType != InstanceNode::TAG_SOURCE_AND_SINK){
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

      if(inst->graphData->nodeType != InstanceNode::TAG_SOURCE_AND_SINK){
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
      if(inst->graphData->nodeType == InstanceNode::TAG_UNCONNECTED){
         inst->graphData->inputDelay = 0;
      } else if(inst->graphData->nodeType == InstanceNode::TAG_SOURCE_AND_SINK){
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

#if 0
static int Visit(ComplexFUInstance*** ordering,ComplexFUInstance* inst){
   if(inst->tag == TAG_PERMANENT_MARK){
      return 0;
   }
   if(inst->tag == TAG_TEMPORARY_MARK){
      Assert(0);
   }

   if(inst->graphData->nodeType == InstanceNode::TAG_SINK ||
     (inst->graphData->nodeType == InstanceNode::TAG_SOURCE_AND_SINK && CHECK_DELAY(inst,DelayType::DELAY_TYPE_SINK_DELAY))){
      return 0;
   }

   inst->tag = TAG_TEMPORARY_MARK;

   int count = 0;
   if(inst->graphData->nodeType == InstanceNode::TAG_COMPUTE){
      for(ConnectionInfo& info : inst->graphData->allInputs){
         count += Visit(ordering,info.instConnectedTo.inst);
      }
   }

   inst->tag = TAG_PERMANENT_MARK;

   if(inst->graphData->nodeType == InstanceNode::TAG_COMPUTE){
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
      if(inst->graphData->nodeType == InstanceNode::TAG_SOURCE || (inst->graphData->nodeType == InstanceNode::TAG_SOURCE_AND_SINK && CHECK_DELAY(inst,DelayType::DELAY_TYPE_SOURCE_DELAY))){
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
      if(inst->graphData->nodeType == InstanceNode::TAG_UNCONNECTED){
         *(computeUnits++) = inst;
         order.numberComputeUnits += 1;
         inst->tag = TAG_PERMANENT_MARK;
      } else if(inst->tag == 0 && inst->graphData->nodeType == InstanceNode::TAG_COMPUTE){
         order.numberComputeUnits += Visit(&computeUnits,inst);
      }
   }

   // Add sink units
   ComplexFUInstance** sinkUnits = computeUnits;
   order.sinks = sinkUnits;

   for(ComplexFUInstance** instPtr : nodes){
      ComplexFUInstance* inst = *instPtr;
      if(inst->graphData->nodeType == InstanceNode::TAG_SINK || (inst->graphData->nodeType == InstanceNode::TAG_SOURCE_AND_SINK && CHECK_DELAY(inst,DelayType::DELAY_TYPE_SINK_DELAY))){
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
#endif

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
   res.instances = PushArray<InstanceNode*>(arena,size).data;
   res.order = PushArray<int>(arena,size).data;

   PushPtr<InstanceNode*> pushPtr = {};
   pushPtr.Init(res.instances,size);

   BLOCK_REGION(arena);

   Hashmap<InstanceNode*,int>* tags = PushHashmap<InstanceNode*,int>(arena,size);

   FOREACH_LIST(ptr,instances){
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

#if 0
void CalculateVersatData(InstanceNode* instances,Arena* arena){

   VersatComputedData* mem = PushArray<VersatComputedData>(arena,nodes.Size()).data;

   FOREACH_LIST(ptr,instances){
      if(inst->declaration->type )
   }

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
            for(InstanceNode* node = iter.Start(subAccel,arena,false); node; node = iter.Next()){
               ComplexFUInstance* inst = node->inst;
               if(inst->declaration->isMemoryMapped){
                  inst->versatData->memoryAddressOffset += memoryAddressOffset;
               }
            };
         }
      }
   }

   blocks.Clear(true);
}
#endif

#if 0
AcceleratorView CreateAcceleratorView(Accelerator* accel,Arena* arena){
   AcceleratorView view = {};

   view.versat = accel->versat;
   view.accel = accel;

   std::unordered_map<ComplexFUInstance*,ComplexFUInstance**> map;
   FOREACH_LIST(ptr,accel->allocated){
      ComplexFUInstance* inst = ptr->inst;
      ComplexFUInstance** space = view.nodes.Alloc();
      map.insert({inst,space});
      *space = inst;
   }

   FOREACH_LIST(edge,accel->edges){
      EdgeView newEdge = {};

      newEdge.edge = edge;
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
         if(iter == map.end()){
            ComplexFUInstance** newNode = view.nodes.Alloc();
            map.insert({node,view.nodes.Size()});
            *newNode = node;
         }
      }
   }

   return view;
}

AcceleratorView SubGraphAroundInstance(Versat* versat,Accelerator* accel,ComplexFUInstance* instance,int layers,Arena* arena){
   AcceleratorView view = {};

   // Allocate edges first
   FOREACH_LIST(edge,accel->edges){
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
         if(iter == map.end()){
            ComplexFUInstance** newNode = view.nodes.Alloc();
            map.insert({node,view.nodes.Size()});
            *newNode = node;
         }
      }
   }

   return view;
}
#endif

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
   #if 0
   AcceleratorView view = CreateAcceleratorView(accel,temp);
   view.CalculateGraphData(temp);
   view.CalculateDAGOrdering(temp);

   // Reorganize nodes based on DAG order
   DAGOrder order = view.order;
   int size = view.nodes.Size();

   accel->ordered = nullptr; // TODO: We could technically reuse the entirety of nodes just by putting them at the top of the free list when we implement a free list. Otherwise we are just leaky memory
   OrderedInstance* ordered = nullptr;
   for(int i = 0; i < size; i++){
      ComplexFUInstance* inst = order.instances[i];
      InstanceNode* ptr = GetInstanceNode(accel,inst);

      OrderedInstance* newOrdered = PushStruct<OrderedInstance>(&accel->accelMemory);
      newOrdered->node = ptr;

      if(!accel->ordered){
         accel->ordered = newOrdered;
         ordered = newOrdered;
      } else {
         ordered->next = newOrdered;
         ordered = newOrdered;
      }
   }
   #endif

   #if 01
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
   #endif

   #if 0
   InstanceNode* ptr = PushStruct<InstanceNode>(&accel->accelMemory);
   *ptr = {};
   ptr->inst = order.instances[0];
   accel->ordered = ptr;

   for(int i = 1; i < size; i++){
      InstanceNode* newPtr = PushStruct<InstanceNode>(&accel->accelMemory);
      *newPtr = {};
      newPtr->inst = order.instances[i];
      ptr->next = newPtr;
      ptr = newPtr;
   }

   #endif

   #if 0
   accel->instances = order.instances[0];
   for(int i = 0; i < size - 1; i++){
      order.instances[i]->next = order.instances[i+1];
   }
   order.instances[size-1]->next = nullptr;
   #endif
}

int CalculateNumberOfUnits(InstanceNode* node){
   int res = 0;

   FOREACH_LIST(ptr,node){
      res += 1;

      if(ptr->inst->declaration->type == FUDeclaration::COMPOSITE){
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
            data[index].memoryMask[i] = GET_BIT(mask,maskSize - i - 1) ? '1' : '0';
         }
         index += 1;
      }
   }

   ComputedData res = {};
   res.external = external;
   res.data = data;

   return res;
}





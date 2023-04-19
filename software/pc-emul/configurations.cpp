#include "configurations.hpp"

CalculatedOffsets CalculateConfigOffsetsIgnoringStatics(Accelerator* accel,Arena* out){
   int size = Size(accel->allocated);

   Array<int> array = PushArray<int>(out,size);

   BLOCK_REGION(out);

   Hashmap<int,int>* sharedConfigs = PushHashmap<int,int>(out,size);

   int index = 0;
   int offset = 0;
   FOREACH_LIST(ptr,accel->allocated){
      ComplexFUInstance* inst = ptr->inst;
      Assert(!(inst->sharedEnable && inst->isStatic));

      if(inst->sharedEnable){
         int sharedIndex = inst->sharedIndex;
         GetOrAllocateResult<int> possibleAlreadyShared = sharedConfigs->GetOrAllocate(sharedIndex);

         if(possibleAlreadyShared.alreadyExisted){
            array[index] = *possibleAlreadyShared.data;
            goto loop_end;
         } else {
            *possibleAlreadyShared.data = offset;
         }
      }

      if(inst->isStatic){
         array[index] = 0;
      } else {
         array[index] = offset;
         offset += inst->declaration->configs.size;
      }

loop_end:
      index += 1;
   }

   CalculatedOffsets res = {};
   res.offsets = array;
   res.max = offset;

   return res;
}

int GetConfigurationSize(FUDeclaration* decl,MemType type){
   int size = 0;

   switch(type){
   case MemType::CONFIG:{
      size = decl->configs.size;
   }break;
   case MemType::DELAY:{
      size = decl->nDelays;
   }break;
   case MemType::EXTRA:{
      size = decl->extraDataSize;
   }break;
   case MemType::OUTPUT:{
      size = decl->totalOutputs;
   }break;
   case MemType::STATE:{
      size = decl->states.size;
   }break;
   case MemType::STORED_OUTPUT:{
      size = decl->totalOutputs;
   }break;
   default: NOT_IMPLEMENTED;
   }

   return size;
}

CalculatedOffsets CalculateConfigurationOffset(Accelerator* accel,MemType type,Arena* out){
   if(type == MemType::CONFIG){
      return CalculateConfigOffsetsIgnoringStatics(accel,out);
   }

   Array<int> array = PushArray<int>(out,Size(accel->allocated));

   BLOCK_REGION(out);

   int index = 0;
   int offset = 0;
   FOREACH_LIST(ptr,accel->allocated){
      ComplexFUInstance* inst = ptr->inst;
      array[index] = offset;

      int size = GetConfigurationSize(inst->declaration,type);

      offset += size;
      index += 1;
   }

   CalculatedOffsets res = {};
   res.offsets = array;
   res.max = offset;

   return res;
}

CalculatedOffsets CalculateOutputsOffset(Accelerator* accel,int offset,Arena* out){
   Array<int> array = PushArray<int>(out,Size(accel->allocated));

   BLOCK_REGION(out);

   int index = 0;
   FOREACH_LIST(ptr,accel->allocated){
      ComplexFUInstance* inst = ptr->inst;
      array[index] = offset;

      int size = inst->declaration->totalOutputs;

      offset += size;
      index += 1;
   }

   CalculatedOffsets res = {};
   res.offsets = array;
   res.max = offset;

   return res;
}

// TODO: Do not like this implementation, and should be something more general and not static for this file. Put somewhere in versatPrivate
int NumberUnits(Accelerator* accel,Arena* arena){
   BLOCK_REGION(arena);

   int count = 0;
   AcceleratorIterator iter = {};
   for(InstanceNode* node = iter.Start(accel,arena,true); node; node = iter.Next()){
      count += 1;
   }
   return count;
}

CalculatedOffsets ExtractConfig(Accelerator* accel,Arena* out){
   int count = 0;
   int maxConfig = 0;
   AcceleratorIterator iter = {};
   region(out){
      for(InstanceNode* node = iter.Start(accel,out,true); node; node = iter.Next()){
         ComplexFUInstance* inst = node->inst;
         if(inst->declaration->configs.size && !IsConfigStatic(accel,inst)){
            int config = inst->config - accel->configAlloc.ptr;
            maxConfig = std::max(config,maxConfig);
         }

         count += 1;
      }
   }

   Array<int> offsets = PushArray<int>(out,count);

   int index = 0;
   region(out){
      for(InstanceNode* node = iter.Start(accel,out,true); node; node = iter.Next(),index += 1){
         ComplexFUInstance* inst = node->inst;
         int config = 0;
         if(IsConfigStatic(accel,inst)){
            config = inst->config - accel->staticAlloc.ptr + maxConfig;
         } else {
            config = inst->config - accel->configAlloc.ptr;
         }

         offsets[index] = config;
      }
   }

   CalculatedOffsets res = {};
   res.offsets = offsets;
   res.max = maxConfig;
   return res;
}

CalculatedOffsets ExtractState(Accelerator* accel,Arena* out){
   int count = NumberUnits(accel,out);
   Array<int> offsets = PushArray<int>(out,count);

   AcceleratorIterator iter = {};
   int index = 0;
   int max = 0;
   for(InstanceNode* node = iter.Start(accel,out,true); node; node = iter.Next(),index += 1){
      ComplexFUInstance* inst = node->inst;
      int state = inst->state - accel->stateAlloc.ptr;
      max = std::max(state,max);
      offsets[index] = state;
   }

   CalculatedOffsets res = {};
   res.offsets = offsets;
   res.max = max;
   return res;
}

CalculatedOffsets ExtractDelay(Accelerator* accel,Arena* out){
   int count = NumberUnits(accel,out);
   Array<int> offsets = PushArray<int>(out,count);

   AcceleratorIterator iter = {};
   int index = 0;
   int max = 0;
   for(InstanceNode* node = iter.Start(accel,out,true); node; node = iter.Next(),index += 1){
      ComplexFUInstance* inst = node->inst;
      int delay = inst->delay - accel->delayAlloc.ptr;
      max = std::max(delay,max);
      offsets[index] = delay;
   }

   CalculatedOffsets res = {};
   res.offsets = offsets;
   res.max = max;
   return res;
}

CalculatedOffsets ExtractMem(Accelerator* accel,Arena* out){
   int count = NumberUnits(accel,out);
   Array<int> offsets = PushArray<int>(out,count);

   AcceleratorIterator iter = {};
   int index = 0;
   int max = 0;
   for(InstanceNode* node = iter.Start(accel,out,true); node; node = iter.Next(),index += 1){
      ComplexFUInstance* inst = node->inst;
      int mem = inst->externalMemory - accel->externalMemoryAlloc.ptr;
      max = std::max(mem,max);
      offsets[index] = mem;
   }

   CalculatedOffsets res = {};
   res.offsets = offsets;
   res.max = max;
   return res;
}

CalculatedOffsets ExtractOutputs(Accelerator* accel,Arena* out){
   int count = NumberUnits(accel,out);
   Array<int> offsets = PushArray<int>(out,count);

   AcceleratorIterator iter = {};
   int index = 0;
   int max = 0;
   for(InstanceNode* node = iter.Start(accel,out,true); node; node = iter.Next(),index += 1){
      ComplexFUInstance* inst = node->inst;
      int output = inst->outputs - accel->outputAlloc.ptr;
      int storedOutput = inst->storedOutputs - accel->storedOutputAlloc.ptr;
      Assert(output == storedOutput || inst->outputs == nullptr); // Should be true for the foreseeable future. Only when we stop allocating storedOutputs for operations should this change. (And we might not do that ever)
      max = std::max(output,max);
      offsets[index] = output;
   }

   CalculatedOffsets res = {};
   res.offsets = offsets;
   res.max = max;
   return res;
}

CalculatedOffsets ExtractExtraData(Accelerator* accel,Arena* out){
   int count = NumberUnits(accel,out);
   Array<int> offsets = PushArray<int>(out,count);

   AcceleratorIterator iter = {};
   int index = 0;
   int max = 0;
   for(InstanceNode* node = iter.Start(accel,out,true); node; node = iter.Next(),index += 1){
      ComplexFUInstance* inst = node->inst;
      int extraData = inst->extraData - accel->extraDataAlloc.ptr;
      max = std::max(extraData,max);
      offsets[index] = extraData;
   }

   CalculatedOffsets res = {};
   res.offsets = offsets;
   res.max = max;
   return res;
}

CalculatedOffsets ExtractDebugData(Accelerator* accel,Arena* out){
   int count = NumberUnits(accel,out);
   Array<int> offsets = PushArray<int>(out,count);

   AcceleratorIterator iter = {};
   int index = 0;
   int max = 0;
   for(InstanceNode* node = iter.Start(accel,out,true); node; node = iter.Next(),index += 1){
      ComplexFUInstance* inst = node->inst;
      int debugData = inst->debugData - accel->debugDataAlloc.ptr;
      max = std::max(debugData,max);
      offsets[index] = debugData;
   }

   CalculatedOffsets res = {};
   res.offsets = offsets;
   res.max = max;
   return res;
}

bool CheckCorrectConfiguration(Accelerator* topLevel,ComplexFUInstance* inst){
   if(IsConfigStatic(topLevel,inst)){
      Assert(inst->config == nullptr || Inside(&topLevel->staticAlloc,inst->config));
   } else {
      Assert(inst->config == nullptr || Inside(&topLevel->configAlloc,inst->config));
   }

   Assert(inst->state == nullptr || Inside(&topLevel->stateAlloc,inst->state));
   Assert(inst->delay == nullptr || Inside(&topLevel->delayAlloc,inst->delay));
   Assert(inst->extraData == nullptr || Inside(&topLevel->extraDataAlloc,inst->extraData));
   Assert(inst->outputs == nullptr || Inside(&topLevel->outputAlloc,inst->outputs));
   Assert(inst->storedOutputs == nullptr || Inside(&topLevel->storedOutputAlloc,inst->storedOutputs));

   return true;
}

void CheckCorrectConfiguration(Accelerator* topLevel,FUInstanceInterfaces& inter,ComplexFUInstance* inst){
   if(IsConfigStatic(topLevel,inst)){
      Assert(inst->config == nullptr || Inside(&inter.statics,inst->config));
   } else {
      Assert(inst->config == nullptr || Inside(&inter.config,inst->config));
   }

   Assert(inst->state == nullptr || Inside(&inter.state,inst->state));
   Assert(inst->delay == nullptr || Inside(&inter.delay,inst->delay));
   Assert(inst->extraData == nullptr || Inside(&inter.extraData,inst->extraData));
   Assert(inst->outputs == nullptr || Inside(&inter.outputs,inst->outputs));
   Assert(inst->storedOutputs == nullptr || Inside(&inter.storedOutputs,inst->storedOutputs));
}

// Populates sub accelerator
void PopulateAccelerator(Accelerator* topLevel,Accelerator* accel,FUDeclaration* topDeclaration,FUInstanceInterfaces& inter,Hashmap<StaticId,StaticData>* staticMap){
   int index = 0;
   FOREACH_LIST(ptr,accel->allocated){
      ComplexFUInstance* inst = ptr->inst;
      FUDeclaration* decl = inst->declaration;

      inst->config = nullptr;
      inst->state = nullptr;
      inst->delay = nullptr;
      inst->memMapped = nullptr;
      inst->outputs = nullptr;
      inst->storedOutputs = nullptr;
      inst->extraData = nullptr;

      #if 0
      int numberUnits = 0;
      if(decl->type == FUDeclaration::COMPOSITE){
         numberUnits = CalculateNumberOfUnits(decl->fixedDelayCircuit->allocated);
      }
      inst->debugData = inter.debugData.Push(numberUnits + 1);
      #endif

      if(inst->isStatic && decl->configs.size){
         StaticId id = {};
         id.parent = topDeclaration;
         id.name = inst->name;

         StaticData* staticInfo = staticMap->Get(id);
         Assert(staticInfo);
         inst->config = inter.statics.Set(staticInfo->offset,decl->configs.size);
      } else if(decl->configs.size){
         inst->config = inter.config.Set(topDeclaration->configOffsets.offsets[index],decl->configs.size);
      }
      if(decl->states.size){
         inst->state = inter.state.Set(topDeclaration->stateOffsets.offsets[index],decl->states.size);
      }
      if(decl->isMemoryMapped){
         inter.mem.timesPushed = AlignBitBoundary(inter.mem.timesPushed,decl->memoryMapBits);
         inst->memMapped = (int*) inter.mem.Push(1 << decl->memoryMapBits);
      }
      if(decl->nDelays){
         inst->delay = inter.delay.Set(topDeclaration->delayOffsets.offsets[index],decl->nDelays);
      }
      if(decl->externalMemory.size){
         for(int i = 0; i < decl->externalMemory.size; i++){
            int* externalMemory = inter.externalMemory.Push(1 << decl->externalMemory[i].bitsize);
            if(i == 0){
               inst->externalMemory = externalMemory;
            }
         }
      }
      if(decl->totalOutputs){
         inst->outputs = inter.outputs.Set(topDeclaration->outputOffsets.offsets[index],decl->totalOutputs);
         inst->storedOutputs = inter.storedOutputs.Set(topDeclaration->outputOffsets.offsets[index],decl->totalOutputs);
      }
      if(decl->extraDataSize){
         inst->extraData = inter.extraData.Set(topDeclaration->extraDataOffsets.offsets[index],decl->extraDataSize);
      }

      #if 0 // Should be a debug flag
      CheckCorrectConfiguration(topLevel,inter,inst);
      CheckCorrectConfiguration(topLevel,inst);
      #endif

      index += 1;
   }
}

// Top level
void FUInstanceInterfaces::Init(Accelerator* accel){
   VersatComputedValues val = ComputeVersatValues(accel->versat,accel);

   config.Init(accel->configAlloc);
   state.Init(accel->stateAlloc);
   delay.Init(accel->delayAlloc);
   mem.Init(nullptr,val.memoryMappedBytes);
   outputs.Init(accel->outputAlloc);
   storedOutputs.Init(accel->storedOutputAlloc);
   extraData.Init(accel->extraDataAlloc);
   statics.Init(accel->staticAlloc);
   externalMemory.Init(accel->externalMemoryAlloc);
   debugData.Init(accel->debugDataAlloc);
}

int MemorySize(Array<ExternalMemoryInterface> interfaces){
   int size = 0;

   for(ExternalMemoryInterface& inter : interfaces){
      size = AlignBitBoundary(size,inter.bitsize);
      size += 1 << inter.bitsize;
   }

   return size;
}

// Sub instance
void FUInstanceInterfaces::Init(Accelerator* topLevel,ComplexFUInstance* inst){
   FUDeclaration* decl = inst->declaration;
   //int numberUnits = CalculateNumberOfUnits(decl->fixedDelayCircuit->allocated);

   config.Init(inst->config,decl->configOffsets.max);
   state.Init(inst->state,decl->stateOffsets.max);
   delay.Init(inst->delay,decl->delayOffsets.max);
   mem.Init((Byte*) inst->memMapped,1 << decl->memoryMapBits);
   outputs.Init(inst->outputs,decl->totalOutputs);
   storedOutputs.Init(inst->storedOutputs,decl->totalOutputs);
   extraData.Init(inst->extraData,decl->extraDataOffsets.max);
   statics.Init(topLevel->staticAlloc);
   if(decl->externalMemory.size){
      externalMemory.Init(inst->externalMemory,MemorySize(decl->externalMemory));
   }
   #if 0
   debugData.Init(inst->debugData + 1,numberUnits);
   #endif
}

void FUInstanceInterfaces::AssertEmpty(bool checkStatic){
   #if 0
   Assert(config.Empty());
   Assert(state.Empty());
   Assert(delay.Empty());
   Assert(outputs.Empty());
   Assert(storedOutputs.Empty());
   Assert(extraData.Empty());
   if(checkStatic){
      Assert(statics.Empty());
   }
   #endif
}

// The true "Accelerator" populator
void PopulateAccelerator2(Accelerator* accel,FUDeclaration* topDeclaration,FUInstanceInterfaces& inter,Hashmap<StaticId,StaticData>* staticMap){
   STACK_ARENA(tempInst,Kilobyte(64));
   Arena* temp = &tempInst;

   int sharedUnits = 0;
   FOREACH_LIST(ptr,accel->allocated){
      if(ptr->inst->sharedEnable){
         sharedUnits += 1;
      }
   }

   Hashmap<int,int*>* sharedToConfigPtr = PushHashmap<int,int*>(temp,sharedUnits);

   FOREACH_LIST(ptr,accel->allocated){
      ComplexFUInstance* inst = ptr->inst;
      FUDeclaration* decl = inst->declaration;

      inst->config = nullptr;
      inst->state = nullptr;
      inst->delay = nullptr;
      inst->memMapped = nullptr;
      inst->outputs = nullptr;
      inst->storedOutputs = nullptr;
      inst->extraData = nullptr;

      #if 0
      int numberUnits = 0;
      if(decl->type == FUDeclaration::COMPOSITE){
         numberUnits = CalculateNumberOfUnits(decl->fixedDelayCircuit->allocated);
      }
      inst->debugData = inter.debugData.Push(numberUnits + 1);
      #endif

      if(decl->configs.size){
         if(inst->isStatic){
            StaticId id = {};
            id.parent = topDeclaration;
            id.name = inst->name;

            StaticData* staticInfo = staticMap->Get(id);
            Assert(staticInfo);
            inst->config = &inter.statics.ptr[staticInfo->offset];
         } else if(inst->sharedEnable){
            int** ptr = sharedToConfigPtr->Get(inst->sharedIndex);

            if(ptr){
               inst->config = *ptr;
            } else {
               inst->config = inter.config.Push(decl->configs.size);
               sharedToConfigPtr->Insert(inst->sharedIndex,inst->config);
            }
         } else {
            inst->config = inter.config.Push(decl->configs.size);
         }
      }

      if(decl->states.size){
         inst->state = inter.state.Push(decl->states.size);
      }
      if(decl->isMemoryMapped){
         inter.mem.timesPushed = AlignBitBoundary(inter.mem.timesPushed,decl->memoryMapBits);
         inst->memMapped = (int*) inter.mem.Push(1 << decl->memoryMapBits);
      }
      if(decl->nDelays){
         inst->delay = inter.delay.Push(decl->nDelays);
      }
      if(decl->externalMemory.size){
         for(int i = 0; i < decl->externalMemory.size; i++){
            int* externalMemory = inter.externalMemory.Push(1 << decl->externalMemory[i].bitsize);
            if(i == 0){
               inst->externalMemory = externalMemory;
            }
         }
      }
      #if 1
      if(decl->totalOutputs){
         inst->outputs = inter.outputs.Push(decl->totalOutputs);
         inst->storedOutputs = inter.storedOutputs.Push(decl->totalOutputs);
      }
      #endif
      if(decl->extraDataSize){
         inst->extraData = inter.extraData.Push(decl->extraDataSize);
      }

      CheckCorrectConfiguration(accel,inst);
   }
}

Hashmap<String,int*>* ExtractNamedSingleConfigs(Accelerator* accel,Arena* out){
   int count = 0;
   AcceleratorIterator iter = {};
   region(out){
      for(InstanceNode* node = iter.Start(accel,out,true); node; node = iter.Next()){
         ComplexFUInstance* inst = node->inst;
         FUDeclaration* decl = inst->declaration;
         if(decl->type == FUDeclaration::SINGLE && decl->configs.size){
            count += 1;
         }
      }
   }

   Hashmap<String,int*>* res = PushHashmap<String,int*>(out,count);

   for(InstanceNode* node = iter.Start(accel,out,true); node; node = iter.Next()){
      ComplexFUInstance* inst = node->inst;
      FUDeclaration* decl = inst->declaration;
      if(decl->type == FUDeclaration::SINGLE && decl->configs.size){
         String name = iter.GetFullName(out);
         res->Insert(name,inst->config);
      }
   }

   return res;
}





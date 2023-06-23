#include "configurations.hpp"

#include "debugGUI.hpp"
#include "versat.hpp"
#include "acceleratorStats.hpp"

// Top level
void FUInstanceInterfaces::Init(Accelerator* accel){
   VersatComputedValues val = ComputeVersatValues(accel->versat,accel);
   
   config.Init(accel->configAlloc.ptr,val.nConfigs);
   delay.Init(accel->configAlloc.ptr + accel->startOfDelay,val.nDelays);
   statics.Init(accel->configAlloc.ptr + accel->startOfStatic,val.nStatics);

   state.Init(accel->stateAlloc);
   mem.Init(nullptr,val.memoryMappedBytes);
   outputs.Init(accel->outputAlloc);
   storedOutputs.Init(accel->storedOutputAlloc);
   extraData.Init(accel->extraDataAlloc);
   externalMemory.Init(accel->externalMemoryAlloc);
   //debugData.Init(accel->debugDataAlloc);
}

// Sub instance
void FUInstanceInterfaces::Init(Accelerator* topLevel,FUInstance* inst){
   FUDeclaration* decl = inst->declaration;

   VersatComputedValues val = ComputeVersatValues(topLevel->versat,topLevel);
   statics.Init(topLevel->configAlloc.ptr + topLevel->startOfStatic,val.nStatics);
   
   config.Init(inst->config,decl->configOffsets.max);
   state.Init(inst->state,decl->stateOffsets.max);
   delay.Init(inst->delay,decl->delayOffsets.max);
   mem.Init((Byte*) inst->memMapped,1 << decl->memoryMapBits);
   outputs.Init(inst->outputs,decl->outputOffsets.max);
   storedOutputs.Init(inst->storedOutputs,decl->outputOffsets.max);
   extraData.Init(inst->extraData,decl->extraDataOffsets.max);
   if(decl->externalMemory.size){
      externalMemory.Init(inst->externalMemory,MemorySize(decl->externalMemory));
   }

   #if 0
   debugData.Init(inst->debugData + 1,numberUnits);
   #endif
}

void FUInstanceInterfaces::AssertEmpty(){
   #if 0
   Assert(config.Empty());
   Assert(state.Empty());
   Assert(delay.Empty());
   Assert(outputs.Empty());
   Assert(storedOutputs.Empty());
   Assert(extraData.Empty());
   #endif
}

void AddOffset(CalculatedOffsets* offsets,int amount){
   for(int i = 0; i < offsets->offsets.size; i++){
      offsets->offsets[i] += amount;
   }
   offsets->max += amount;
}

CalculatedOffsets CalculateConfigOffsetsIgnoringStatics(Accelerator* accel,Arena* out){
   int size = Size(accel->allocated);

   Array<int> array = PushArray<int>(out,size);

   BLOCK_REGION(out);

   Hashmap<int,int>* sharedConfigs = PushHashmap<int,int>(out,size);

   int index = 0;
   int offset = 0;
   FOREACH_LIST(ptr,accel->allocated){
      FUInstance* inst = ptr->inst;
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
      size = decl->delayOffsets.max;
   }break;
   case MemType::EXTRA:{
      size = decl->extraDataOffsets.max;
   }break;
   case MemType::OUTPUT:{
      size = decl->outputOffsets.max;
   }break;
   case MemType::STATE:{
      size = decl->states.size;
   }break;
   case MemType::STORED_OUTPUT:{
      size = decl->outputOffsets.max;
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
      FUInstance* inst = ptr->inst;
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
      FUInstance* inst = ptr->inst;
      array[index] = offset;

      int size = inst->declaration->outputOffsets.max;

      offset += size;
      index += 1;
   }

   CalculatedOffsets res = {};
   res.offsets = array;
   res.max = offset;

   return res;
}

CalculatedOffsets ExtractConfig(Accelerator* accel,Arena* out){
   int count = 0;
   int maxConfig = 0;
   AcceleratorIterator iter = {};
   region(out){
      for(InstanceNode* node = iter.Start(accel,out,true); node; node = iter.Next()){
         FUInstance* inst = node->inst;
         if(inst->declaration->configs.size){
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
         FUInstance* inst = node->inst;
         int config = 0;
         if(inst->config == nullptr){
            offsets[index] = -1;
            continue;
         }

         offsets[index] = inst->config - accel->configAlloc.ptr;
      }
   }

   CalculatedOffsets res = {};
   res.offsets = offsets;
   res.max = maxConfig;
   return res;
}

CalculatedOffsets ExtractState(Accelerator* accel,Arena* out){
   int count = NumberUnits(accel);
   Array<int> offsets = PushArray<int>(out,count);

   AcceleratorIterator iter = {};
   int index = 0;
   int max = 0;
   for(InstanceNode* node = iter.Start(accel,out,true); node; node = iter.Next(),index += 1){
      FUInstance* inst = node->inst;

      if(inst->state == nullptr){
         offsets[index] = -1;
         continue;
      }

      int state = inst->state - accel->stateAlloc.ptr;
      Assert(state < accel->stateAlloc.size);
      
      max = std::max(state,max);
      offsets[index] = state;
   }

   CalculatedOffsets res = {};
   res.offsets = offsets;
   res.max = max;
   return res;
}

CalculatedOffsets ExtractDelay(Accelerator* accel,Arena* out){
   int count = NumberUnits(accel);
   Array<int> offsets = PushArray<int>(out,count);

   AcceleratorIterator iter = {};
   int index = 0;
   int max = 0;
   for(InstanceNode* node = iter.Start(accel,out,true); node; node = iter.Next(),index += 1){
      FUInstance* inst = node->inst;

      if(inst->delay == nullptr){
         offsets[index] = -1;
         continue;
      }

      int delay = inst->delay - accel->configAlloc.ptr;
      max = std::max(delay,max);
      offsets[index] = delay;
   }

   CalculatedOffsets res = {};
   res.offsets = offsets;
   res.max = max;
   return res;
}

CalculatedOffsets ExtractMem(Accelerator* accel,Arena* out){
   int count = NumberUnits(accel);
   Array<int> offsets = PushArray<int>(out,count);

   AcceleratorIterator iter = {};
   int index = 0;
   int max = 0;
   for(InstanceNode* node = iter.Start(accel,out,true); node; node = iter.Next(),index += 1){
      FUInstance* inst = node->inst;

      if(inst->externalMemory == nullptr){
         offsets[index] = -1;
         continue;
      }

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
   int count = NumberUnits(accel);
   Array<int> offsets = PushArray<int>(out,count);

   AcceleratorIterator iter = {};
   int index = 0;
   int max = 0;
   for(InstanceNode* node = iter.Start(accel,out,true); node; node = iter.Next(),index += 1){
      FUInstance* inst = node->inst;

      if(inst->outputs == nullptr){
         offsets[index] = -1;
         continue;
      }

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
   int count = NumberUnits(accel);
   Array<int> offsets = PushArray<int>(out,count);

   AcceleratorIterator iter = {};
   int index = 0;
   int max = 0;
   for(InstanceNode* node = iter.Start(accel,out,true); node; node = iter.Next(),index += 1){
      FUInstance* inst = node->inst;

      if(inst->extraData == nullptr){
         offsets[index] = -1;
         continue;
      }

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
   #if 0
   int count = NumberUnits(accel);
   Array<int> offsets = PushArray<int>(out,count);

   AcceleratorIterator iter = {};
   int index = 0;
   int max = 0;
   for(InstanceNode* node = iter.Start(accel,out,true); node; node = iter.Next(),index += 1){
      FUInstance* inst = node->inst;

      if(inst->debugData == nullptr){
         offsets[index] = -1;
         continue;
      }

      int debugData = inst->debugData - accel->debugDataAlloc.ptr;
      max = std::max(debugData,max);
      offsets[index] = debugData;
   }

   CalculatedOffsets res = {};
   res.offsets = offsets;
   res.max = max;
   return res;
   #endif
   return (CalculatedOffsets){};
}

bool CheckCorrectConfiguration(Accelerator* topLevel,FUInstance* inst){
   Assert(inst->config == nullptr || Inside(&topLevel->configAlloc,inst->config));
   Assert(inst->state == nullptr || Inside(&topLevel->stateAlloc,inst->state));
   Assert(inst->extraData == nullptr || Inside(&topLevel->extraDataAlloc,inst->extraData));
   Assert(inst->outputs == nullptr || Inside(&topLevel->outputAlloc,inst->outputs));
   Assert(inst->storedOutputs == nullptr || Inside(&topLevel->storedOutputAlloc,inst->storedOutputs));

   return true;
}

void CheckCorrectConfiguration(Accelerator* topLevel,FUInstanceInterfaces& inter,FUInstance* inst){
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
void PopulateAccelerator(Accelerator* topLevel,Accelerator* accel,FUDeclaration* topDeclaration,FUInstanceInterfaces& inter,std::unordered_map<StaticId,StaticData>* staticMap){
   int index = 0;
   FOREACH_LIST(ptr,accel->allocated){
      FUInstance* inst = ptr->inst;
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
      if(IsTypeHierarchical(decl)){
         numberUnits = CalculateNumberOfUnits(decl->fixedDelayCircuit->allocated);
      }
      inst->debugData = inter.debugData.Push(numberUnits + 1);
      #endif

      if(inst->isStatic && decl->configs.size){
         StaticId id = {};
         id.parent = topDeclaration;
         id.name = inst->name;

         #if 0
         StaticData* staticInfo = staticMap->Get(id);
         #endif

         auto iter = staticMap->find(id);

         Assert(iter != staticMap->end());
         inst->config = &inter.statics.ptr[iter->second.offset];
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
      if(decl->delayOffsets.max){
         inst->delay = inter.delay.Set(topDeclaration->delayOffsets.offsets[index],decl->delayOffsets.max);
      }
      if(decl->externalMemory.size){
         for(int i = 0; i < decl->externalMemory.size; i++){
            int* externalMemory = inter.externalMemory.Push(1 << decl->externalMemory[i].bitsize);
            if(i == 0){
               inst->externalMemory = externalMemory;
            }
         }
      }
      if(decl->outputOffsets.max){
         inst->outputs = inter.outputs.Set(topDeclaration->outputOffsets.offsets[index],decl->outputOffsets.max);
         inst->storedOutputs = inter.storedOutputs.Set(topDeclaration->outputOffsets.offsets[index],decl->outputOffsets.max);
      }
      if(decl->extraDataOffsets.max){
         inst->extraData = inter.extraData.Set(topDeclaration->extraDataOffsets.offsets[index],decl->extraDataOffsets.max);
      }

      #if 0 // Should be a debug flag
      CheckCorrectConfiguration(topLevel,inter,inst);
      CheckCorrectConfiguration(topLevel,inst);
      #endif

      index += 1;
   }
}

bool IsConfigStatic(Accelerator* topLevel,FUInstance* inst){
   int offset = inst->config - topLevel->configAlloc.ptr;
   if(offset >= topLevel->startOfStatic){
      return true;
   }
   return false;
}

// The true "Accelerator" populator
void PopulateTopLevelAccelerator(Accelerator* accel){
   STACK_ARENA(tempInst,Kilobyte(64));
   Arena* temp = &tempInst;

   int sharedUnits = 0;
   FOREACH_LIST(ptr,accel->allocated){
      if(ptr->inst->sharedEnable){
         sharedUnits += 1;
      }
   }

   FUInstanceInterfaces inter = {};
   inter.Init(accel);

   Hashmap<int,int*>* sharedToConfigPtr = PushHashmap<int,int*>(temp,sharedUnits);

   FOREACH_LIST(ptr,accel->allocated){
      FUInstance* inst = ptr->inst;
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
      if(IsTypeHierarchical(decl)){
         numberUnits = CalculateNumberOfUnits(decl->fixedDelayCircuit->allocated);
      }
      inst->debugData = inter.debugData.Push(numberUnits + 1);
      #endif

      if(decl->configs.size){
         if(inst->isStatic){
            StaticId id = {};
            id.name = inst->name;

            auto iter = accel->staticUnits.find(id);

            Assert(iter != accel->staticUnits.end());
            inst->config = &inter.statics.ptr[iter->second.offset];
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
      if(decl->delayOffsets.max){
         inst->delay = inter.delay.Push(decl->delayOffsets.max);
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
      if(decl->outputOffsets.max){
         inst->outputs = inter.outputs.Push(decl->outputOffsets.max);
         inst->storedOutputs = inter.storedOutputs.Push(decl->outputOffsets.max);
      }
      #endif
      if(decl->extraDataOffsets.max){
         inst->extraData = inter.extraData.Push(decl->extraDataOffsets.max);
      }

      CheckCorrectConfiguration(accel,inst);
   }
}

Hashmap<String,SizedConfig>* ExtractNamedSingleConfigs(Accelerator* accel,Arena* out){
   STACK_ARENA(temp,Kilobyte(1));

   int count = 0;
   AcceleratorIterator iter = {};
   region(out){
      for(InstanceNode* node = iter.Start(accel,out,false); node; node = iter.Next()){
         FUInstance* inst = node->inst;
         FUDeclaration* decl = inst->declaration;
         if(decl->type == FUDeclaration::SINGLE){
            count += decl->configs.size;
         }
      }
   }

   Hashmap<String,SizedConfig>* res = PushHashmap<String,SizedConfig>(out,count);

   for(InstanceNode* node = iter.Start(accel,out,false); node; node = iter.Next()){
      FUInstance* inst = node->inst;
      FUDeclaration* decl = inst->declaration;
      if(decl->type == FUDeclaration::SINGLE && decl->configs.size){
         BLOCK_REGION(&temp);
         String name = iter.GetFullName(&temp,"_");

         for(int i = 0; i < decl->configs.size; i++){
            String fullName = PushString(out,"%.*s_%.*s",UNPACK_SS(name),UNPACK_SS(decl->configs[i].name));
            res->Insert(fullName,(SizedConfig){(iptr*)inst->config,inst->declaration->configs.size});
         }
      }
   }

   return res;
}

Hashmap<String,SizedConfig>* ExtractNamedSingleStates(Accelerator* accel,Arena* out){
   STACK_ARENA(temp,Kilobyte(1));

   int count = 0;
   AcceleratorIterator iter = {};
   region(out){
      for(InstanceNode* node = iter.Start(accel,out,false); node; node = iter.Next()){
         FUInstance* inst = node->inst;
         FUDeclaration* decl = inst->declaration;
         if(decl->type == FUDeclaration::SINGLE){
            count += decl->states.size;
         }
      }
   }

   Hashmap<String,SizedConfig>* res = PushHashmap<String,SizedConfig>(out,count);

   for(InstanceNode* node = iter.Start(accel,out,false); node; node = iter.Next()){
      FUInstance* inst = node->inst;
      FUDeclaration* decl = inst->declaration;
      if(decl->type == FUDeclaration::SINGLE && decl->states.size){
         BLOCK_REGION(&temp);
         String name = iter.GetFullName(&temp,"_");

         for(int i = 0; i < decl->states.size; i++){
            String fullName = PushString(out,"%.*s_%.*s",UNPACK_SS(name),UNPACK_SS(decl->states[i].name));
            res->Insert(fullName,(SizedConfig){(iptr*)inst->config,inst->declaration->states.size});
         }
      }
   }

   return res;
}

Hashmap<String,SizedConfig>* ExtractNamedSingleMem(Accelerator* accel,Arena* out){
   int count = 0;
   AcceleratorIterator iter = {};
   region(out){
      for(InstanceNode* node = iter.Start(accel,out,false); node; node = iter.Next()){
         FUInstance* inst = node->inst;
         FUDeclaration* decl = inst->declaration;
         if(decl->type == FUDeclaration::SINGLE){
            count += 1;
         }
      }
   }

   Hashmap<String,SizedConfig>* res = PushHashmap<String,SizedConfig>(out,count);

   for(InstanceNode* node = iter.Start(accel,out,false); node; node = iter.Next()){
      FUInstance* inst = node->inst;
      FUDeclaration* decl = inst->declaration;
      if(decl->type == FUDeclaration::SINGLE && decl->states.size){
         String name = iter.GetFullName(out,"_");
         res->Insert(name,(SizedConfig){(iptr*) inst->memMapped,1 << decl->memoryMapBits});
      }
   }

   return res;
}

void CalculateStaticConfigurationPositions(Versat* versat,Accelerator* accel,Arena* temp){
   BLOCK_REGION(temp);

   VersatComputedValues val = ComputeVersatValues(versat,accel);
   int staticStart = val.nConfigs + val.nDelays;

   accel->calculatedStaticPos = PushHashmap<StaticId,StaticData>(accel->accelMemory,val.nStatics);

   int staticIndex = 0; // staticStart; TODO: For now, test with static info beginning at zero
   AcceleratorIterator iter = {};
   for(InstanceNode* node = iter.Start(accel,temp,false); node; node = iter.Next()){
      FUInstance* inst = node->inst;
      FUDeclaration* decl = inst->declaration; 
      if(inst->isStatic){
         FUDeclaration* parentDecl = iter.ParentInstance()->inst->declaration;
         
         StaticId id = {};
         id.name = inst->name;
         id.parent = parentDecl;

         GetOrAllocateResult res = accel->calculatedStaticPos->GetOrAllocate(id);
         
         if(res.alreadyExisted){
            //Assert(data == *res.data);
         } else {
            StaticData data = {};
            data.offset = staticIndex;
            data.configs = decl->configs;

            *res.data = data;
            staticIndex += decl->configs.size;
         }
      }
   }
   accel->staticSize = staticIndex;
}

void SetDefaultConfiguration(FUInstance* instance){
   // TODO; is this function even being used ?
   // I think whether to save defaults should be at the accelerator level
   FUInstance* inst = (FUInstance*) instance;

   inst->savedConfiguration = true;
}

void ShareInstanceConfig(FUInstance* inst, int shareBlockIndex){
   inst->sharedIndex = shareBlockIndex;
   inst->sharedEnable = true;
}

void SetStatic(Accelerator* accel,FUInstance* inst){
   // After two phase compilation change, we only need to store the change in the instance. 
   #if 0
   int size = inst->declaration->configs.size;
   iptr* oldConfig = inst->config;

   StaticId id = {};
   id.name = inst->name;

   auto iter = accel->staticUnits.find(id);
   if(iter == accel->staticUnits.end()){
      bool repopulate = false;
      if(accel->staticAlloc.size + size > accel->staticAlloc.reserved){
         int oldSize = accel->staticAlloc.size;
         ZeroOutRealloc(&accel->staticAlloc,accel->staticAlloc.size + size);
         accel->staticAlloc.size = oldSize;

         repopulate = true;
      }

      iptr* ptr = Push(&accel->staticAlloc,size);

      StaticData data = {};
      data.offset = ptr - accel->staticAlloc.ptr;

      accel->staticUnits.insert({id,data});

      // Only copy data if new static. No reason, do not know if always copying it would be better or not
      Memcpy(ptr,inst->config,size);
      inst->config = ptr;

      if(repopulate){
         Arena* arena = &accel->versat->temp;
         BLOCK_REGION(arena);
         AcceleratorIterator iter = {};
         iter.Start(accel,arena,true);
      }
   }

   RemoveChunkAndCompress(&accel->configAlloc,oldConfig,size);
   #endif
   
   inst->isStatic = true;
}

void InitializeAccelerator(Versat* versat,Accelerator* accel,Arena* temp){
   UnitValues val = CalculateAcceleratorValues(versat,accel);
   int numberUnits = CalculateNumberOfUnits(accel->allocated);

   ZeroOutAlloc(&accel->configAlloc,val.configs + val.delays + val.statics);
   ZeroOutAlloc(&accel->stateAlloc,val.states);
   ZeroOutAlloc(&accel->outputAlloc,val.totalOutputs);
   ZeroOutAlloc(&accel->storedOutputAlloc,val.totalOutputs);
   ZeroOutAlloc(&accel->extraDataAlloc,val.extraData);
   ZeroOutAlloc(&accel->externalMemoryAlloc,val.externalMemorySize);

   CalculateStaticConfigurationPositions(versat,accel,temp);
   accel->startOfStatic = val.configs;
   accel->startOfDelay = val.configs + val.statics;
   
   AcceleratorIterator iter = {};
   for(InstanceNode* node = iter.Start(accel,temp,true); node; node = iter.Next()){
      FUInstance* inst = node->inst;
      FUDeclaration* type = inst->declaration;
      if(type->initializeFunction){
         type->initializeFunction(inst);
      }  
   }
}

String ReprStaticConfig(StaticId id,Wire* wire,Arena* out){
   String identifier = PushString(out,"%.*s_%.*s_%.*s",UNPACK_SS(id.parent->name),UNPACK_SS(id.name),UNPACK_SS(wire->name));

   return identifier;
}

OrderedConfigurations ExtractOrderedConfigurationNames(Versat* versat,Accelerator* accel,Arena* out,Arena* temp){
   UnitValues val = CalculateAcceleratorValues(versat,accel);

   OrderedConfigurations res =  {};
   res.configs = PushArray<Wire>(out,val.configs);
   res.statics = PushArray<Wire>(out,val.statics);
   res.delays  = PushArray<Wire>(out,val.delays);
   
   for(int i = 0; i < val.configs; i++){
      AcceleratorIterator iter = {};
      for(InstanceNode* node = iter.Start(accel,temp,true); node; node = iter.Next()){
         FUInstance* inst = node->inst;
         FUDeclaration* decl = inst->declaration;

         if(decl->configs.size && inst->config && !IsConfigStatic(accel,inst)){
            int index = inst->config - accel->configAlloc.ptr;
            if(index != i){
               continue;
            }

            String name = iter.GetFullName(temp,"_");
            
            for(int ii = 0; ii < decl->configs.size; ii++){
               res.configs[index + ii] = decl->configs[ii];
               res.configs[index + ii].name = PushString(out,"%.*s_%.*s",UNPACK_SS(name),UNPACK_SS(decl->configs[ii].name));
            }
         }
      }
   }
   
   for(Pair<StaticId,StaticData>& pair : accel->calculatedStaticPos){
      int offset = pair.second.offset;
      StaticId& id = pair.first;
      StaticData& data = pair.second;
      for(int ii = 0; ii < data.configs.size; ii++){
         String identifier = ReprStaticConfig(id,&data.configs[ii],out);
         res.statics[ii + offset] = data.configs[ii];
         res.statics[ii + offset].name = identifier;
      }
   }
   
   for(int i = 0; i < val.delays; i++){
      res.delays[i].name = PushString(out,"TOP_Delay%d",i);
      res.delays[i].bitsize = 32; // TODO: For now, delay is set at 32 bits
   }

   return res;
}

Array<Wire> OrderedConfigurationsAsArray(OrderedConfigurations ordered,Arena* out){
   int size = ordered.configs.size + ordered.statics.size + ordered.delays.size;
   Array<Wire> res = PushArray<Wire>(out,size);

   int index = 0;
   for(Wire& wire : ordered.configs){
      res[index++] = wire;
   }
   for(Wire& wire : ordered.statics){
      res[index++] = wire;
   }
   for(Wire& wire : ordered.delays){
      res[index++] = wire;
   }

   return res;
}














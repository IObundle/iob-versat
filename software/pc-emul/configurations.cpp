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













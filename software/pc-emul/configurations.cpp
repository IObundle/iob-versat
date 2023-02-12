#include "configurations.hpp"

CalculatedOffsets CalculateConfigOffsetsIgnoringStatics(Accelerator* accel,Arena* out){
   Array<int> array = PushArray<int>(out,accel->numberInstances);

   ARENA_MARKER(out);

   Hashmap<int,int> sharedConfigs = {};
   sharedConfigs.Init(out,accel->numberInstances);

   int index = 0;
   int offset = 0;
   FOREACH_LIST(inst,accel->instances){

      Assert(!(inst->sharedEnable && inst->isStatic));

      if(inst->sharedEnable){
         int sharedIndex = inst->sharedIndex;
         GetOrAllocateResult<int> possibleAlreadyShared = sharedConfigs.GetOrAllocate(sharedIndex);

         if(possibleAlreadyShared.result){
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
   res.size = offset;

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

   Array<int> array = PushArray<int>(out,accel->numberInstances);

   ARENA_MARKER(out);

   int index = 0;
   int offset = 0;
   FOREACH_LIST(inst,accel->instances){
      array[index] = offset;

      int size = GetConfigurationSize(inst->declaration,type);

      offset += size;
      index += 1;
   }

   CalculatedOffsets res = {};
   res.offsets = array;
   res.size = offset;

   return res;
}

CalculatedOffsets CalculateOutputsOffset(Accelerator* accel,int offset,Arena* out){
   Array<int> array = PushArray<int>(out,accel->numberInstances);

   ARENA_MARKER(out);

   int index = 0;
   FOREACH_LIST(inst,accel->instances){
      array[index] = offset;

      int size = inst->declaration->totalOutputs;

      offset += size;
      index += 1;
   }

   CalculatedOffsets res = {};
   res.offsets = array;
   res.size = offset;

   return res;
}

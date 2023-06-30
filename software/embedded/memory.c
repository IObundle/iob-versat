#include "memory.hpp"
#include "printf.h"

Arena InitArena(size_t size){
   Arena res = {};

   res.totalAllocated = size;
   res.mem = (Byte*) calloc(size,sizeof(Byte));

   return res;
}

Byte* MarkArena(Arena* arena){
   return &arena->mem[arena->used];
}

void PopMark(Arena* arena,Byte* mark){
   arena->used = mark - arena->mem;
}

void Free(Arena* arena){
   free(arena->mem);
   *arena = {};
}

Byte* PushBytes(Arena* arena, size_t size){
   Byte* ptr = &arena->mem[arena->used];

   if(arena->used + size > arena->totalAllocated){
      printf("[%s] Used: %zd, Size: %zd, Total: %zd\n",__PRETTY_FUNCTION__,arena->used,size,arena->totalAllocated);
   }

   //memset(ptr,0,size);
   arena->used += size;

   return ptr;
}

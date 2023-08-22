#include "printf.h"

#ifndef __cplusplus
#include <stdlib.h>
#else
#include <cstdlib>
#endif

typedef unsigned char Byte;
typedef struct{
   Byte* mem;
   size_t used;
   size_t totalAllocated;
} Arena;

Arena InitArena(size_t size){
   Arena res = {};

   res.totalAllocated = size;
   res.mem = (Byte*) malloc(size * sizeof(Byte));

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
   arena->mem = 0;
   arena->used = 0;
   arena->totalAllocated = 0;
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

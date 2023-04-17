#include "memory.hpp"

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

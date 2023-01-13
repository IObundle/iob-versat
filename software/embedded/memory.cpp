#include "memory.hpp"

void InitArena(Arena* arena,size_t size){
   arena->used = 0;
   arena->totalAllocated = size;
   arena->mem = (Byte*) calloc(size,sizeof(Byte));
}

Byte* MarkArena(Arena* arena){
   return &arena->mem[arena->used];
}

void PopMark(Arena* arena,Byte* mark){
   arena->used = mark - arena->mem;
}

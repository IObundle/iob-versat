#include "memory.hpp"

#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "stdio.h"
#include "stdlib.h"

int GetPageSize(){
   static int pageSize = 0;

   if(pageSize == 0){
      pageSize = getpagesize();
   }

   return pageSize;
}

static int pagesAllocated = 0;
static int pagesDeallocated = 0;

void* AllocatePages(int pages){
   static int fd = 0;

   if(fd == 0){
      fd = open("/dev/zero", O_RDWR);

      Assert(fd != -1);
   }

   pagesAllocated += pages;
   void* res = mmap(0, pages * GetPageSize(), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, fd, 0);
   Assert(res != MAP_FAILED);

   return res;
}

void DeallocatePages(void* ptr,int pages){
   pagesDeallocated += pages;
   munmap(ptr,pages * GetPageSize());
}

void InitArena(Arena* arena,size_t size){
   arena->used = 0;
   arena->totalAllocated = size;
   arena->mem = (Byte*) calloc(size,sizeof(Byte));
   arena->align = false;
}

Byte* MarkArena(Arena* arena){
   return &arena->mem[arena->used];
}

void PopMark(Arena* arena,Byte* mark){
   arena->used = mark - arena->mem;
}

Byte* PushBytes(Arena* arena, int size){
   Byte* ptr = &arena->mem[arena->used];

   memset(ptr,0,size);
   arena->used += size;

   if(arena->align){
      arena->used = ALIGN_4(arena->used);
   }

   Assert(arena->used < arena->totalAllocated);

   return ptr;
}

SizedString PushFile(Arena* arena,const char* filepath){
   SizedString res = {};
   FILE* file = fopen(filepath,"r");

   if(!file){
      res.size = -1;
      return res;
   }

   long int size = GetFileSize(file);

   Byte* mem = PushBytes(arena,size + 1);
   fread(mem,sizeof(Byte),size,file);
   mem[size] = '\0';

   fclose(file);

   res.size = size;
   res.str = (const char*) mem;

   return res;
}

SizedString PushString(Arena* arena,SizedString ss){
   Byte* mem = PushBytes(arena,ss.size);

   memcpy(mem,ss.str,ss.size);

   SizedString res = {};
   res.str = (const char*) mem;
   res.size = ss.size;

   return res;
}





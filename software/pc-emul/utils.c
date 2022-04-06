#include "utils.h"

#include <unistd.h>
#include <sys/mman.h>

#include "stdio.h"
#include "string.h"
#include "assert.h"

static int GetPageSize(){
   static int pageSize = 0;

   if(pageSize == 0){
      pageSize = getpagesize();
   }

   return pageSize;
}

void* AllocatePages(int pages){
   void* res = mmap(0, pages * GetPageSize(), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

   Assert(res != MAP_FAILED);

   return res;
}

void DeallocatePages(void* ptr,int pages){
   munmap(ptr,pages * GetPageSize());
}

// Misc

int mini(int a1,int a2){
   int res = (a1 < a2 ? a1 : a2);

   return res;
}

int maxi(int a1,int a2){
   int res = (a1 > a2 ? a1 : a2);

   return res;
}

int RoundUpDiv(int dividend,int divisor){
   int div = dividend / divisor;

   if(dividend % divisor == 0){
      return div;
   } else {
      return (div + 1);
   }
}

void FixedStringCpy(char* dest,const char* src,int size){
   int i = 0;
   for(i = 0; i < size - 1 && src[i] != '\0'; i++){
      dest[i] = src[i];
   }
   dest[i] = '\0';
}

void FlushStdout(){
   fflush(stdout);
}

// Pool

#if MARK
typedef struct PoolHeader_t{
   void* nextPage;
   int allocatedUnits;
} PoolHeader;

typedef struct PoolInfo_t{
   int unitsPerFullPage;
   int bitmapSize;
   int unitsPerPage;
   int pageGranuality;
} PoolInfo;

typedef struct PoolPageInfo_t{
   PoolHeader* header;
   unsigned char* bitmap;
} PoolPageInfo;

typedef struct ElementInfo_t{
   char* entity;
   char* bitmap;
   int bitmask;
} ElementInfo;

static PoolInfo GetPoolInfo(Pool* pool){
   PoolInfo info = {};

   Assert(pool->elementSize != 0);

   info.unitsPerFullPage = (GetPageSize() - sizeof(PoolHeader)) / pool->elementSize;
   info.bitmapSize = RoundUpDiv(info.unitsPerFullPage,8);
   info.unitsPerPage = (GetPageSize() - sizeof(PoolHeader) - info.bitmapSize) / pool->elementSize;
   info.pageGranuality = 1;

   return info;
}

static PoolPageInfo GetPoolPageInfo(PoolInfo info,void* pageStart){
   PoolPageInfo pageInfo = {};

   pageInfo.header = (PoolHeader*) (pageStart + info.pageGranuality * GetPageSize() - sizeof(PoolHeader));
   pageInfo.bitmap = ((char*) pageInfo.header) - info.bitmapSize;

   return pageInfo;
}

static void* PoolNextPage(PoolHeader* header){
   if(!header->nextPage){
      header->nextPage = AllocatePages(1);
   }

   return header->nextPage;
}

static ElementInfo PoolGetElement(Pool* pool,int index){
   PoolInfo info = GetPoolInfo(pool);

   void* ptr = pool->mem;
   PoolPageInfo pageInfo = GetPoolPageInfo(info,ptr);
   while(index >= info.unitsPerPage){
      ptr = PoolNextPage(pageInfo.header);
      index -= info.unitsPerPage;
      pageInfo = GetPoolPageInfo(info,ptr);
   }

   int bitmapIndex = index / 8;
   int bit = 7 - (index % 8);

   ElementInfo elem = {};

   elem.entity = ((char*) ptr) + pool->elementSize * index;
   elem.bitmap = &pageInfo.bitmap[bitmapIndex];
   elem.bitmask = (1 << bit);

   return elem;
}

void* Alloc_(Pool* pool,int size){
   if(pool->elementSize == 0){
      pool->elementSize = size;
   }

   Assert(size + sizeof(PoolHeader) + 1 < GetPageSize());
   Assert(pool->elementSize == size);

   if(!pool->mem){
      pool->mem = AllocatePages(1);
   }

   PoolInfo poolInfo = GetPoolInfo(pool);

   for(int i = 0; 1; i++){
      ElementInfo info = PoolGetElement(pool,i);

       char* page = (char*) ((unsigned int)info.entity & ~(GetPageSize() - 1));
      PoolPageInfo pageInfo = GetPoolPageInfo(poolInfo,page);

      if(!(*info.bitmap & info.bitmask)){
         Assert((long unsigned int) info.entity + pool->elementSize - 1 < (long unsigned int) pageInfo.bitmap)

         pool->allocated += 1;
         *info.bitmap |= info.bitmask;
         return info.entity;
      }
   }
}

void* AllocFixed_(Pool* pool,int index,int size){
   if(pool->elementSize == 0){
      pool->elementSize = size;
   }

   Assert(size + sizeof(PoolHeader) + 1 < GetPageSize());
   Assert(pool->elementSize == size);

   if(!pool->mem){
      pool->mem = AllocatePages(1);
   }

   ElementInfo info = PoolGetElement(pool,index);

   if(*info.bitmap & info.bitmask){
      Assert(0);
   }

   *info.bitmap |= info.bitmask;
   pool->allocated += 1;

   return info.entity;
}

void* PoolGet_(Pool* pool,int index){
   if(!pool->allocated){
      return NULL;
   }

   ElementInfo info = PoolGetElement(pool,index);

   if(*info.bitmap & info.bitmask){
      return info.entity;
   } else {
      return NULL; // Do not return current data, error if not valid
   }
}

// Not very efficient, but in theory, only used rarely (once per ForEach)
void* PoolGetValid_(Pool* pool,int index){
   if(!pool->allocated){
      return NULL;
   }

   void* first = PoolGet_(pool,0);

   if(first && index != 0){
      Assert(1);
   }

   if(first && index == 0){
      return first;
   }

   void* res = PoolGetNextValid(pool,pool->mem);
   for(int i = 0; i < index; i++){
      res = PoolGetNextValid(pool,res);
   }

   return res;
}

void PoolRemoveElement(Pool* pool,void* ptr){
   if(!pool->allocated){
      return;
   }

   PoolInfo info = GetPoolInfo(pool);
   char* page = (char*) ((unsigned int)ptr & ~(GetPageSize() - 1));
   PoolPageInfo pageInfo = GetPoolPageInfo(info,page);

   int pageIndex = ((char*) ptr - page) / pool->elementSize;

   int bitmapIndex = pageIndex / 8;
   int bit = 7 - (pageIndex % 8);

   pageInfo.bitmap[bitmapIndex] &= ~(1 << bit);

   pool->allocated -= 1;
}

void* PoolGetNextValid(Pool* pool,void* ptr){
   if(!pool->allocated){
      return NULL;
   }

   PoolInfo info = GetPoolInfo(pool);
   char* page = (char*) ((unsigned int)ptr & ~(GetPageSize() - 1));
   PoolPageInfo pageInfo = GetPoolPageInfo(info,page);

   int pageIndex = ((char*) ptr - page) / pool->elementSize + 1;

   while(1){
      int bitmapIndex = pageIndex / 8;
      int bit = 7 - (pageIndex % 8);

      if(pageInfo.bitmap[bitmapIndex] & (1 << bit)){
         void* res = ((char*) page) + pool->elementSize * pageIndex;

         return res;
      }
      if(pageIndex >= info.unitsPerPage){
         if(pageInfo.header->nextPage){
            pageIndex = 0;
            page = pageInfo.header->nextPage;
            pageInfo = GetPoolPageInfo(info,page);
            continue;
         } else {
            return NULL;
         }
      } else {
         pageIndex += 1;
      }
   }
}

void PoolClear(Pool* pool){
   if(pool->mem == NULL){
      return;
   }

   PoolInfo info = GetPoolInfo(pool);
   void* pagePtr = pool->mem;
   while(pagePtr){
      PoolPageInfo pageInfo = GetPoolPageInfo(info,pagePtr);
      memset(pageInfo.bitmap,0,sizeof(char) * info.bitmapSize);
      pagePtr = pageInfo.header->nextPage;
   }

   pool->allocated = 0;
}

void PoolFree(Pool* pool){
   if(pool->mem == NULL){
      return;
   }

   PoolInfo info = GetPoolInfo(pool);

   void* pagePtr = pool->mem;
   while(pagePtr){
      PoolPageInfo pageInfo = GetPoolPageInfo(info,pagePtr);

      void* nextPtr = pageInfo.header->nextPage;
      DeallocatePages(pagePtr,1);
      pagePtr = nextPtr;
   }

   *pool = (Pool){};
}
#endif

char* GetHierarchyNameRepr(HierarchyName name){
   static char buffer[MAX_NAME_SIZE * 16];

   int hierarchySize = 0;
   HierarchyName* ptr = &name;
   int index = 0;
   int first = 1;
   while(ptr){
      if(first){
         first = 0;
      } else {
         buffer[index++] = ':';
         Assert(index < MAX_NAME_SIZE * 16);
      }

      hierarchySize += 1;
      for(int i = 0; i < MAX_NAME_SIZE && ptr->str[i] != '\0'; i++){
         buffer[index++] = ptr->str[i];
         Assert(index < MAX_NAME_SIZE * 16);
      }

      ptr = ptr->parent;
   }

   return buffer;
}































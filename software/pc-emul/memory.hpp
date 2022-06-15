#ifndef INCLUDED_MEMORY
#define INCLUDED_MEMORY

#include <cstddef>

#include "utils.hpp"

typedef char Byte;

inline size_t Kilobyte(int val){return val * 1024;};
inline size_t Megabyte(int val){return Kilobyte(val) * 1024;};

int GetPageSize();
void* AllocatePages(int pages);
void DeallocatePages(void* ptr,int pages);

void* ZeroOutRealloc(void* ptr,int newSize,int oldSize); // Realloc but zeroes out extra memory allocated

struct Arena{
   Byte* mem;
   size_t used;
   size_t totalAllocated;
};

Byte* MarkArena(Arena* arena);
void PopMark(Arena* arena,Byte* mark);
Byte* PushBytes(Arena* arena, int size);
SizedString PushFile(Arena* arena,const char* filepath);
SizedString PushString(Arena* arena,SizedString ss);
#define PushStruct(ARENA,STRUCT) (STRUCT*) PushBytes(ARENA,sizeof(STRUCT))
#define PushArray(ARENA,SIZE,STRUCT) (STRUCT*) PushBytes(ARENA,sizeof(STRUCT) * SIZE)

/*
   Pool
*/

struct PoolHeader{
   Byte* nextPage;
   int allocatedUnits;
};

struct PoolInfo{
   int unitsPerFullPage;
   int bitmapSize;
   int unitsPerPage;
   int pageGranuality;
};

struct PageInfo{
   PoolHeader* header;
   Byte* bitmap;
};

template<typename T> class Pool;

template<typename T>
class PoolIterator{
   Pool<T>* pool;
   int fullIndex;
   int bit;
   int index;
   Byte* page;
   T* lastVal;

   friend class Pool<T>;

public:

   PoolIterator(Pool<T>* pool);

   bool operator!=(const PoolIterator& iter);
   PoolIterator& operator++();
   T* operator*();
};

// Pool
template<typename T>
class Pool{
   Byte* mem;
   int allocated;
   int endSize; // Used for end iterator creation

   friend class PoolIterator<T>;

private:

   PoolInfo GetPoolInfo();
   PageInfo GetPageInfo(PoolInfo poolInfo,Byte* page);

public:

   #if 0
   ~Pool();
   #endif

   T* Alloc();
   void Remove(T* elem);

   T* Get(int index);

   void Clear();

   int Size(){return allocated;};

   PoolIterator<T> begin();
   PoolIterator<T> end();

};

// Impl
template<typename T>
PoolIterator<T>::PoolIterator(Pool<T>* pool)
:pool(pool)
,fullIndex(0)
,bit(7)
,index(0)
,lastVal(nullptr)
{
   page = pool->mem;

   if(page && pool->allocated){
      PoolInfo info = pool->GetPoolInfo();
      PageInfo pageInfo = pool->GetPageInfo(info,page);

      if(!(pageInfo.bitmap[index] & (1 << bit))){
         ++(*this);
      }
   }
}

template<typename T>
bool PoolIterator<T>::operator!=(const PoolIterator<T>& iter){
   if(this->pool != iter.pool){
      return true; // Should probably be a error
   }
   if(this->fullIndex < pool->endSize){ // Kinda of a hack, for now
      return true;
   }

   return false;
}

template<typename T>
PoolIterator<T>& PoolIterator<T>::operator++(){
   PoolInfo info = pool->GetPoolInfo();
   PageInfo pageInfo = pool->GetPageInfo(info,page);

   while(fullIndex < pool->endSize){
      fullIndex += 1;
      bit -= 1;
      if(bit < 0){
         index += 1;
         bit = 7;
      }

      if(index * 8 + (7 - bit) >= info.unitsPerPage){
         index = 0;
         bit = 7;
         page = pageInfo.header->nextPage;
         if(page == nullptr){
            break;
         }
         pageInfo = pool->GetPageInfo(info,page);
      }

      if(pageInfo.bitmap[index] & (1 << bit)){
         break;
      }
   }

   return *this;
}

template<typename T>
T* PoolIterator<T>::operator*(){
   if(page == nullptr){
      return nullptr;
   }

   T* view = (T*) page;
   T* val = &view[index * 8 + (7 - bit)];

   PoolInfo info = pool->GetPoolInfo();
   PageInfo pageInfo = pool->GetPageInfo(info,page);

   Assert(pageInfo.bitmap[index] & (1 << bit));

   return val;
}

#if 0
template<typename T>
Pool<T>::~Pool(){
   PoolInfo info = GetPoolInfo();

   byte* ptr = mem;
   while(ptr){
      PageInfo page = GetPageInfo(info,ptr);

      byte* nextPage = page.header->nextPage;

      DeallocatePages(ptr,1);

      ptr = nextPage;
   }
}
#endif

template<typename T>
PoolInfo Pool<T>::GetPoolInfo(){
   PoolInfo info = {};

   info.unitsPerFullPage = (GetPageSize() - sizeof(PoolHeader)) / sizeof(T);
   info.bitmapSize = RoundUpDiv(info.unitsPerFullPage,8);
   info.unitsPerPage = (GetPageSize() - sizeof(PoolHeader) - info.bitmapSize) / sizeof(T);
   info.pageGranuality = 1;

   return info;
}

template<typename T>
PageInfo Pool<T>::GetPageInfo(PoolInfo poolInfo,Byte* page){
   PageInfo info = {};

   info.header = (PoolHeader*) (page + poolInfo.pageGranuality * GetPageSize() - sizeof(PoolHeader));
   info.bitmap = (Byte*) info.header - poolInfo.bitmapSize;

   return info;
}

template<typename T>
T* Pool<T>::Alloc(){
   static int bitmask[] = {0x80,0xc0,0xe0,0xf0,0xf8,0xfc,0xfe,0xff};

   if(!mem){
      mem = (Byte*) AllocatePages(1);
   }

   PoolInfo info = GetPoolInfo();

   int fullIndex = 0;
   Byte* ptr = mem;
   PageInfo page = GetPageInfo(info,ptr);
   while(page.header->allocatedUnits == info.unitsPerPage){
      if(!page.header->nextPage){
         page.header->nextPage = (Byte*) AllocatePages(1);
      }

      ptr = page.header->nextPage;
      page = GetPageInfo(info,ptr);
      fullIndex += info.unitsPerPage;
   }

   T* view = (T*) ptr;
   for(int index = 0; index * 8 < info.unitsPerPage; index += 1){
      char val = page.bitmap[index];

      if(info.unitsPerPage - index * 8 < 8){
         if(val == bitmask[(info.unitsPerPage - 1) % 8]){
            continue;
         }
      } else {
         if(val == 0xff){
            continue;
         }
      }

      for(int i = 7; i >= 0; i--){
         if(!(val & (1 << i))){ // empty location
            page.bitmap[index] |= (1 << i);

            page.header->allocatedUnits += 1;
            allocated += 1;

            fullIndex += index * 8 + (7 - i);

            if(fullIndex + 1 > endSize){
               endSize = fullIndex + 1;
            }

            T* inst = new (&view[index * 8 + (7 - i)]) T(); // Needed for anything stl based

            return inst;
         }
      }
   }

   Assert(0);
   return nullptr;
}

template<typename T>
T* Pool<T>::Get(int index){
   if(!mem){
      return nullptr;
   }

   PoolInfo info = GetPoolInfo();

   Byte* ptr = mem;
   PageInfo page = GetPageInfo(info,ptr);
   while(index >= page.header->allocatedUnits){
      if(!page.header->nextPage){
         return nullptr;
      }

      ptr = page.header->nextPage;
      page = GetPageInfo(info,ptr);
      index -= info.unitsPerPage;
   }

   int bitmapIndex = index / 8;
   int bitIndex = (7 - index % 8);

   if(!(page.bitmap[bitmapIndex] & (1 << bitIndex))){
      return nullptr;
   }

   T* view = (T*) ptr;

   return &view[bitmapIndex * 8 + (7 - bitIndex)];
}

template<typename T>
void Pool<T>::Remove(T* elem){
   PoolInfo info = GetPoolInfo();
   Byte* page = (Byte*) ((unsigned int)elem & ~(GetPageSize() - 1));
   PageInfo pageInfo = GetPageInfo(info,page);

   int pageIndex = ((Byte*) elem - page) / sizeof(T);
   int bitmapIndex = pageIndex / 8;
   int bit = 7 - (pageIndex % 8);

   pageInfo.bitmap[bitmapIndex] &= ~(1 << bit);
   pageInfo.header->allocatedUnits -= 1;

   allocated -= 1;
}

template<typename T>
void Pool<T>::Clear(){
   PoolInfo info = GetPoolInfo();

   Byte* page = mem;
   while(page){
      PageInfo pageInfo = GetPageInfo(info,page);

      memset(pageInfo.bitmap,0,info.bitmapSize);
      pageInfo.header->allocatedUnits = 0;

      page = pageInfo.header->nextPage;
   }

   endSize = 0;
   allocated = 0;
}

template<typename T>
PoolIterator<T> Pool<T>::begin(){
   if(!mem || allocated == 0){
      return end();
   }

   PoolIterator<T> iter(this);

   return iter;
}

template<typename T>
PoolIterator<T> Pool<T>::end(){
   PoolIterator<T> iter(this);

   iter.fullIndex = endSize;

   return iter;
}


#endif // INCLUDED_MEMORY

#ifndef INCLUDED_MEMORY
#define INCLUDED_MEMORY

#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include "utils.hpp"

typedef char Byte;

inline size_t Kilobyte(int val){return val * 1024;};
inline size_t Megabyte(int val){return Kilobyte(val) * 1024;};

int GetPageSize();
void* AllocatePages(int pages);
void DeallocatePages(void* ptr,int pages);
void CheckMemoryStats();

template<typename T>
struct Allocation{
   T* ptr;
   int size;
   int reserved;
};

template<typename T>
bool ZeroOutAlloc(Allocation<T>* alloc,int newSize); // Possible reallocs (returns true if so) but clears all memory allocated to zero

template<typename T>
bool ZeroOutRealloc(Allocation<T>* alloc,int newSize); // Returns true if it actually performed reallocation

template<typename T>
void Free(Allocation<T>* alloc);

struct Arena{
   Byte* mem;
   size_t used;
   size_t totalAllocated;
   bool align;
};

void InitArena(Arena* arena,size_t size);
void Free(Arena* arena);
Byte* MarkArena(Arena* arena);
void PopMark(Arena* arena,Byte* mark);
Byte* PushBytes(Arena* arena, int size);
SizedString PushFile(Arena* arena,const char* filepath);
SizedString PushString(Arena* arena,SizedString ss);
SizedString PushString(Arena* arena,const char* format,...) __attribute__ ((format (printf, 2, 3)));
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

class GenericPoolIterator{
   PoolInfo poolInfo;
   PageInfo pageInfo;
   int fullIndex;
   int bit;
   int index;
   int numberElements;
   int elemSize;
   Byte* page;

public:

   GenericPoolIterator(Byte* page,int numberElements,int elemSize);

   bool HasNext();
   GenericPoolIterator& operator++();
   Byte* operator*();
};

template<typename T> class Pool;

template<typename T>
class PoolIterator{
   Pool<T>* pool;
   PageInfo pageInfo;
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

PoolInfo CalculatePoolInfo(int elemSize);
PageInfo GetPageInfo(PoolInfo poolInfo,Byte* page);

// Pool
template<typename T>
class Pool{
   Byte* mem;
   int allocated;
   int endSize; // Used for end iterator creation
   PoolInfo info;

   friend class PoolIterator<T>;

public:

   Pool();
   ~Pool();

   T* Alloc();
   void Remove(T* elem);

   T* Get(int index);
   Byte* GetMemoryPtr();

   void Clear(bool clearMemory = false);

   int Size(){return allocated;};

   PoolIterator<T> begin();
   PoolIterator<T> end();
};

// Impl
template<typename T>
bool ZeroOutAlloc(Allocation<T>* alloc,int newSize){
   Assert(newSize > 0);

   bool didRealloc = false;
   if(newSize > alloc->reserved){
      T* tmp = (T*) realloc(alloc->ptr,newSize * sizeof(T));

      Assert(tmp);

      alloc->ptr = tmp;
      alloc->reserved = newSize;
      didRealloc = true;
   }

   memset(alloc->ptr,0,alloc->reserved * sizeof(T));
   return didRealloc;
}

template<typename T>
bool ZeroOutRealloc(Allocation<T>* alloc,int newSize){
   Assert(newSize > 0);

   bool didRealloc = false;
   if(newSize > alloc->reserved){
      T* tmp = (T*) realloc(alloc->ptr,newSize * sizeof(T));

      Assert(tmp);

      alloc->ptr = tmp;
      alloc->reserved = newSize;
      didRealloc = true;
   }

   // Clear out free (allocated now or before) space
   if(alloc->reserved - alloc->size > 0){
      char* view = (char*) alloc->ptr;
      memset(&view[alloc->size],0,(alloc->reserved - alloc->size) * sizeof(T));
   }

   return didRealloc;
}

template<typename T>
void Free(Allocation<T>* alloc){
   free(alloc->ptr);
   alloc->ptr = nullptr;
   alloc->reserved = 0;
   alloc->size = 0;
}

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
      pageInfo = GetPageInfo(pool->info,page);

      if(!(pageInfo.bitmap[index] & (1 << bit))){
         ++(*this);
      }
   }
}

template<typename T>
bool PoolIterator<T>::operator!=(const PoolIterator<T>& iter){
   Assert(this->pool == iter.pool)

   if(this->fullIndex < pool->endSize){ // Kinda of a hack, for now
      return true;
   }

   return false;
}

template<typename T>
PoolIterator<T>& PoolIterator<T>::operator++(){
   PoolInfo info = pool->info;

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
         pageInfo = GetPageInfo(info,page);
      }

      if(pageInfo.bitmap[index] & (1 << bit)){
         break;
      }
   }

   return *this;
}

template<typename T>
T* PoolIterator<T>::operator*(){
   Assert(page != nullptr);

   T* view = (T*) page;
   T* val = &view[index * 8 + (7 - bit)];

   Assert(pageInfo.bitmap[index] & (1 << bit));

   return val;
}

template<typename T>
Pool<T>::Pool()
:mem(nullptr)
,allocated(0)
,endSize(0)
{
   info = CalculatePoolInfo(sizeof(T));
}

template<typename T>
Pool<T>::~Pool(){
   Byte* ptr = mem;
   while(ptr){
      PageInfo page = GetPageInfo(info,ptr);

      Byte* nextPage = page.header->nextPage;

      DeallocatePages(ptr,1);

      ptr = nextPage;
   }
}

template<typename T>
T* Pool<T>::Alloc(){
   static int bitmask[] = {0x80,0xc0,0xe0,0xf0,0xf8,0xfc,0xfe,0xff};

   if(!mem){
      mem = (Byte*) AllocatePages(1);
   }

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
Byte* Pool<T>::GetMemoryPtr(){
   return mem;
}

template<typename T>
void Pool<T>::Clear(bool freeMemory){
   if(freeMemory){
      Byte* page = mem;
      while(page){
         PageInfo pageInfo = GetPageInfo(info,page);
         Byte* next = pageInfo.header->nextPage;

         DeallocatePages(page,1);
         page = next;
      }
      mem = nullptr;
   } else {
      Byte* page = mem;
      while(page){
         PageInfo pageInfo = GetPageInfo(info,page);

         memset(pageInfo.bitmap,0,info.bitmapSize);
         pageInfo.header->allocatedUnits = 0;

         page = pageInfo.header->nextPage;
      }
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

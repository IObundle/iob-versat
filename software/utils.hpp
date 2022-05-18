#ifndef INCLUDED_UTILS_HPP
#define INCLUDED_UTILS_HPP

#include <string.h>
#include <new>

#include "assert.h"

#define ALIGN_4(val) ((val + 3) & ~0x3)
#define ARRAY_SIZE(array) sizeof(array) / sizeof(array[0])

#define NUMBER_ARGS_(T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,Arg, ...) Arg
#define NUMBER_ARGS(...) NUMBER_ARGS_(-1,##__VA_ARGS__,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0)

// Misc

typedef unsigned char byte;

int mini(int a1,int a2);
int maxi(int a1,int a2);
int RoundUpDiv(int dividend,int divisor);

int AlignNextPower2(int val);

void FlushStdout();
void* ZeroOutRealloc(void* ptr,int newSize,int oldSize); // Realloc but zeroes out extra memory allocated

#define DebugSignal() printf("%s:%d\n",__FILE__,__LINE__);

#if 1
#define Assert(EXPR) \
   { \
   int _ = (int) (EXPR);   \
   if(!_){ \
      FlushStdout(); \
      assert(_ && EXPR); \
   } \
   }
#else
#define Assert(EXPR) do {} while(0)
#endif

int GetPageSize();
void* AllocatePages(int pages);
void DeallocatePages(void* ptr,int pages);

struct SizedString{
   const char* str;
   size_t size;
};
SizedString MakeSizedString(const char* str, size_t size = 0);
#define MAKE_SIZED_STRING(STR) (MakeSizedString(STR,strlen(STR))) // Should compute length at compile time

struct AllocInfo{
   int size;
   int allocated;
};

void FixedStringCpy(char* dest,SizedString src);

// Compare strings regardless of implementation
bool CompareString(SizedString str1,SizedString str2);
bool CompareString(const char* str1,SizedString str2);
bool CompareString(SizedString str1,const char* str2);
bool CompareString(const char* str1,const char* str2);

#define MAX_NAME_SIZE 64
// Hierarchical naming scheme
struct HierarchyName{
   char str[MAX_NAME_SIZE];
   HierarchyName* parent;
};

char* GetHierarchyNameRepr(HierarchyName name); // Uses statically allocaded memory, take care

struct PoolHeader{
   byte* nextPage;
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
   byte* bitmap;
};

template<typename T> class Pool;

template<typename T>
class PoolIterator{
   Pool<T>* pool;
   int fullIndex;
   int bit;
   int index;
   byte* page;
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
   byte* mem;
   int allocated;
   int endSize; // Used for end iterator creation

   friend class PoolIterator<T>;

private:

   PoolInfo GetPoolInfo();
   PageInfo GetPageInfo(PoolInfo poolInfo,byte* page);

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
PageInfo Pool<T>::GetPageInfo(PoolInfo poolInfo,byte* page){
   PageInfo info = {};

   info.header = (PoolHeader*) (page + poolInfo.pageGranuality * GetPageSize() - sizeof(PoolHeader));
   info.bitmap = (byte*) info.header - poolInfo.bitmapSize;

   return info;
}

template<typename T>
T* Pool<T>::Alloc(){
   static int bitmask[] = {0x80,0xc0,0xe0,0xf0,0xf8,0xfc,0xfe,0xff};

   if(!mem){
      mem = (byte*) AllocatePages(1);
   }

   PoolInfo info = GetPoolInfo();

   int fullIndex = 0;
   byte* ptr = mem;
   PageInfo page = GetPageInfo(info,ptr);
   while(page.header->allocatedUnits == info.unitsPerPage){
      if(!page.header->nextPage){
         page.header->nextPage = (byte*) AllocatePages(1);
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

   byte* ptr = mem;
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
   byte* page = (byte*) ((unsigned int)elem & ~(GetPageSize() - 1));
   PageInfo pageInfo = GetPageInfo(info,page);

   int pageIndex = ((byte*) elem - page) / sizeof(T);
   int bitmapIndex = pageIndex / 8;
   int bit = 7 - (pageIndex % 8);

   pageInfo.bitmap[bitmapIndex] &= ~(1 << bit);
   pageInfo.header->allocatedUnits -= 1;

   allocated -= 1;
}

template<typename T>
void Pool<T>::Clear(){
   PoolInfo info = GetPoolInfo();

   byte* page = mem;
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

#endif // INCLUDED_UTILS_HPP

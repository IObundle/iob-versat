#ifndef INCLUDED_MEMORY
#define INCLUDED_MEMORY

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <stdarg.h>

#include "utilsCore.hpp"
#include "logger.hpp"

inline size_t Kilobyte(int val){return val * 1024;};
inline size_t Megabyte(int val){return Kilobyte(val) * 1024;};
inline size_t Gigabyte(int val){return Megabyte(val) * 1024;};

inline size_t BitSizeToByteSize(int bitSize){return ((bitSize + 7) / 8);};
inline size_t BitSizeToDWordSize(int bitSize){return ((bitSize + 31) / 32);};

int GetPageSize();
void* AllocatePages(int pages);
void DeallocatePages(void* ptr,int pages);
long PagesAvailable();
void CheckMemoryStats();

// Similar to std::vector but reallocations can only be done explicitly. No random reallocations or bugs from their random occurance
template<typename T>
struct Allocation{
   T* ptr;
   int size;
   int reserved;
};

template<typename T> bool ZeroOutAlloc(Allocation<T>* alloc,int newSize); // Clears all memory, returns true if reallocation occurs
template<typename T> bool ZeroOutRealloc(Allocation<T>* alloc,int newSize); // Only clears memory from buffer growth, returns true if reallocation occurs
template<typename T> void RemoveChunkAndCompress(Allocation<T>* alloc,T* ptr,int size);
template<typename T> bool Inside(Allocation<T>* alloc,T* ptr);
template<typename T> void Free(Allocation<T>* alloc);
template<typename T> T* Push(Allocation<T>* alloc, int amount); // Fails if not enough memory

// Care, functions that push to an arena do not clear it to zero or to any value.
struct Arena{
   Byte* mem;
   size_t used;
   size_t totalAllocated;
};

Arena InitArena(size_t size); // Calls calloc
Arena InitLargeArena(); //
Arena SubArena(Arena* arena,size_t size);
void PopToSubArena(Arena* top,Arena subArena);
void Free(Arena* arena);
Byte* MarkArena(Arena* arena);
void PopMark(Arena* arena,Byte* mark);
Byte* PushBytes(Arena* arena, size_t size);
size_t SpaceAvailable(Arena* arena);
String PointArena(Arena* arena,Byte* mark);
String PushFile(Arena* arena,const char* filepath);
String PushString(Arena* arena,String ss);
String PushString(Arena* arena,const char* format,...) __attribute__ ((format (printf, 2, 3)));
String vPushString(Arena* arena,const char* format,va_list args);
void PushNullByte(Arena* arena);

class ArenaMarker{
public:
   Arena* arena;
   Byte* mark;
   ArenaMarker(Arena* arena){this->arena = arena; this->mark = MarkArena(arena);};
   ~ArenaMarker(){PopMark(this->arena,this->mark);};
   void Pop(){PopMark(this->arena,this->mark);};
   operator bool(){return true;}; // For the region trick
};

#define __marker(LINE) marker_ ## LINE
#define _marker(LINE) __marker( LINE )
#define BLOCK_REGION(ARENA) ArenaMarker _marker(__LINE__)(ARENA)

#define region(ARENA) if(ArenaMarker _marker(__LINE__){ARENA})

// Do not abuse stack arenas.
#define STACK_ARENA(NAME,SIZE) \
   Arena NAME = {}; \
   char buffer_##NAME[SIZE]; \
   NAME.mem = (Byte*) buffer_##NAME; \
   NAME.totalAllocated = SIZE;

template<typename T>
Array<T> PushArray(Arena* arena,int size){Array<T> res = {}; res.size = size; res.data = (T*) PushBytes(arena,sizeof(T) * size); return res;};

template<typename T>
Array<T> PointArray(Arena* arena,Byte* mark){String data = PointArena(arena,mark); Array<T> res = {}; res.data = (T*) data.data; res.size = data.size / sizeof(T); return res;}

template<typename T>
T* PushStruct(Arena* arena){T* res = (T*) PushBytes(arena,sizeof(T)); return res;};

// Arena that allocates more blocks of memory like a list.
struct DynamicArena{
   DynamicArena* next;
   Byte* mem;
   size_t used;
   int pagesAllocated;
};

DynamicArena* CreateDynamicArena(int numberPages = 1);
Arena SubArena(DynamicArena* arena,size_t size);
Byte* PushBytes(DynamicArena* arena, size_t size);
void Clear(DynamicArena* arena);

template<typename T>
Array<T> PushArray(DynamicArena* arena,int size){Array<T> res = {}; res.size = size; res.data = (T*) PushBytes(arena,sizeof(T) * size); return res;};

template<typename T>
T* PushStruct(DynamicArena* arena){T* res = (T*) PushBytes(arena,sizeof(T)); return res;};

// A wrapper for a "push" type interface for a block of memory
template<typename T>
class PushPtr{
public:
   T* ptr;
   int maximumTimes;
   int timesPushed;

   PushPtr(){
      ptr = nullptr;
      maximumTimes = 0;
      timesPushed = 0;
   }

   PushPtr(Arena* arena,int maximum){
      this->ptr = PushArray<T>(arena,maximum).data;
      this->maximumTimes = maximum;
      this->timesPushed = 0;
   }

   void Init(T* ptr,int maximum){
      this->ptr = ptr;
      this->maximumTimes = maximum;
      this->timesPushed = 0;
   }

   void Init(Array<T> arr){
      this->ptr = arr.data;
      this->maximumTimes = arr.size;
      this->timesPushed = 0;
   }

   void Init(Allocation<T> alloc){
      this->ptr = alloc.ptr;
      this->maximumTimes = alloc.size;
      this->timesPushed = 0;
   }

   T* Push(int times){
      T* res = &ptr[timesPushed];
      timesPushed += times;

      Assert(timesPushed <= maximumTimes);
      return res;
   }

   void PushValue(T val){
      T* space = Push(1);
      *space = val;
   }

   void Push(Array<T> array){
      T* data = Push(array.size);

      for(int i = 0; i < array.size; i++){
         data[i] = array[i];
      }
   }

   T* Set(int pos,int times){
      T* res = &ptr[pos];

      timesPushed = std::max(timesPushed,pos + times);
      Assert(timesPushed <= maximumTimes);
      return res;
   }

   Array<T> AsArray(){
      Array<T> res = {};
      res.data = ptr;
      res.size = timesPushed;
      return res;
   }

   void Reset(){
      timesPushed = 0;
   }

   bool Empty(){
      bool res = (maximumTimes == timesPushed);
      return res;
   }
};

template<typename T> bool Inside(PushPtr<T>* push,T* ptr);

class BitArray;

class BitIterator{
public:
   BitArray* array;
   int currentByte;
   int currentBit;

public:
   bool operator!=(BitIterator& iter);
   void operator++();
   int operator*(); // Returns index where it is set to one;
};

class BitArray{
public:
   Byte* memory;
   int bitSize;
   int byteSize;

public:
   void Init(Byte* memory,int bitSize);
   void Init(Arena* arena,int bitSize);

   void Fill(bool value);
   void Copy(BitArray array);

   int Get(int index);
   void Set(int index,bool value);

   int GetNumberBitsSet();

   int FirstBitSetIndex();
   int FirstBitSetIndex(int start);

   String PrintRepresentation(Arena* output);

   void operator&=(BitArray& other);

   BitIterator begin();
   BitIterator end();
};

template<typename Key,typename Data>
class HashmapIterator{
public:
   Pair<Key,Data>* pairs;
   int index;

public:
   bool operator!=(HashmapIterator& iter);
   void operator++();
   Pair<Key,Data>& operator*();
};

template<typename Data>
struct GetOrAllocateResult{
   Data* data;
   bool alreadyExisted;
};

// An hashmap implementation for arenas. Does not allocate any memory after construction and also iterates by order of insertion. Construct with PushHashmap function
template<typename Key,typename Data>
struct Hashmap{
   int nodesAllocated;
   int nodesUsed;
   Pair<Key,Data>** buckets;
   Pair<Key,Data>*  data;
   Pair<Key,Data>** next; // Next is separated from data, to allow easy iteration of the data

   // Remaining data is:
   // Pair<Key,Data>* bucketsData[nodesAllocated];
   // Pair<Key,Data>* nextArray[nodesAllocated];
   // Pair<Key,Data> dataData[nodesAllocated];

   // Construct by calling PushHashmap

   Data* Insert(Key key,Data data);
   Data* InsertIfNotExist(Key key,Data data);

   Data* Get(Key key); // TODO: Should return an optional
   Data* GetOrInsert(Key key,Data data);
   Data GetOrFail(Key key);

   void Clear();

   GetOrAllocateResult<Data> GetOrAllocate(Key key); // More efficient way for the Get, check if null, Insert pattern

   bool Exists(Key key);

   //HashmapIterator<Key,Data> begin();
   //HashmapIterator<Key,Data> end();
};

template<typename Key,typename Data>
HashmapIterator<Key,Data> begin(Hashmap<Key,Data>* hashmap);

template<typename Key,typename Data>
HashmapIterator<Key,Data> end(Hashmap<Key,Data>* hashmap);

template<typename Key,typename Data>
Hashmap<Key,Data>* PushHashmap(Arena* arena,int maxAmountOfElements);

template<typename Key,typename Data>
Hashmap<Key,Data>* PushHashmap(DynamicArena* arena,int maxAmountOfElements);

template<typename Key,typename Data>
Array<Key> HashmapKeyArray(Hashmap<Key,Data>* hashmap,Arena* out);

template<typename Key,typename Data>
Array<Data> HashmapDataArray(Hashmap<Key,Data>* hashmap,Arena* out);

// Set implementation for arenas
template<typename Data>
struct Set{
   // TODO: Better impl would not use a map due to wasted space, but its fine for now
   Hashmap<Data,int>* map;

   void Insert(Data data);
   bool Exists(Data data);
};

template<typename Data>
Set<Data>* PushSet(Arena* arena,int maxAmountOfElements);

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
   int elemSize;
   Byte* page;

public:

   void Init(Byte* page,int elemSize);

   bool HasNext();
   void operator++();
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

public:

   void Init(Pool<T>* pool,Byte* page);

   void Advance(); // Does not care if valid or not, always advance one position
   bool IsValid();

   bool HasNext(){return (page != nullptr);};
   bool operator!=(PoolIterator& iter);
   void operator++();
   T* operator*();
};

PoolInfo CalculatePoolInfo(int elemSize);
PageInfo GetPageInfo(PoolInfo poolInfo,Byte* page);

// A vector like class except no reallocations. Useful for storing entities that cannot be reallocated.
template<typename T>
struct Pool{
   Byte* mem; // TODO: replace with PoolHeader instead of using Byte and casting
   PoolInfo info;

   T* Alloc();
   T* Alloc(int index); // Returns nullptr if element already allocated at given position
   void Remove(T* elem);

   T* Get(int index); // Returns nullptr if element not allocated (call Alloc(int index) to allocate)
   T& GetOrFail(int index);
   Byte* GetMemoryPtr();

   int Size();
   void Clear(bool clearMemory = false);
   Pool<T> Copy();

   PoolIterator<T> begin();
   PoolIterator<T> end();

   PoolIterator<T> beginNonValid();

   int MemoryUsage();
};

#include "memory.inl"

#endif // INCLUDED_MEMORY

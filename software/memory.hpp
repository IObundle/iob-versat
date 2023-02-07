#ifndef INCLUDED_MEMORY
#define INCLUDED_MEMORY

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <stdarg.h>

#include "utils.hpp"
#include "logger.hpp"

template<typename T> inline int Hash(T const& t);

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

template<typename T>
struct Allocation{
   T* ptr;
   int size;
   int reserved;
};

template<typename T> bool ZeroOutAlloc(Allocation<T>* alloc,int newSize); // Possible reallocs (returns true if so) but clears all memory allocated to zero
template<typename T> bool ZeroOutRealloc(Allocation<T>* alloc,int newSize); // Returns true if it actually performed reallocation
template<typename T> void Reserve(Allocation<T>* alloc,int reservedSize);
template<typename T> void RemoveChunkAndCompress(Allocation<T>* alloc,T* ptr,int size);
template<typename T> void Alloc(Allocation<T>* alloc,int newSize);
template<typename T> bool Inside(Allocation<T>* alloc,T* ptr);
template<typename T> void Free(Allocation<T>* alloc);
template<typename T> int MemoryUsage(Allocation<T> alloc);

template<typename T>
class PushPtr{
public:
   T* ptr;
   int maximumTimes;
   int timesPushed;

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

   bool Empty(){
      bool res = (maximumTimes == timesPushed);
      return res;
   }
};

struct Arena{
   Byte* mem;
   size_t used;
   size_t totalAllocated;
};

void InitArena(Arena* arena,size_t size); // Calls calloc
void InitLargeArena(Arena* arena,size_t size); //
Arena SubArena(Arena* arena,size_t size);
void PopToSubArena(Arena* top,Arena subArena);
void Free(Arena* arena);
Byte* MarkArena(Arena* arena);
void PopMark(Arena* arena,Byte* mark);
Byte* PushBytes(Arena* arena, int size);
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
};

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

template<typename Key,typename Data>
struct HashmapHeader{
   int nodesAllocated;
   int nodesUsed;
   Pair<Key,Data>** buckets;
   Pair<Key,Data>*  data;
   Pair<Key,Data>** next; // Next is separated from data, to allow easy iteration of the data

   // Remaining data is:
   // Pair<Key,Data>* bucketsData[nodesAllocated];
   // Pair<Key,Data>* nextArray[nodesAllocated];
   // Pair<Key,Data> dataData[nodesAllocated];
};

// An hashmap implementation for arenas. Does not allocate any memory after construction. Need to pass correct amount of maxAmountOfElements
template<typename Key,typename Data>
struct Hashmap{
   HashmapHeader<Key,Data>* header;

public:

   void Init(Arena* arena,int maxAmountOfElements);

   Data* Insert(Key key,Data data);
   Data* InsertIfNotExist(Key key,Data data);

   Data* Get(Key key);
   Data GetOrFail(Key key);

   bool Exists(Key key);

   HashmapIterator<Key,Data> begin();
   HashmapIterator<Key,Data> end();
};

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

   friend class Pool<T>;

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

// Pool
template<typename T>
struct Pool{
   Byte* mem;
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

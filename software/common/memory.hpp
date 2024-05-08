#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <stdarg.h>

#include "utilsCore.hpp"
#include "logger.hpp"

/* 
  TODO: Using fixed size for structures like hashmap and set is a bit
  wasteful.  Would like to implement hashmap and set structures
  that keep a pointer to an arena and can grow by allocating
  more space (implement set using binary tree, for hashmap try
  to find something).

  TODO: Right now we allocate a lot of dynamic arrays using the Push +
  PointArray form. This is error prone, a single allocation
  inside can lead to incorrect results.  Would like to formalize
  this pattern by defining a DynamicArray structure that acts
  the same way as an ArenaList and such.  To make it less error
  prone, add some debug info stored into the arena that records
  whether the arena is currently being used to allocate dynamic
  arrays and throw some error if we detect an allocation that
  shouldn't happen.  Wrap this extra code inside a DEBUG
  directive.
 */

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

#undef VERSAT_DEBUG

// Care, functions that push to an arena do not clear it to zero or to any value.
// Need to initialize the values directly.
struct Arena{
  Byte* mem;
  size_t used;
  size_t totalAllocated;
  size_t maximum;

  // TODO: Locked could be an enum for all the cases, like dynamic array and string. That way we could actually check if we are doing anything bad (Check inside a PushString if we are inside a DynamicArray section, for example).
  bool locked; // Certain constructs [like DynamicArray] "lock" arena preventing the arena from being used by other functions.
}; 

#define VERSAT_DEBUG

void AlignArena(Arena* arena,int alignment);
Arena InitArena(size_t size); // Calls calloc
Arena InitLargeArena(); //
Arena SubArena(Arena* arena,size_t size);
void PopToSubArena(Arena* top,Arena subArena);
void Reset(Arena* arena);
void Free(Arena* arena);
Byte* PushBytes(Arena* arena, size_t size);
size_t SpaceAvailable(Arena* arena);
String PushChar(Arena* arena,const char);
String PushString(Arena* arena,int size);
String PushString(Arena* arena,String ss);
String PushString(Arena* arena,const char* format,...) __attribute__ ((format (printf, 2, 3)));
String vPushString(Arena* arena,const char* format,va_list args);
void PushNullByte(Arena* arena);

struct ArenaMark{
  Arena* arena;
  Byte* mark;
}; 

ArenaMark MarkArena(Arena* arena);
void PopMark(ArenaMark mark);

// TODO: Maybe add Optional to indicate error opening file
//       In general error handling is pretty lacking overall.
String PushFile(Arena* arena,FILE* file);
String PushFile(Arena* arena,const char* filepath);

class ArenaMarker{
public:
  ArenaMark mark;
  
  ArenaMarker(Arena* arena){this->mark = MarkArena(arena);};
  ~ArenaMarker(){PopMark(this->mark);};
  void Pop(){PopMark(this->mark);};
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
Array<T> PushArray(Arena* arena,int size){AlignArena(arena,alignof(T)); Array<T> res = {}; res.size = size; res.data = (T*) PushBytes(arena,sizeof(T) * size); return res;};

template<typename T>
T* PushStruct(Arena* arena){AlignArena(arena,alignof(T)); T* res = (T*) PushBytes(arena,sizeof(T)); return res;};

// Arena that allocates more blocks of memory like a list.
struct DynamicArena{
  DynamicArena* next;
  Byte* mem;
  size_t used;
  int pagesAllocated;
};

void AlignArena(DynamicArena* arena,int alignment);
DynamicArena* CreateDynamicArena(int numberPages = 1);
Arena SubArena(DynamicArena* arena,size_t size);
Byte* PushBytes(DynamicArena* arena, size_t size);
void Clear(DynamicArena* arena);
String PushString(DynamicArena* arena,String str);

template<typename T>
Array<T> PushArray(DynamicArena* arena,int size){AlignArena(arena,alignof(T)); Array<T> res = {}; res.size = size; res.data = (T*) PushBytes(arena,sizeof(T) * size); return res;};

template<typename T>
T* PushStruct(DynamicArena* arena){AlignArena(arena,alignof(T)); T* res = (T*) PushBytes(arena,sizeof(T)); return res;};

template<typename T>
struct DynamicArray{
  ArenaMark mark;
  
  ~DynamicArray();

  Array<T> AsArray();
  T* PushElem();
};

template<typename T>
DynamicArray<T> StartArray(Arena* arena);

template<typename T>
Array<T> EndArray(DynamicArray<T> arr);

struct DynamicString{
  Arena* arena;
  Byte* mark;
};

DynamicString StartString(Arena *arena);
String EndString(DynamicString mark);

// A wrapper for a "push" type interface for a block of memory
// TODO: This probably can be eleminated, very few areas of the code actually use this.
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

  T* Push(int times){
    T* res = &ptr[timesPushed];
    timesPushed += times;

    Assert(timesPushed <= maximumTimes);
    return res;
  }

  int Mark(){
    return timesPushed;
  }

  Array<T> PopMark(int mark){
    int size = timesPushed - mark;

    Array<T> arr = {};
    arr.data = &ptr[timesPushed];
    arr.size = size;

    return arr;
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

    timesPushed = pos + times > timesPushed ? pos + times : timesPushed;
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

struct BitArray{
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

/*
  Hashmap
*/

template<typename Key,typename Data>
class HashmapIterator{
public:
  Pair<Key,Data>* pairs;
  int index;

public:
  bool operator!=(HashmapIterator& iter);
  void operator++();
  Pair<Key,Data> operator*();
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
  // Remaining data in the arena is:
  // Pair<Key,Data>* bucketsData[nodesAllocated];
  // Pair<Key,Data>* nextArray[nodesAllocated];
  // Pair<Key,Data> dataData[nodesAllocated];

  // Construct by calling PushHashmap
  
  Data* Insert(Key key,Data data);
  Data* InsertIfNotExist(Key key,Data data);
  bool  CheckOrInsert(Key key,Data data); // Returns true if key already exists, otherwise inserts and returns false.
  
  //void Remove(Key key); // TODO: For current implementation, it is difficult to remove keys and keep the same properties (being able to iterate by order of insertion). Probably need to change impl to keep a linked list 
  
  Data* Get(Key key);
  Data* GetOrInsert(Key key,Data data);
  Data& GetOrFail(Key key);
  GetOrAllocateResult<Data> GetOrAllocate(Key key); // More efficient way for the Get, check if null, Insert pattern

  void Clear();

  bool Exists(Key key);
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
Array<Key> PushHashmapKeyArray(Arena* out,Hashmap<Key,Data>* hashmap);

template<typename Key,typename Data>
Array<Data> PushHashmapDataArray(Arena* out,Hashmap<Key,Data>* hashmap);

/*
  Set
*/

// Set implementation for arenas
template<typename Data>
struct Set{
  // TODO: Better impl would not use a map due to wasted space, but its fine for now
  Hashmap<Data,int>* map;

  void Insert(Data data);

  bool ExistsOrInsert(Data data);
  bool Exists(Data data);
};

template<typename Data>
class SetIterator{
public:
  HashmapIterator<Data,int> innerIter;

public:
  bool operator!=(SetIterator& iter);
  void operator++();
  Data& operator*();
};

template<typename Data>
Set<Data>* PushSet(Arena* arena,int maxAmountOfElements);

template<typename Data>
SetIterator<Data> begin(Set<Data>* set);

template<typename Data>
SetIterator<Data> end(Set<Data>* set);

/*
  TrieMap
*/

template<typename Key,typename Data>
struct TrieMapNode{
  TrieMapNode<Key,Data>* childs[4];
  Pair<Key,Data> pair;
  TrieMapNode<Key,Data>* next;
};

template<typename Key,typename Data>
struct TrieMapIterator{
  TrieMapNode<Key,Data>* ptr;

  bool operator!=(TrieMapIterator& iter);
  void operator++();
  Pair<Key,Data>& operator*(); // Care when changing values of Key, can only change values that do not change Hash value or equality otherwise TrieMap stops working
};

template<typename Key,typename Data>
struct TrieMap{
  TrieMapNode<Key,Data>* childs[4];

  Arena* arena;
  TrieMapNode<Key,Data>* head;
  TrieMapNode<Key,Data>* tail;
  int inserted;
  
  Data* Insert(Key key,Data data);
  Data* InsertIfNotExist(Key key,Data data);
  
  Data* Get(Key key);
  Data* GetOrInsert(Key key,Data data);
  Data& GetOrFail(Key key);
  GetOrAllocateResult<Data> GetOrAllocate(Key key); // More efficient way for the Get, check if null, Insert pattern

  bool Exists(Key key);
};

template<typename Key,typename Data>
TrieMap<Key,Data>* PushTrieMap(Arena* arena);

template<typename Key,typename Data>
TrieMapIterator<Key,Data> begin(TrieMap<Key,Data>* map);

template<typename Key,typename Data>
TrieMapIterator<Key,Data> end(TrieMap<Key,Data>* map);

/*
  TrieSet
 */

template<typename Data>
struct TrieSetIterator{
  TrieMapIterator<Data,int> innerIter;

public:
  bool operator!=(TrieSetIterator& iter);
  void operator++();
  Data& operator*();
};
  
template<typename Data>
struct TrieSet{
  TrieMap<Data,int>* map;

  void Insert(Data data);

  bool ExistsOrInsert(Data data);
  bool Exists(Data data);
};

template<typename Data>
TrieSet<Data>* PushTrieSet(Arena* arena);

template<typename Data>
TrieSetIterator<Data> begin(TrieSet<Data>* set);

template<typename Data>
TrieSetIterator<Data> end(TrieSet<Data>* set);

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
public:
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

// Start of implementation

template<typename T> bool Inside(PushPtr<T>* push,T* ptr){
  bool res = (ptr >= push->ptr && ptr < (push->ptr + push->maximumTimes));
  return res;
}

template<typename T> DynamicArray<T> StartArray(Arena* arena){
  DynamicArray<T> arr = {};

  AlignArena(arena,alignof(T));

  arr.mark = MarkArena(arena);

#ifdef VERSAT_DEBUG
  arena->locked = true;
#endif

  return arr;
}

template<typename T>
DynamicArray<T>::~DynamicArray(){
  // Unlocking the arena must be done in the destructor as well otherwise
  // a return of function would leave the arena locked for good.
#ifdef VERSAT_DEBUG
  mark.arena->locked = false;
#endif
}

template<typename T> Array<T> DynamicArray<T>::AsArray(){
  Byte* data = mark.mark;
  int size = &mark.arena->mem[mark.arena->used] - data;

  Assert(size >= 0);
  Assert(size % sizeof(T) == 0); // Ensures data is properly aligned
  
  Array<T> res = {};
  res.data = (T*) data;
  res.size = size / sizeof(T);

  return res;
}

template<typename T> T* DynamicArray<T>::PushElem(){
#ifdef VERSAT_DEBUG
  Assert(mark.arena->locked);
  mark.arena->locked = false;
#endif

  T* res = PushStruct<T>(this->mark.arena);

#ifdef VERSAT_DEBUG
  mark.arena->locked = true;
#endif

  return res;
}
  
template<typename T> Array<T> EndArray(DynamicArray<T> arr){
  arr.mark.arena->locked = false;
  
  return arr.AsArray();
}

template<typename Key,typename Data>
bool HashmapIterator<Key,Data>::operator!=(HashmapIterator& iter){
  bool res = (iter.index != this->index);
  return res;
}

template<typename Key,typename Data>
void HashmapIterator<Key,Data>::operator++(){
  index += 1;
}

template<typename Key,typename Data>
Pair<Key,Data> HashmapIterator<Key,Data>::operator*(){
  return pairs[index];
}

template<typename Key,typename Data>
Hashmap<Key,Data>* PushHashmap(Arena* arena,int maxAmountOfElements){
  Hashmap<Key,Data>* map = PushStruct<Hashmap<Key,Data>>(arena);
  *map = {};

  if(maxAmountOfElements > 0){
    int size = AlignNextPower2(maxAmountOfElements) * 2;

    map->nodesAllocated = size;
    map->nodesUsed = 0;
    map->buckets = PushArray<Pair<Key,Data>*>(arena,size).data;
    map->next = PushArray<Pair<Key,Data>*>(arena,size).data;
    map->data = PushArray<Pair<Key,Data>>(arena,size).data;

    map->Clear();
  }

  return map;
}

template<typename Key,typename Data>
Hashmap<Key,Data>* PushHashmap(DynamicArena* arena,int maxAmountOfElements){
  //TODO: This is a copy of PushHashmap but with a DynamicArena. Should just refactor the init to a common function
  Hashmap<Key,Data>* map = PushStruct<Hashmap<Key,Data>>(arena);
  *map = {};

  if(maxAmountOfElements > 0){
    int size = AlignNextPower2(maxAmountOfElements) * 2;

    map->nodesAllocated = size;
    map->nodesUsed = 0;
    map->buckets = PushArray<Pair<Key,Data>*>(arena,size).data;
    map->next = PushArray<Pair<Key,Data>*>(arena,size).data;
    map->data = PushArray<Pair<Key,Data>>(arena,size).data;

    map->Clear();
  }

  return map;
}

template<typename Key,typename Data>
Array<Key> PushHashmapKeyArray(Arena* out,Hashmap<Key,Data>* hashmap){
  Array<Key> res = PushArray<Key>(out,hashmap->nodesUsed);

  int index = 0;
  for(Pair<Key,Data>& pair : *hashmap){
    res[index++] = pair.first;
  }
  return res;
}

template<typename Key,typename Data>
Array<Data> PushHashmapDataArray(Arena* out,Hashmap<Key,Data>* hashmap){
  Array<Data> res = PushArray<Data>(out,hashmap->nodesUsed);

  int index = 0;
  for(Pair<Key,Data>& pair : *hashmap){
    res[index++] = pair.second;
  }
  return res;
}

template<typename Key,typename Data>
void Hashmap<Key,Data>::Clear(){
  Memset<Pair<Key,Data>*>(this->buckets,nullptr,this->nodesAllocated);
  Memset<Pair<Key,Data>*>(this->next,nullptr,this->nodesAllocated);
  nodesUsed = 0;
}

template<typename Key,typename Data>
Data* Hashmap<Key,Data>::Insert(Key key,Data data){
  int mask = this->nodesAllocated - 1;
  int index = std::hash<Key>()(key) & mask; // Size is power of 2

  Pair<Key,Data>* ptr = this->buckets[index];

  if(ptr == nullptr){
    Assert(this->nodesUsed < this->nodesAllocated);
    Pair<Key,Data>* node = &this->data[this->nodesUsed++];

    node->key = key;
    node->data = data;

    this->buckets[index] = node;

    return &node->data;
  } else {
    int previousNextIndex = 0;
    for(; ptr;){
      if(ptr->key == key){ // Same key
            ptr->data = data; // No duplicated keys, overwrite data
            return &ptr->data;
      }

      previousNextIndex = ptr - this->data;
      ptr = this->next[previousNextIndex];
    }

    Assert(this->nodesUsed < this->nodesAllocated);

    int newNodeIndex = this->nodesUsed++;
    Pair<Key,Data>* newNode = &this->data[newNodeIndex];
    newNode->key = key;
    newNode->data = data;

    this->next[previousNextIndex] = newNode;

    return &newNode->data;
  }

  return nullptr;
}

template<typename Key,typename Data>
Data* Hashmap<Key,Data>::InsertIfNotExist(Key key,Data data){
  // TODO: Could be optimized
  Data* ptr = Get(key);

  if(ptr == nullptr){
    return Insert(key,data);
  }

  return nullptr;
}

template<typename Key,typename Data>
bool Hashmap<Key,Data>::CheckOrInsert(Key key,Data data){
  // TODO: Could be optimized
  Data* ptr = Get(key);

  if(ptr == nullptr){
    Insert(key,data);
    return false;
  }

  return true;
}

#if 0
template<typename Key,typename Data>
void Hashmap<Key,Data>::Remove(Key key){
  Assert(Get(key) != nullptr);

  int mask = this->nodesAllocated - 1;
  int index = std::hash<Key>()(key) & mask; // Size is power of 2

  Pair<Key,Data>* ptr = this->buckets[index];
  Assert(ptr);

  int previousNextIndex = -1;
  int nextIndex = 0;
  for(; ptr;){
    if(ptr->key == key){ // Same key
       nextIndex = ptr - this->data;
       break;
    }

    previousNextIndex = ptr - this->data;
    ptr = this->next[previousNextIndex];
  }

  if(previousNextIndex == -1){
    this->next[nextIndex] = nullptr;
  } else {
    this->next[previousNextIndex] = this->next[nextIndex];
  }

  nodesUsed -= 1;
}
#endif

template<typename Key,typename Data>
bool Hashmap<Key,Data>::Exists(Key key){
  Data* ptr = Get(key);

  if(ptr == nullptr){
    return false;
  }
  return true;
}

template<typename Key,typename Data>
Data* Hashmap<Key,Data>::Get(Key key){
  if(this->nodesUsed == 0){
    return nullptr;
  }

  int mask = this->nodesAllocated - 1;
  int index = std::hash<Key>()(key) & mask; // Size is power of 2

  Pair<Key,Data>* ptr = this->buckets[index];
  for(; ptr;){
    if(ptr->key == key){ // Same key
         return &ptr->data;
    }

    int index = ptr - this->data;
    ptr = this->next[index];
  }

  return nullptr;
}

template<typename Key,typename Data>
Data* Hashmap<Key,Data>::GetOrInsert(Key key,Data data){
  Data* ptr = Get(key);

  if(ptr == nullptr){
    return Insert(key,data);
  }

  return ptr;
}

template<typename Key,typename Data>
Data& Hashmap<Key,Data>::GetOrFail(Key key){
  Data* ptr = Get(key);
  Assert(ptr);
  return *ptr;
}

template<typename Key,typename Data>
GetOrAllocateResult<Data> Hashmap<Key,Data>::GetOrAllocate(Key key){
  //TODO: implement this more efficiently, instead of using separated calls
  Data* ptr = Get(key);

  GetOrAllocateResult<Data> res = {};

  if(ptr){
    res.alreadyExisted = true;
  } else {
    ptr = Insert(key,(Data){});
  }

  res.data = ptr;
  return res;
}

#if 0
template<typename Key,typename Data>
HashmapIterator<Key,Data> Hashmap<Key,Data>::begin(){
  HashmapIterator<Key,Data> iter = {};
  iter.pairs = this->data;
  return iter;
}

template<typename Key,typename Data>
HashmapIterator<Key,Data> Hashmap<Key,Data>::end(){
  HashmapIterator<Key,Data> iter = {};

  iter.pairs = this->data;
  iter.index = this->nodesUsed;

  return iter;
}
#endif

template<typename Key,typename Data>
HashmapIterator<Key,Data> begin(Hashmap<Key,Data>* hashmap){
  HashmapIterator<Key,Data> iter = {};

  if(hashmap){
    iter.pairs = hashmap->data;
  }
  return iter;
}

template<typename Key,typename Data>
HashmapIterator<Key,Data> end(Hashmap<Key,Data>* hashmap){
  HashmapIterator<Key,Data> iter = {};

  if(hashmap){
    iter.pairs = hashmap->data;
    iter.index = hashmap->nodesUsed;
  }

  return iter;
}

/*
  TrieMap
*/

template<typename Key,typename Data>
TrieMap<Key,Data>* PushTrieMap(Arena* arena){
  TrieMap<Key,Data>* map = PushStruct<TrieMap<Key,Data>>(arena);
  *map = {};
  map->arena = arena;

  return map;
}

template<typename Key,typename Data>
Data* TrieMap<Key,Data>::Insert(Key key,Data data){
  int index = std::hash<Key>()(key);
  int select = index & 3;
  if(this->childs[select] == nullptr || this->childs[select]->pair.key == key){
    TrieMapNode<Key,Data>* node = PushStruct<TrieMapNode<Key,Data>>(arena);
    *node = {};
    node->pair.first = key;
    node->pair.second = data;
    this->childs[select] = node;
    
    if(!this->head){
      this->head = node;
      this->tail = node;
    } else {
      this->tail->next = node;
      this->tail = node;
    }
    return &node->pair.second;
  }

  index >>= 2;
  
  TrieMapNode<Key,Data>* current = (TrieMapNode<Key,Data>*) this->childs[select];
  for(; 1; index >>= 2){
    int select = index & 3;
    if(current->childs[select] == nullptr || current->childs[select]->pair.first == key){
      TrieMapNode<Key,Data>* node = PushStruct<TrieMapNode<Key,Data>>(arena);
      *node = {};
      node->pair.first = key;
      node->pair.second = data;
      this->tail->next = node;
      this->tail = node;

      current->childs[select] = node;
      return &node->pair.second;
    } else {
      current = current->childs[select];
    }
  }

  NOT_POSSIBLE("Should not reach here");
  return nullptr;
}

template<typename Key,typename Data>
Data* TrieMap<Key,Data>::InsertIfNotExist(Key key,Data data){
  int index = std::hash<Key>()(key);
  int select = index & 3;
  if(this->childs[select] == nullptr){
    TrieMapNode<Key,Data>* node = PushStruct<TrieMapNode<Key,Data>>(arena);
    *node = {};
    node->pair.first = key;
    node->pair.second = data;
    this->childs[select] = node;
    
    if(!this->head){
      this->head = node;
      this->tail = node;
    } else {
      this->tail->next = node;
      this->tail = node;
    }
    return &node->pair.second;
  } else if(this->childs[select]->pair.key == key){
    return &this->childs[select]->pair.second;
  }

  index >>= 2;
  
  TrieMapNode<Key,Data>* current = (TrieMapNode<Key,Data>*) this->childs[select];
  for(; 1; index >>= 2){
    int select = index & 3;
    if(current->childs[select] == nullptr){
      TrieMapNode<Key,Data>* node = PushStruct<TrieMapNode<Key,Data>>(arena);
      *node = {};
      node->pair.first = key;
      node->pair.second = data;
      this->tail->next = node;
      this->tail = node;

      current->childs[select] = node;
      return &node->pair.second;
    } else if(current->childs[select]->pair.first == key) {
      return &current->childs[select]->pair.second;
    } else {
      current = current->childs[select];
    }
  }

  NOT_POSSIBLE("Should not reach here");
  return nullptr;
}
  
template<typename Key,typename Data>
Data* TrieMap<Key,Data>::Get(Key key){
  int index = std::hash<Key>()(key);
  int select = index & 3;
  if(this->childs[select] == nullptr){
    return nullptr;
  } else if(this->childs[select]->pair.key == key){
    return &this->childs[select]->pair.second;
  }

  index >>= 2;
  
  TrieMapNode<Key,Data>* current = (TrieMapNode<Key,Data>*) this->childs[select];
  for(; 1; index >>= 2){
    int select = index & 3;
    if(current->childs[select] == nullptr){
      return nullptr;
    } else if(current->childs[select]->pair.first == key) {
      return &current->childs[select]->pair.second;
    } else {
      current = current->childs[select];
    }
  }

  NOT_POSSIBLE("Should not reach here");
  return nullptr;
}

template<typename Key,typename Data>
Data* TrieMap<Key,Data>::GetOrInsert(Key key,Data data){
  Data* got = Get(key);

  if(got == nullptr){
    got = Insert(key,data);
  }

  return got;
}

template<typename Key,typename Data>
Data& TrieMap<Key,Data>::GetOrFail(Key key){
  Data* got = Get(key);
  Assert(got);
  return *got;
}

template<typename Key,typename Data>
GetOrAllocateResult<Data> TrieMap<Key,Data>::GetOrAllocate(Key key){
  // TODO: Could improve performance
  GetOrAllocateResult<Data> res = {};
  
  Data* got = Get(key);
  if(got){
    res.data = got;
    res.alreadyExisted = true;
  } else {
    res.data = Insert(key,{});
    res.alreadyExisted = false;
  }
  
  return res;
}

template<typename Key,typename Data>
bool TrieMap<Key,Data>::Exists(Key key){
  Data* data = Get(key);
  bool res = (data != nullptr);
  return res;
}

template<typename Key,typename Data>
TrieMapIterator<Key,Data> begin(TrieMap<Key,Data>* map){
  TrieMapIterator<Key,Data> iter = {};

  if(map) iter.ptr = map->head;

  return iter;
}

template<typename Key,typename Data>
TrieMapIterator<Key,Data> end(TrieMap<Key,Data>* map){
  TrieMapIterator<Key,Data> iter = {};
  
  return iter;
}

template<typename Key,typename Data>
bool TrieMapIterator<Key,Data>::operator!=(TrieMapIterator& iter){
  return this->ptr != iter.ptr;
}

template<typename Key,typename Data>
void TrieMapIterator<Key,Data>::operator++(){
  if(this->ptr) this->ptr = this->ptr->next;
}

template<typename Key,typename Data>
Pair<Key,Data>& TrieMapIterator<Key,Data>::operator*(){
  return this->ptr->pair;
}

/*
  Set
*/

template<typename Data>
void Set<Data>::Insert(Data data){
  map->Insert(data,0);
}

template<typename Data>
bool Set<Data>::ExistsOrInsert(Data data){
  if(Exists(data)){
    return true;
  }

  Insert(data);
  return false;
}

template<typename Data>
bool Set<Data>::Exists(Data data){
  int* possible = map->Get(data);
  bool res = (possible != nullptr);
  return res;
}

template<typename Data>
Set<Data>* PushSet(Arena* arena,int maxAmountOfElements){
  Set<Data>* set = PushStruct<Set<Data>>(arena);
  set->map = PushHashmap<Data,int>(arena,maxAmountOfElements);

  return set;
}

template<typename Data>
bool SetIterator<Data>::operator!=(SetIterator<Data>& iter){
  return (this->innerIter != iter.innerIter);
}

template<typename Data>
void SetIterator<Data>::operator++(){
  ++this->innerIter;  
}

template<typename Data>
Data& SetIterator<Data>::operator*(){
  return (*this->innerIter).first;
}

template<typename Data>
SetIterator<Data> begin(Set<Data>* set){
  SetIterator<Data> iter = {};
  iter.innerIter = begin(set->map);
  return iter;
}

template<typename Data>
SetIterator<Data> end(Set<Data>* set){
  SetIterator<Data> iter = {};
  iter.innerIter = end(set->map);
  return iter;
}

/*
  TrieSet
*/

template<typename Data>
bool TrieSetIterator<Data>::operator!=(TrieSetIterator<Data>& iter){
  return this->map != iter.map;
}

template<typename Data>
void TrieSetIterator<Data>::operator++(){
  ++this->map;
}

template<typename Data>
Data& TrieSetIterator<Data>::operator*(){
  return this->map->first;
}
  
template<typename Data>
void TrieSet<Data>::Insert(Data data){
  this->map->Insert(data,0);
}

template<typename Data>
bool TrieSet<Data>::ExistsOrInsert(Data data){
  bool res = map->Exists(data);

  if(res){
    return true;
  } else {
    map->Insert(data,0);
  }

  return false;
}

template<typename Data>
bool TrieSet<Data>::Exists(Data data){
  bool res = map->Exists(data);
  return res;
}

template<typename Data>
TrieSet<Data>* PushTrieSet(Arena* arena){
  TrieSet<Data>* set = PushStruct<TrieSet<Data>>(arena);

  set->map = PushTrieMap<Data,int>(arena);

  return set;
}

template<typename Data>
TrieSetIterator<Data> begin(TrieSet<Data>* set){
  TrieSetIterator<Data> iter = {};
  iter.innerIter = begin(set->map);
  return iter;
}

template<typename Data>
TrieSetIterator<Data> end(TrieSet<Data>* set){
  TrieSetIterator<Data> iter = {};
  iter.innerIter = end(set->map);
  return iter;
}

/*
  Pool
*/

template<typename T>
void PoolIterator<T>::Init(Pool<T>* pool,Byte* page){
  *this = {};

  this->pool = pool;
  this->page = page;
  this->bit = 7;

  if(page){
    pageInfo = GetPageInfo(pool->info,page);

    if(!IsValid()){
      ++(*this);
    }
  }
}

template<typename T>
bool PoolIterator<T>::operator!=(PoolIterator<T>& iter){
  bool res = this->page != iter.page; // We only care about for ranges, so no need to be specific

  return res;
}

template<typename T>
void PoolIterator<T>::Advance(){
  PoolInfo& info = pool->info;

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
    if(page != nullptr){
      pageInfo = GetPageInfo(info,page);
    }
  }
}

template<typename T>
bool PoolIterator<T>::IsValid(){
  bool res = pageInfo.bitmap[index] & (1 << bit);

  return res;
}

template<typename T>
void PoolIterator<T>::operator++(){
  while(page){
    Advance();

    if(IsValid()){
      break;
    }
  }
}

template<typename T>
T* PoolIterator<T>::operator*(){
  Assert(page != nullptr);

  T* view = (T*) page;
  T* val = &view[index * 8 + (7 - bit)];

  return val;
}

#if 0
template<typename T>
Pool<T>::Pool()
:mem(nullptr)
,allocated(0)
,endSize(0)
{

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
#endif

template<typename T>
T* Pool<T>::Alloc(){
  static int bitmask[] = {0x80,0xc0,0xe0,0xf0,0xf8,0xfc,0xfe,0xff};

  if(!mem){
    mem = (Byte*) AllocatePages(1);
    info = CalculatePoolInfo(sizeof(T));
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
    u8 val = (u8) page.bitmap[index];

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

            fullIndex += index * 8 + (7 - i);

            T* inst = new (&view[index * 8 + (7 - i)]) T(); // Needed for anything stl based

            return inst;
      }
    }
  }

  Assert(0);
  return nullptr;
}

template<typename T>
T* Pool<T>::Alloc(int index){
  if(!mem){
    mem = (Byte*) AllocatePages(1);
    info = CalculatePoolInfo(sizeof(T));
  }

  Byte* ptr = mem;
  PageInfo page = GetPageInfo(info,ptr);
  while(index >= info.unitsPerPage){
    if(!page.header->nextPage){
      page.header->nextPage = (Byte*) AllocatePages(1);
    }

    ptr = page.header->nextPage;
    page = GetPageInfo(info,ptr);
    index -= info.unitsPerPage;
  }

  int bitmapIndex = index / 8;
  int bitIndex = (7 - index % 8);

  if(page.bitmap[bitmapIndex] & (1 << bitIndex)){
    return nullptr;
  }

  T* view = (T*) ptr;
  page.bitmap[bitmapIndex] |= (1 << bitIndex);

  return &view[bitmapIndex * 8 + (7 - bitIndex)];
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
T& Pool<T>::GetOrFail(int index){
  T* res = Get(index);

  Assert(res);

  return *res;
}

template<typename T>
void Pool<T>::Remove(T* elem){
  Byte* page = (Byte*) ((uintptr_t)elem & ~(GetPageSize() - 1));
  PageInfo pageInfo = GetPageInfo(info,page);

  int pageIndex = ((Byte*) elem - page) / sizeof(T);
  int bitmapIndex = pageIndex / 8;
  int bit = 7 - (pageIndex % 8);

  pageInfo.bitmap[bitmapIndex] &= ~(1 << bit);
  pageInfo.header->allocatedUnits -= 1;
}

template<typename T>
Byte* Pool<T>::GetMemoryPtr(){
  return mem;
}

template<typename T>
Pool<T> Pool<T>::Copy(){
  Pool<T> other = {};

  for(T* ptr : *this){
    T* inst = other.Alloc();
    *inst = *ptr;
  }

  return other;
}

template<typename T>
int Pool<T>::Size(){
  int count = 0;

  Byte* page = mem;
  while(page){
    PageInfo pageInfo = GetPageInfo(info,page);
    count += pageInfo.header->allocatedUnits;
    page = pageInfo.header->nextPage;
  }

  return count;
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
}

template<typename T>
PoolIterator<T> Pool<T>::begin(){
  PoolIterator<T> iter = {};
  iter.Init(this,mem);

  return iter;
}

template<typename T>
PoolIterator<T> Pool<T>::end(){
  PoolIterator<T> iter = {};
  iter.Init(this,nullptr);

  return iter;
}

template<typename T>
PoolIterator<T> Pool<T>::beginNonValid(){
  PoolIterator<T> iter = {};
  iter.Init(this,mem);

  return iter;
}

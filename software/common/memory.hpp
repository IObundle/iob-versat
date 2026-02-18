#pragma once

#include "utilsCore.hpp"

#if defined(__SANITIZE_ADDRESS__)
#include <sanitizer/asan_interface.h>
#define ASAN_POISON_MEMORY_REGION(addr, size) \
  __asan_poison_memory_region((addr), (size))
#define ASAN_UNPOISON_MEMORY_REGION(addr, size) \
  __asan_unpoison_memory_region((addr), (size))
#else
#define ASAN_POISON_MEMORY_REGION(addr, size) \
  ((void)(addr), (void)(size))
#define ASAN_UNPOISON_MEMORY_REGION(addr, size) \
  ((void)(addr), (void)(size))
#endif 

struct Arena;
extern Arena* contextArenas[2];

extern Arena singleUseCasesArenas[8];
extern u8 singleUseArenaBeingUsed;

// Pass nullptr to get one arena, pass an arena to get a guaranteed different arena to the one passed
// Calling code must pass any arena that it contains to this function to make sure that the arena returned is different. We only receive one currently because we only expect to support out/temp flows (only 2 arenas required).
// Check the TEMP_REGION macros and their usage to better understand how to use this approach for memory management.
// Credit to Ryan Fleury for this technique.
Arena* GetArena(Arena* diff);
Arena* GetArena2(Arena* diff,Arena* diff2);

Arena* GetSingleUseArena();
void FreeSingleUseArena(Arena* arena);

inline size_t Kilobyte(int val){return val * 1024;};
inline size_t Megabyte(int val){return Kilobyte(val) * 1024;};
inline size_t Gigabyte(int val){return Megabyte(val) * 1024;};

inline size_t BitSizeToByteSize(int bitSize){return ((bitSize + 7) / 8);};
inline size_t BitSizeToDWordSize(int bitSize){return ((bitSize + 31) / 32);};

int AlignToNextPage(int amount);
int GetPageSize();
void* AllocatePages(int pages);
void DeallocatePages(void* ptr,int pages);
long PagesAvailable();

enum ArenaFlags : u8{
  ArenaFlags_NO_POP = (1 << 1) // Mark an arena such that 
};

struct Arena{
  Byte* mem;
  size_t used;
  size_t totalAllocated;
  size_t maximum;
  
  const char* fileCreationPlace;
  u16 lineCreationPlace;

  ArenaFlags flags;
}; 

#define InitArena(SIZE) InitArena_(SIZE,__FILE__,__LINE__);
Arena InitArena_(size_t size,const char* file,int line);

void AlignArena(Arena* arena,int alignment);
void Reset(Arena* arena);
void Free(Arena* arena);
size_t MemoryUsage(Arena* arena);
Byte* PushBytes(Arena* arena, size_t size);
size_t SpaceAvailable(Arena* arena);

void ReportArenaUsage();
void DebugInitRegion(Arena* arena,const char* file,const char* function,int line);
void DebugEndRegion(Arena* arena,const char* file,const char* function,int line);

// All push strings append a null byte. Do not rely on this behaviour.
String PushString(Arena* arena,String ss);
String PushString(Arena* arena,const char* format,...) __attribute__ ((format (printf, 2, 3)));
String vPushString(Arena* arena,const char* format,va_list args);

// TODO: Remove this, since all the push functions now append a null byte regardless.
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
String PushFile(Arena* arena,String filepath);
String PushFile(Arena* arena,const char* filepath);

struct ArenaMarker{
  ArenaMark mark;
  const char* functionName;
  const char* file;
  int line;
  
  ArenaMarker(Arena* arena,const char* file,const char* functionName,int line){
    this->mark = MarkArena(arena);
    this->functionName = functionName;
    this->line = line;
    this->file = file;
    DebugInitRegion(arena,file,functionName,line);
  };
  ~ArenaMarker(){
    DebugEndRegion(mark.arena,file,functionName,line); PopMark(this->mark);};
  operator bool(){return true;}; // For the region trick
};

#define __marker(LINE) marker_ ## LINE
#define _marker(LINE) __marker( LINE )
#define BLOCK_REGION(ARENA) ArenaMarker _marker(__LINE__)(ARENA,__FILE__,__FUNCTION__,__LINE__)

#define region(ARENA) if(ArenaMarker _marker(__LINE__){ARENA,__FILE__,__FUNCTION__,__LINE__})

struct ArenaFlagsMarker{
  Arena* arena;
  ArenaFlags previousValue;
  
  ArenaFlagsMarker(Arena* arena,ArenaFlags newFlags){
    this->arena = arena;
    this->previousValue = arena->flags;
    arena->flags = newFlags;
  };
  ~ArenaFlagsMarker(){
    this->arena->flags = this->previousValue;
  };
};

#define __marker2(LINE) marker2_ ## LINE
#define _marker2(LINE) __marker2( LINE )
#define ARENA_FLAGS_REGION(ARENA,NEWFLAGS) ArenaFlagsMarker _marker2(__LINE__)(ARENA,NEWFLAGS)

struct FreeArenaMark{
  Arena* arena;
  
  FreeArenaMark(Arena* arena){
    this->arena = arena;
  };
  ~FreeArenaMark(){
    FreeSingleUseArena(this->arena);
  };
};

#define __marker3(LINE) marker3_ ## LINE
#define _marker3(LINE) __marker3( LINE )

#define FREE_ARENA(NAME) Arena* NAME = GetSingleUseArena(); \
  FreeArenaMark _marker3(__LINE__)(NAME)

// NOTE: Use these helpers to interact with the arena flags stuff. Do not do it manually.
#define ARENA_NO_POP(ARENA) ARENA_FLAGS_REGION(ARENA,(ArenaFlags) (((u8) (ARENA)->flags) | (u8) ArenaFlags_NO_POP))

#define TEMP_REGION(NAME,OUT_ARENA) \
  Arena* NAME = GetArena(OUT_ARENA); \
  BLOCK_REGION(NAME)

#define TEMP_REGION2(NAME,OUT_ARENA,OUT_ARENA2) \
  Arena* NAME = GetArena2(OUT_ARENA,OUT_ARENA2); \
  BLOCK_REGION(NAME)

// For now let PushArray also zero out
template<typename T>
Array<T> PushArray(Arena* arena,int size){AlignArena(arena,alignof(T)); Array<T> res = {}; res.size = size; res.data = (T*) PushBytes(arena,sizeof(T) * size); Memset(res,(T){}); return res;};

inline void MemZero_(void* ptr,ssize_t size){u8* view = (u8*) ptr; for(ssize_t i = 0; i < size; i++) view[i] = 0;};
#define MemZero(PTR,TYPE) MemZero_(PTR,sizeof(TYPE));

template<typename T>
T* PushStruct(Arena* arena){AlignArena(arena,alignof(T)); T* res = (T*) PushBytes(arena,sizeof(T)); MemZero(res,T); return res;};

// TODO: Remove this, do a proper linked list arena implementation 
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
struct GrowableArray{
  Arena* arena;
  T* data;
  int size;
  int capacity;

  T& operator[](int index){
    while(size <= index){
      PushElem();
    }
    return data[index];
  }
  
  void PushArray(Array<T> arr){
    for(int i = 0; i < arr.size; i++){
      *PushElem() = arr[i];
    }
  }

  Array<T> AsArray();
  T* PushElem();
};

template<typename T>
GrowableArray<T> StartArray(Arena* arena,int startCapacity = 1){
  GrowableArray<T> res = {};

  res.arena = arena;
  res.data = PushArray<T>(arena,startCapacity).data;
  res.size = 0;
  res.capacity = startCapacity;

  return res;
}

template<typename T>
T* GrowableArray<T>::PushElem(){
  // TODO: Still need to handle alignment problems, if they appear
  
  if(size < capacity){
    return &data[size++];
  }

  // Simply extend array
  if(data + capacity == (T*) PushBytes(arena,0)){
    size += 1;
    capacity += 1;
    return PushStruct<T>(arena); // NOTE: It is possible that there exists an alignment bug hidden here, but still have not found any example that would trigger it. 
  }

  // Need to copy it
  Array<T> newArray = ::PushArray<T>(arena,capacity * 2);
  capacity = newArray.size;
  
  memcpy(newArray.data,data,sizeof(T) * size);
  data = newArray.data;
  return &data[size++];
}

template<typename T>
Array<T> GrowableArray<T>::AsArray(){
  if(size == 0){
    return {};
  }
  
  Array<T> res = {};
  res.data = data;
  res.size = size;
  return res;
}

template<typename T>
Array<T> EndArray(GrowableArray<T> arr){
  return arr.AsArray();
}

#define STRING_NODE_SIZE (1024 - sizeof(void*) - sizeof(i16)) 
struct StringNode{
  StringNode* next;
  i16 used;
  char buffer[STRING_NODE_SIZE];
};

struct StringBuilder{
  Arena* arena;
  StringNode* head;
  StringNode* tail;
  bool debugPrint;
    
  // Allocates using the arena provided. EndString joins all these into one final string in the out arena
  void PushChar(char ch);
  void PushString(String str);
  void PushString(const char* format,...) __attribute__ ((format (printf, 2, 3)));
  void vPushString(const char* format,va_list args);

  void PushSpaces(int amount);
};

StringBuilder* StartString(Arena* arena);
String EndString(Arena* out,StringBuilder* builder);

template<typename T>
class Stack{
public:
  Array<T> array;
  int used;

  int Size(){return used;};
  void Push(T t){array[used++] = t;};
  T Pop(){Assert(used > 0); return array[--used];};
};

template<typename T>
Stack<T>* PushQueue(Arena* out,int maxSize){
  Stack<T>* res = PushStruct<Stack<T>>(out);
  *res = {};
  
  res->array = PushArray<T>(out,maxSize);

  return res;
}

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
  Pair<Key,Data*> operator*();
};

template<typename Data>
struct GetOrAllocateResult{
  Data* data;
  bool alreadyExisted;
};

// An hashmap implementation for arenas. Does not allocate any memory after construction (fixed max size) and also iterates by order of insertion. Construct with PushHashmap function
template<typename Key,typename Data>
struct Hashmap{
  int nodesAllocated;
  union{
    int nodesUsed;
    int size;
  };
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
Array<Pair<Key,Data>> PushHashmapArray(Arena* out,Hashmap<Key,Data>* hashmap);

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
  
  bool ExistsOrInsert(Data data); // Returns true if exists otherwise false and inserts
  bool Exists(Data data);
};

template<typename Data>
class SetIterator{
public:
  HashmapIterator<Data,int> innerIter;

public:
  bool operator!=(SetIterator& iter);
  void operator++();
  Data operator*();
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
  bool valid;
};

template<typename Key,typename Data>
struct TrieMapIterator{
  TrieMapNode<Key,Data>* ptr;

  bool operator!=(TrieMapIterator& iter);
  void operator++();
  Pair<Key,Data> operator*(); // TODO: Put data as a pointer, to allow code to change it if needed
};

template<typename Key,typename Data>
struct TrieMap{
  TrieMapNode<Key,Data>* childs[4];

  Arena* arena;
  TrieMapNode<Key,Data>* head;
  TrieMapNode<Key,Data>* tail;
  int inserted; // Technically size, not a total count of how many where inserted
  
  Data* Insert(Key key,Data data);
  Data* InsertIfNotExist(Key key,Data data);
  
  Data* Get(Key key);
  Data* GetOrInsert(Key key,Data data);
  Data& GetOrFail(Key key);
  Data& GetOrElse(Key key,Data&& other);
  GetOrAllocateResult<Data> GetOrAllocate(Key key); // More efficient way for the Get, check if null, Insert pattern

  bool Remove(Key key);
  void RemoveOrFail(Key key);

  void Clear(); // TODO: Not properly tested.
  
  bool Exists(Key key);

  __attribute__((noinline)) Array<Pair<Key,Data>> AsArray(Arena* out);
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
  Data operator*();
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
  int unitsPerPageBlock;
  int bitmapSize;
  int pagesPerBlock;
};

struct PageInfo{
  PoolHeader* header;
  Byte* bitmap;
};

struct GenericPoolIterator{
  PoolInfo poolInfo;
  PageInfo pageInfo;
  int fullIndex;
  int bit;
  int index;
  int elemSize;
  Byte* page;

  void Init(Byte* page,int elemSize);

  bool HasNext();
  void operator++();
  Byte* operator*();
};

GenericPoolIterator IteratePool(void* pool,int sizeOfType,int alignmentOfType);
bool HasNext(GenericPoolIterator iter);
void* Next(GenericPoolIterator& iter);

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
  bool operator==(PoolIterator& iter);
  void operator++();
  T* operator*();
};

PoolInfo CalculatePoolInfo(int elemSize);
PageInfo GetPageInfo(PoolInfo poolInfo,Byte* page);

// A vector like class except no reallocations. Useful for storing entities that cannot be reallocated (so we can store pointers to them directly).
// While we could build this on top of an arena, this container uses mapped pages proprieties to work, so it needs to be standalone.
// TODO: There is a problem with the interface and the usage of this class. While we want to use this as a vector, we also want to guarantee that data is never moved. This means that Removing an elem cannot do it by copying. Because of this, when we iterate a Pool, we need to be able to handle units in the middle being removed, which prevents the iteration from being fast. A Pool where Remove does not exist would be much faster than a Pool that allows removing elements. I think it would be better to add a Pool that does not allow Remove and let the programmer use a fast Pool (in fact, I already got a problem when using the slow Pool before, so this is needed).
template<typename T>
struct Pool{
  Byte* mem; // TODO: replace with PoolHeader instead of using Byte and casting
  PoolInfo info;
public:
  T* Alloc();
  T* Alloc(int index); // Not used and complicates Pool implementation, but for now we keep it.
  void Remove(T* elem);

  T* Get(int index); // Returns nullptr if element not allocated (call Alloc(int index) to allocate)
  T& GetOrFail(int index);
  Byte* GetMemoryPtr();

  T* GetFromAllocated(int index);
  
  int Size();
  void Clear(bool clearMemory = false);
  Pool<T> Copy();

  PoolIterator<T> begin();
  PoolIterator<T> end();

  PoolIterator<T> beginNonValid();

  int MemoryUsage();
};


template<typename Key,typename Data>
struct MyHashmapIterator{
  HashmapIterator<Key,Data> iter;
  HashmapIterator<Key,Data> end;

  bool HasNext(){
    return (iter != end);
  };

  Pair<Key,Data*> Next(){
    auto res = *iter;
    ++iter;
    return res;
  };
};

template<typename Key,typename Data>
MyHashmapIterator<Key,Data> Iterate(Hashmap<Key,Data>* hashmap){
  MyHashmapIterator<Key,Data> res = {};

  res.iter = begin(hashmap);
  res.end  = end(hashmap);
  return res;
}

template<typename Key,typename Data>
MyHashmapIterator<Key,Data> Iterate(Hashmap<Key,Data>& hashmap){
  MyHashmapIterator<Key,Data> res = {};

  res.iter = begin(&hashmap);
  res.end  = end(&hashmap);
  return res;
}

struct GenericArrayIterator{
  void* array;
  int sizeOfType;
  int alignmentOfType;
  int index;
};

struct GenericTrieMapIterator{
  void* trieMap;
  void* ptr;
  int sizeOfType;
  int alignmentOfType;
  int index;
};

struct GenericHashmapIterator{
  void* hashmap;
  int sizeOfType;
  int alignmentOfType;
  int index;
};

// Start of implementation

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
Pair<Key,Data*> HashmapIterator<Key,Data>::operator*(){
  Pair<Key,Data*> p = {pairs[index].first,&pairs[index].second};
  return p;
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
Array<Pair<Key,Data>> PushHashmapArray(Arena* out,Hashmap<Key,Data>* hashmap){
  Array<Pair<Key,Data>> res = PushArray<Pair<Key,Data>>(out,hashmap->nodesUsed);

  int index = 0;
  for(Pair<Key,Data*>& pair : *hashmap){
    res[index++] = {pair.first,*pair.second};
  }
  return res;

}

template<typename Key,typename Data>
Array<Key> PushHashmapKeyArray(Arena* out,Hashmap<Key,Data>* hashmap){
  Array<Key> res = PushArray<Key>(out,hashmap->nodesUsed);

  int index = 0;
  for(Pair<Key,Data*>& pair : *hashmap){
    res[index++] = pair.first;
  }
  return res;
}

template<typename Key,typename Data>
Array<Data> PushHashmapDataArray(Arena* out,Hashmap<Key,Data>* hashmap){
  Array<Data> res = PushArray<Data>(out,hashmap->nodesUsed);

  int index = 0;
  for(Pair<Key,Data*>& pair : *hashmap){
    res[index++] = *pair.second;
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
  u64 index = Hash(key) & mask; // Size is power of 2

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
  u64 index = Hash(key) & mask; // Size is power of 2
  
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
using ChildrenPtr = TrieMapNode<Key,Data>*[4];
  
template<typename Key,typename Data>
Data* TrieMap<Key,Data>::Insert(Key key,Data data){
  u64 index = Hash(key);

  TrieMapNode<Key,Data>* (*current)[4] = &this->childs;

  for(; 1; index >>= 2){
    int select = index & 3;
    if((*current)[select] == nullptr || !(*current)[select]->valid){
      TrieMapNode<Key,Data>* node = PushStruct<TrieMapNode<Key,Data>>(arena);
      *node = {};
      node->pair.first = key;
      node->pair.second = data;
      node->valid = true;

      if(!this->head){
        this->head = node;
        this->tail = node;
      } else {
        this->tail->next = node;
        this->tail = node;
      }

      (*current)[select] = node;
      inserted += 1;
      return &node->pair.second;
    } else if((*current)[select]->pair.first == key){
      (*current)[select]->pair.second = data;
      return &(*current)[select]->pair.second;
    } else {
      current = &((*current)[select]->childs);
    }
  }
  
  NOT_POSSIBLE("Should not reach here");
}

template<typename Key,typename Data>
Data* TrieMap<Key,Data>::InsertIfNotExist(Key key,Data data){
  u64 index = Hash(key);

  TrieMapNode<Key,Data>* (*current)[4] = &this->childs;
  for(; 1; index >>= 2){
    int select = index & 3;
    if((*current)[select] == nullptr || !(*current)[select]->valid){
      TrieMapNode<Key,Data>* node = PushStruct<TrieMapNode<Key,Data>>(arena);
      *node = {};
      node->pair.first = key;
      node->pair.second = data;
      node->valid = true;

      if(!this->head){
        this->head = node;
        this->tail = node;
      } else {
        this->tail->next = node;
        this->tail = node;
      }
      
      (*current)[select] = node;
      inserted += 1;
      return &node->pair.second;
    } else if((*current)[select]->pair.first == key) {
      return &(*current)[select]->pair.second;
    } else {
      current = &((*current)[select]->childs);
    }
  }

  NOT_POSSIBLE("Should not reach here");
}
  
template<typename Key,typename Data>
Data* TrieMap<Key,Data>::Get(Key key){
  u64 index = Hash(key);

  TrieMapNode<Key,Data>* (*current)[4] = &this->childs;
  for(; 1; index >>= 2){
    int select = index & 3;
    if((*current)[select] == nullptr){
      return nullptr;
    } else if((*current)[select]->valid && (*current)[select]->pair.first == key) {
      return &(*current)[select]->pair.second;
    } else {
      current = &((*current)[select]->childs);
    }
  }

  NOT_POSSIBLE("Should not reach here");
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
Data& TrieMap<Key,Data>::GetOrElse(Key key,Data&& other){
  Data* got = Get(key);

  if(!got){
    return other;
  }
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
bool TrieMap<Key,Data>::Remove(Key key){
  if(!head){
    Assert(!tail);
    return false;
  }
  
  TrieMapNode<Key,Data>* node = nullptr;
  TrieMapNode<Key,Data>* previous = nullptr;
  bool found = false;
  if(head->pair.key == key && head->valid){
    node = head;
    found = true;
  } else {
    for(node = head; node; node = node->next){
      if(node->valid && node->pair.key == key){
        found = true;
        break;
      }
      previous = node;
    }
  }

  if(!found){
    return false;
  }
  
  if(!previous){
    if(head == tail){
      head = nullptr;
      tail = nullptr;
    } else {
      head = node->next;
    }
  } else if(node == tail){
    previous->next = nullptr;
    tail = previous;
  } else{
    previous->next = node->next;
  }
  
  node->valid = false;
  node->next = nullptr;
  this->inserted -= 1;
  
  return true;
}

template<typename Key,typename Data>
void TrieMap<Key,Data>::RemoveOrFail(Key key){
  bool res = Remove(key);
  Assert(res);
}

template<typename Key,typename Data>
void TrieMap<Key,Data>::Clear(){
  for(auto* node = head; node; node = node->next){
    node->valid = false;
  }
  
  this->head = nullptr;
  this->tail = nullptr;
  this->inserted = 0;
}
  
template<typename Key,typename Data>
bool TrieMap<Key,Data>::Exists(Key key){
  Data* data = Get(key);
  bool res = (data != nullptr);
  return res;
}

template<typename Key,typename Data>
Array<Pair<Key,Data>> TrieMap<Key,Data>::AsArray(Arena* out){
  Array<Pair<Key,Data>> result = PushArray<Pair<Key,Data>>(out,this->inserted);

  int index = 0;
  for(auto pair : this){
    result[index++] = (Pair<Key,Data>){pair.first,pair.second};
  }
  
  return result;
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
Pair<Key,Data> TrieMapIterator<Key,Data>::operator*(){
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
Data SetIterator<Data>::operator*(){
  return (*this->innerIter).first;
}

template<typename Data>
SetIterator<Data> begin(Set<Data>* set){
  SetIterator<Data> iter = {};
  if(set == nullptr){
    return iter;
  }

  iter.innerIter = begin(set->map);
  return iter;
}

template<typename Data>
SetIterator<Data> end(Set<Data>* set){
  SetIterator<Data> iter = {};
  if(set == nullptr){
    return iter;
  }

  iter.innerIter = end(set->map);
  return iter;
}

/*
  TrieSet
*/

template<typename Data>
bool TrieSetIterator<Data>::operator!=(TrieSetIterator<Data>& iter){
  return this->innerIter != iter.innerIter;
}

template<typename Data>
void TrieSetIterator<Data>::operator++(){
  ++this->innerIter;
}

template<typename Data>
Data TrieSetIterator<Data>::operator*(){
  return (*this->innerIter).first;
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
bool PoolIterator<T>::operator==(PoolIterator<T>& iter){
  bool res = this->page == iter.page; // We only care about for ranges, so no need to be specific

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

  if(index * 8 + (7 - bit) >= info.unitsPerPageBlock){
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

template<typename T>
T* Pool<T>::Alloc(){
  static int bitmask[] = {0x80,0xc0,0xe0,0xf0,0xf8,0xfc,0xfe,0xff};

  if(!mem){
    info = CalculatePoolInfo(sizeof(T));
    mem = (Byte*) AllocatePages(info.pagesPerBlock);
  }

  int fullIndex = 0;
  Byte* ptr = mem;
  PageInfo page = GetPageInfo(info,ptr);
  while(page.header->allocatedUnits == info.unitsPerPageBlock){
    if(!page.header->nextPage){
      page.header->nextPage = (Byte*) AllocatePages(info.pagesPerBlock);
    }

    ptr = page.header->nextPage;
    page = GetPageInfo(info,ptr);
    fullIndex += info.unitsPerPageBlock;
  }

  T* view = (T*) ptr;
  for(int index = 0; index * 8 < info.unitsPerPageBlock; index += 1){
    u8 val = (u8) page.bitmap[index];

    if(info.unitsPerPageBlock - index * 8 < 8){
      if(val == bitmask[(info.unitsPerPageBlock - 1) % 8]){
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
        
        T* inst = &view[index * 8 + (7 - i)];
        *inst = {};

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
    info = CalculatePoolInfo(sizeof(T));
    mem = (Byte*) AllocatePages(info.pagesPerBlock);
  }

  Byte* ptr = mem;
  PageInfo page = GetPageInfo(info,ptr);
  while(index >= info.unitsPerPageBlock){
    if(!page.header->nextPage){
      page.header->nextPage = (Byte*) AllocatePages(info.pagesPerBlock);
    }

    ptr = page.header->nextPage;
    page = GetPageInfo(info,ptr);
    index -= info.unitsPerPageBlock;
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
    index -= info.unitsPerPageBlock;
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
T* Pool<T>::GetFromAllocated(int index){
  //TODO: Slow
  int i = 0;
  for(T* elem : *this){
    if(i == index){
      return elem;
    }

    i += 1;
  }

  return nullptr;
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

#pragma once

// Utilities that depend on memory stuff
// Dependency order is utilsCore <- memory <- utils

#include "utilsCore.hpp"
#include "filesystem.hpp"
#include "memory.hpp"

// TODO: There exists a bunch of enums that do not really have a good place to be and that are used throught the entier code base.
//       Misc Enums that we probably want to stuff into a Defs.hpp file
enum Direction{
  Direction_NONE,
  Direction_OUTPUT = 1,
  Direction_INPUT = 2,

  Direction_WRITE = 1,
  Direction_READ = 2
};

FILE* OpenFileAndCreateDirectories(String path,const char* format,FilePurpose purpose);

Array<String> Split(String content,char sep,Arena* out); // For now only split over one char. 

String TrimWhitespaces(String in);

String GetCommonPath(String path1,String path2,Arena* out);
String OS_NormalizePath(String in,Arena* out);

Opt<Array<String>> GetAllFilesInsideDirectory(String dirPath,Arena* out);

String PushEscapedString(Arena* out,String toEscape,char spaceSubstitute);
void   PrintEscapedString(String toEscape,char spaceSubstitute);

String GetAbsolutePath(String path,Arena* out);

String ReprMemorySize(size_t val,Arena* out);

String JoinStrings(Array<String> strings,String separator,Arena* out);

static inline bool Contains(String bigger,String smaller){
  TEMP_REGION(temp,nullptr);
  
  String withNull = PushString(temp,bigger);
  
  void* ptr = (void*) strstr(withNull.data,StaticFormat("%.*s",UN(smaller)));
  bool res = (ptr != nullptr);
  
  return res;
}

template<typename Value,typename Error>
struct Result{
  union{
    Value value;
    Error error;
  };
  bool isError;
  
  Result() = default;
  Result(Value val){value = val; isError = false;};
  Result(Error err){error = err; isError = true;};
  
  //operator bool(){return !isError;} // This was previously commented out. Do not know if this can cause any error 
};
#define CHECK(RESULT) if((RESULT).isError){return (RESULT).error;}

// For cases where both Value and Error are the type
template<typename T>
struct Result<T,T>{
  union{
    T value;
    T error;
  };
  bool isError;

  Result() = default;
};

template<typename T>
Result<T,T> MakeResultValue(T val){Result<T,T> res = {}; res.value = val; return res;};
template<typename T>
Result<T,T> MakeResultError(T val){Result<T,T> res = {}; res.error = val; res.isError = true; return res;};

// A templated type for carrying the index in an array
// A performant design would allocate a separate array because these functions copy data around.
// Use these for prototyping, easing of debugging and stuff
template<typename T>
struct IndexedStruct : public T{
   int index;
};

template<typename T>
struct SingleLink{
  SingleLink<T>* next;
  T elem;
};

template<typename T>
struct DoubleLink{
  DoubleLink<T>* next;
  DoubleLink<T>* prev;
  T elem;
};

// NOTE: Do not use this if adding nodes to the list. This is mainly a helper for quick iterations that only need to read data
template<typename T>
struct ArenaListIterator{
  SingleLink<T>* ptr;

  bool operator!=(ArenaListIterator& iter){
    return iter.ptr != this->ptr;
  }
  void operator++(){
    ptr = ptr->next;
  }
  T& operator*(){
    return ptr->elem;
  }
};

// A list inside an arena. Normally used with a temp arena and then converted to an array in an out arena 
template<typename T>
struct ArenaList{
  SingleLink<T>* head;
  SingleLink<T>* tail;
  Arena* arena; // For now store arena inside structure. 

  T* PushElem();
};

// Implement C++ style foreach 
template<typename T>
ArenaListIterator<T> begin(ArenaList<T>* list){if(!list){return end(list);};return ArenaListIterator<T>{list->head};};

template<typename T>
ArenaListIterator<T> end(ArenaList<T>* list){return ArenaListIterator<T>{nullptr};};

String JoinStrings(ArenaList<String>* strings,String separator,Arena* out);

template<typename T>
struct ArenaDoubleList{
  DoubleLink<T>* head;
  DoubleLink<T>* tail;
  Arena* arena; // For now store arena inside structure. 

  T* PushElem();
  DoubleLink<T>* PushNode();

  T* AppendNode(DoubleLink<T>* node); // Does not allocate anything in the arena. Need more testing to see if this is a good approach.
};

struct GenericArenaListIterator{
  SingleLink<int>* ptr; // The type of T is irrelevant, since pointer size is always the same
  int sizeOfType;
  int alignmentOfType;
  int index;
};

struct GenericArenaDoubleListIterator{
  DoubleLink<int>* ptr; // The type of T is irrelevant, since pointer size is always the same
  int sizeOfType;
  int alignmentOfType;
  int index;
};

// TODO: This function leaks memory, because it does not return the free nodes
template<typename T,typename Func>
T* ListRemoveAll(T* start,Func compareFunction){
   #if 0
   T* freeListHead = nullptr;
   T* freeListPtr = nullptr;
   #endif

   T* head = nullptr;
   T* listPtr = nullptr;
   for(T* ptr = start; ptr;){
      T* next = ptr->next;
      ptr->next = nullptr;
      bool comp = compareFunction(ptr);

      if(comp){ // Add to free list
         #if 0
         if(freeListPtr){
            freeListPtr->next = ptr;
            freeListPtr = ptr;
         } else {
            freeListHead = ptr;
            freeListPtr = ptr;
         }
         #endif
      } else { // "Add" to return list
         if(listPtr){
            listPtr->next = ptr;
            listPtr = ptr;
         } else {
            head = ptr;
            listPtr = ptr;
         }
      }

      ptr = next;
   }

   return head;
}

template<typename T>
Array<T> Reverse(Array<T> arr,Arena* out){
  Array<T> res = PushArray<T>(out,arr.size);
  
  for(u32 i = 0; i < arr.size; i++){
    res[arr.size - i - 1] = arr[i];
  }

  return res;
}

template<typename T>
T* ReverseList(T* head){
   if(head == nullptr){
      return head;
   }

   T* ptr = nullptr;
   T* next = head;

   while(next != nullptr){
      T* nextNext = next->next;
      next->next = ptr;
      ptr = next;
      next = nextNext;
   }

   return ptr;
}

template<typename T>
T* ListInsert(T* head,T* toAdd){
   if(!head){
      return toAdd;
   }

   toAdd->next = head;
   return toAdd;
}

template<typename T>
int Size(T* start){
   int size = 0;
   FOREACH_LIST(T*,ptr,start){
      size += 1;
   }
   return size;
}

template<typename T>
int Size(ArenaList<T>* list){
  if(!list){
    return 0;
  }

  SingleLink<T>* ptr = list->head;

  int count = 0;
  while(ptr){
    count += 1;
    ptr = ptr->next;
  }
  return count;
}

template<typename T>
int Size(ArenaDoubleList<T>* list){
  if(!list){
    return 0;
  }
  
  DoubleLink<T>* ptr = list->head;

  int count = 0;
  while(ptr){
    count += 1;
    ptr = ptr->next;
  }
  return count;
}

template<typename T>
Array<T> CopyArray(Array<T> arr,Arena* out){
  Array<T> res = PushArray<T>(out,arr.size);
  for(int i = 0; i < arr.size; i++){
    res[i] = arr[i];
  }
  return res;
}

template<typename T,typename D>
Array<T> CopyArray(Array<D> arr,Arena* out){
  Array<T> res = PushArray<T>(out,arr.size);
  for(int i = 0; i < arr.size; i++){
    res[i] = arr[i];
  }
  return res;
}

template<typename T>
ArenaList<T>* PushList(Arena* out){
  ArenaList<T>* res = PushStruct<ArenaList<T>>(out);
  *res = {};
  res->arena = out;
  
  return res;
}

template<typename T>
ArenaDoubleList<T>* PushArenaDoubleList(Arena* out){
  ArenaDoubleList<T>* res = PushStruct<ArenaDoubleList<T>>(out);
  *res = {};
  res->arena = out;
  
  return res;
}

// TODO: Remove this and use PushElem member function
template<typename T>
T* PushListElement(ArenaList<T>* list){
  Arena* out = list->arena;
  SingleLink<T>* s = PushStruct<SingleLink<T>>(out);
  *s = {};
  
  if(!list->head){
    list->head = s;
    list->tail = s;
  } else {
    list->tail->next = s;
    list->tail = s;
  }

  return &s->elem;
}

template<typename T>
T* ArenaList<T>::PushElem(){
  SingleLink<T>* s = PushStruct<SingleLink<T>>(this->arena);
  *s = {};
  
  if(!this->head){
    this->head = s;
    this->tail = s;
  } else {
    this->tail->next = s;
    this->tail = s;
  }

  return &s->elem;
}

template<typename T>
T* ArenaDoubleList<T>::PushElem(){
  DoubleLink<T>* s = PushStruct<DoubleLink<T>>(this->arena);
  *s = {};
  
  if(!this->head){
    this->head = s;
    this->tail = s;
  } else {
    this->tail->next = s;
    s->prev = this->tail;
    this->tail = s;
  }

  return &s->elem;
}

template<typename T>
T* ArenaDoubleList<T>::AppendNode(DoubleLink<T>* node){
  if(!this->head){
    this->head = node;
    this->tail = node;
  } else {
    this->tail->next = node;
    node->prev = this->tail;
    this->tail = node;
  }

  return &node->elem;
}

template<typename T>
DoubleLink<T>* ArenaDoubleList<T>::PushNode(){
  DoubleLink<T>* s = PushStruct<DoubleLink<T>>(this->arena);
  *s = {};
  
  if(!this->head){
    this->head = s;
    this->tail = s;
  } else {
    this->tail->next = s;
    s->prev = this->tail;
    this->tail = s;
  }

  return s;
}

// Returns the next node (so it is possible to continue iteration)
template<typename T>
DoubleLink<T>* RemoveNodeFromList(ArenaDoubleList<T>* list,DoubleLink<T>* node){
  DoubleLink<T>* next = node->next;
  
  if(list->head == node){
    list->head = node->next;
    if(node->next) node->next->prev = nullptr;
  } else if(list->tail == node){
    list->tail = node->prev;
    if(node->prev) node->prev->next = nullptr;
  } else {
    node->prev->next = node->next;
    node->next->prev = node->prev;
  }
  
  node->next = nullptr;
  node->prev = nullptr;

  if(list->head == nullptr || list->tail == nullptr){
    list->head = nullptr;
    list->tail = nullptr;
  }
  
  return next;
}

template<typename T,typename P>
Hashmap<T,P>* PushHashmapFromList(Arena* out,ArenaList<Pair<T,P>>* list){
  int size = Size(list);
  
  Hashmap<T,P>* map = PushHashmap<T,P>(out,size);
  
  FOREACH_LIST(auto*,iter,list->head){ // auto = ListedStruct<Pair<T,P>> (auto used because the ',' in Pair<T,P> does not play well with the foreach macro
    map->Insert(iter->elem.first,iter->elem.second);
  }

  return map;
}

template<typename T>
bool Empty(Array<T> arr){
  bool empty = (arr.size == 0);
  return empty;
}

template<typename T>
bool Empty(ArenaList<T>* list){
  if(list == nullptr){
    return true;
  }
  bool empty = (list->head == nullptr);
  return empty;
}

template<typename T>
bool Empty(ArenaDoubleList<T>* list){
  bool empty = (list->head == nullptr);
  return empty;
}

template<typename T>
bool OnlyOneElement(ArenaList<T>* list){
  bool onlyOne = (list->head != nullptr && list->head == list->tail);
  return onlyOne;
}
  
template<typename T>
Array<T> PushArray(Arena* out,ArenaList<T>* list){
  if(Empty(list)){
    return {};
  }

  auto arr = StartArray<T>(out);
  
  FOREACH_LIST(SingleLink<T>*,iter,list->head){
    T* ptr = arr.PushElem();
    *ptr = iter->elem;
  }

  Array<T> res = EndArray(arr);
  return res;
}

template<typename T,typename P>
Array<Pair<T,P>> PushArray(Arena* out,ArenaList<Pair<T,P>>* list){
  int size = Size(list);
  
  Array<Pair<T,P>> arr = PushArray<Pair<T,P>>(out,size);

  int i = 0;
  FOREACH_LIST_INDEXED(auto*,iter,list->head,i){
    arr[i] = {iter->elem.first,iter->elem.second};
  }

  return arr;
}

template<typename T,typename P>
Array<T> PushArrayFromKeys(Arena* out,Hashmap<T,P>* map){
  int size = map->nodesUsed;
  
  Array<T> arr = PushArray<T>(out,size);

  int index = 0;
  for(Pair<T,P> p : map){
    arr[index++] = p.first;
  }

  return arr;
}

template<typename T,typename P>
Array<P> PushArrayFromData(Arena* out,Hashmap<T,P>* map){
  int size = map->nodesUsed;
  
  Array<P> arr = PushArray<P>(out,size);

  int index = 0;
  for(Pair<T,P> p : map){
    arr[index++] = p.second;
  }

  return arr;
}

template<typename T,typename P>
Array<T> PushArrayFromKeys(Arena* out,TrieMap<T,P>* map){
  int size = map->nodesUsed;
  
  Array<T> arr = PushArray<T>(out,size);

  int index = 0;
  for(Pair<T,P> p : map){
    arr[index++] = p.first;
  }

  return arr;
}

template<typename T,typename P>
Array<P> PushArrayFromData(Arena* out,TrieMap<T,P>* map){
  int size = map->inserted;
  
  Array<P> arr = PushArray<P>(out,size);

  int index = 0;
  for(Pair<T,P> p : map){
    arr[index++] = p.second;
  }

  return arr;
}

template<typename T>
Array<T> PushArray(Arena* out,Set<T>* set){
  auto arr = PushArray<T>(out,set->map->nodesUsed);

  int index = 0;
  for(auto pair : set->map){
    arr[index++] = pair.first;
  }

  return arr;
}

template<typename T>
Array<T> PushArray(Arena* out,TrieSet<T>* set){
  auto arr = PushArray<T>(out,set->map->inserted);

  int index = 0;
  for(auto pair : set->map){
    arr[index++] = pair.first;
  }

  return arr;
}

template<typename T>
Set<T>* PushSet(Arena* out,ArenaList<T>* list){
  int size = Size(list);

  Set<T>* set = PushSet<T>(out,size);

  int i = 0;
  FOREACH_LIST_INDEXED(auto*,iter,list->head,i){
    set->Insert(iter->elem);
  }

  return set;
}

template<typename T>
Hashmap<T,int>* Count(Array<T> arr,Arena* out){
  Hashmap<T,int>* count = PushHashmap<T,int>(out,arr.size);

  for(T& t : arr){
    GetOrAllocateResult<int> res = count->GetOrAllocate(t);
    
    if(res.alreadyExisted){
      *res.data += 1;
    } else {
      *res.data = 1;
    }
  }

  return count;
}

template<typename T>
Hashmap<T,int>* MapElementToIndex(Array<T> arr,Arena* out){
  Hashmap<T,int>* toIndex = PushHashmap<T,int>(out,arr.size);
  int i = 0;
  for(T& t : arr){
    toIndex->Insert(t,i);
    i+=1;
  }
  return toIndex;
}

// If we find ourselves using this too much, maybe consider changing between AoS and SoA
template<typename T,typename R>
Array<R> Extract(Array<T> array,Arena* out,R T::* f){
  Array<R> result = PushArray<R>(out,array.size);
  for(int i = 0; i < array.size; i++){
    result[i] = array[i].*f;
  }
  return result;
}

template<typename T>
Array<T> Unique(Array<T> arr,Arena* out){
  TEMP_REGION(temp,out);

  Set<T>* set = PushSet<T>(temp,arr.size);
  
  for(T t : arr){
    set->Insert(t);
  }
  return PushArray(out,set);
}

// Compares arrays directly, no "set" semantics or nothing. Mostly for simple tests where we need to check if different functions return the exact same values to confirm correctness.
template<typename T>
bool Equal(Array<T> left,Array<T> right){
  if(left.size != right.size){
    return false;
  }

  int size = left.size;
  for(int i = 0; i < size; i++){
    if(!(left[i] == right[i])){
      return false; 
    }
  }
  
  return true;
}

template<typename T>
Array<T> RemoveElement(Array<T> in,int index,Arena* out){
  Assert(index < in.size);
  
  if(in.size <= 1){
    return {};
  }

  Array<T> res = PushArray<T>(out,in.size - 1);
  
  int inserted = 0;
  for(int i = 0; i < in.size; i++){
    if(i == index){
      continue;
    }

    res[inserted++] = in[i];
  }

  return res;
}

template<typename T>
Array<T> RemoveElement(Array<T> in,int index){
  Assert(index < in.size);

  if(in.size <= 1){
    return {};
  }

  for(int i = index; i < in.size - 1; i++){
    SWAP(in[i],in[i+1]);
  }
  in.size -= 1;

  return in;
}

template<typename T>
Array<T> AddElement(Array<T> in,int indexNewElement,Arena* out){
  Array<T> res = PushArray<T>(out,in.size + 1);

  int inserted = 0;
  for(int i = 0; i < in.size; i++){
    if(i == indexNewElement){
      res[inserted++] = {};
    }

    res[inserted++] = in[i];
  }

  return res;
}

// Given [A,B,C], returns [(A,[B,C]),(B,[A,C]),(C,[A,B])] and so on
template<typename T>
Array<Pair<T,Array<T>>> AssociateOneToOthers(Array<T> arr,Arena* out){
  int size = arr.size;
  if(size == 0){
    return {};
  }

  Array<Pair<T,Array<T>>> res = PushArray<Pair<T,Array<T>>>(out,size);

  for(int i = 0; i < size; i++){
    res[i].first = arr[i];
    res[i].second = PushArray<T>(out,size - 1);
    int count = 0;
    for(int ii = 0; ii < size; ii++){
      if(i == ii){
        continue;
      }
      res[i].second[count++] = arr[ii];
    }
  }

  return res;
}

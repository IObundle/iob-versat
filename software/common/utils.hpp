#pragma once

// Utilities that depend on memory or on other utilities

#include "utilsCore.hpp"
#include "filesystem.hpp"
#include "memory.hpp"

FILE* OpenFileAndCreateDirectories(String path,const char* format,FilePurpose purpose);

Opt<Array<String>> GetAllFilesInsideDirectory(String dirPath,Arena* out);

String PushEscapedString(Arena* out,String toEscape,char spaceSubstitute);
void   PrintEscapedString(String toEscape,char spaceSubstitute);

String GetAbsolutePath(const char* path,Arena* out);

Array<int> GetNonZeroIndexes(Array<int> array,Arena* out);

String JoinStrings(Array<String> strings,String separator,Arena* out);

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
  T elem;
  SingleLink<T>* next;
};

template<typename T>
struct DoubleLink{
  T elem;
  DoubleLink<T>* next;
  DoubleLink<T>* prev;
};

// A list inside an arena. Normally used with a temp arena and then converted to an array in a out arena 
template<typename T>
struct ArenaList{
  Arena* arena; // For now store arena inside structure. 
  SingleLink<T>* head;
  SingleLink<T>* tail;

  T* PushElem();
};

template<typename T>
struct ArenaDoubleList{
  Arena* arena; // For now store arena inside structure. 
  DoubleLink<T>* head;
  DoubleLink<T>* tail;

  T* PushElem();
  T* PushNode(DoubleLink<T>* node); // Does not allocate anything in the arena. Need more testing to see if this is a good approach.
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

template<typename T>
ArenaList<T>* PushArenaList(Arena* out){
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
T* ArenaDoubleList<T>::PushNode(DoubleLink<T>* node){
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
bool Empty(ArenaList<T>* list){
  bool empty = (list->head == nullptr);
  return empty;
}

template<typename T>
bool OnlyOneElement(ArenaList<T>* list){
  bool onlyOne = (list->head != nullptr && list->head == list->tail);
  return onlyOne;
}
  
template<typename T>
Array<T> PushArrayFromList(Arena* out,ArenaList<T>* list){
  if(Empty(list)){
    return {};
  }

  auto arr = StartGrowableArray<T>(out);
  
  FOREACH_LIST(SingleLink<T>*,iter,list->head){
    T* ptr = arr.PushElem();
    *ptr = iter->elem;
  }

  Array<T> res = EndArray(arr);
  return res;
}

template<typename T,typename P>
Array<Pair<T,P>> PushArrayFromList(Arena* out,ArenaList<Pair<T,P>>* list){
  int size = Size(list);
  
  Array<Pair<T,P>> arr = PushArray<Pair<T,P>>(out,size);

  int i = 0;
  FOREACH_LIST_INDEXED(auto*,iter,list->head,i){
    arr[i] = {iter->elem.first,iter->elem.second};
  }

  return arr;
}

template<typename T,typename P>
Array<T> PushArrayFromHashmapKeys(Arena* out,Hashmap<T,P>* map){
  int size = map->nodesUsed;
  
  Array<T> arr = PushArray<T>(out,size);

  int index = 0;
  for(Pair<T,P> p : map){
    arr[index++] = p.first;
  }

  return arr;
}

template<typename T,typename P>
Array<P> PushArrayFromHashmapData(Arena* out,Hashmap<T,P>* map){
  int size = map->nodesUsed;
  
  Array<P> arr = PushArray<P>(out,size);

  int index = 0;
  for(Pair<T,P> p : map){
    arr[index++] = p.second;
  }

  return arr;
}

template<typename T,typename P>
Array<T> PushArrayFromTrieMapKeys(Arena* out,TrieMap<T,P>* map){
  int size = map->nodesUsed;
  
  Array<T> arr = PushArray<T>(out,size);

  int index = 0;
  for(Pair<T,P> p : map){
    arr[index++] = p.first;
  }

  return arr;
}

template<typename T,typename P>
Array<P> PushArrayFromTrieMapData(Arena* out,TrieMap<T,P>* map){
  int size = map->inserted;
  
  Array<P> arr = PushArray<P>(out,size);

  int index = 0;
  for(Pair<T,P> p : map){
    arr[index++] = p.second;
  }

  return arr;
}

template<typename T>
Array<T> PushArrayFromSet(Arena* out,Set<T>* set){
  auto arr = PushArray<T>(out,set->map->nodesUsed);

  int index = 0;
  for(auto pair : set->map){
    arr[index++] = pair.first;
  }

  return arr;
}

template<typename T>
Array<T> PushArrayFromSet(Arena* out,TrieSet<T>* set){
  auto arr = PushArray<T>(out,set->map->nodesUsed);

  int index = 0;
  for(auto pair : set->map){
    arr[index++] = pair.first;
  }

  return arr;
}

template<typename T>
Set<T>* PushSetFromList(Arena* out,ArenaList<T>* list){
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
Array<T> Unique(Array<T> arr,Arena* out,Arena* temp){
  BLOCK_REGION(temp);

  Set<T>* set = PushSet<T>(temp,arr.size);
  
  for(T t : arr){
    set->Insert(t);
  }
  return PushArrayFromSet(out,set);
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

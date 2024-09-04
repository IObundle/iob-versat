#pragma once

// Utilities that depend on memory

#include "utilsCore.hpp"
#include "memory.hpp"

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
struct ListedStruct{
  T elem;
  ListedStruct<T>* next;
};

// A list inside an arena. Normally used with a temp arena and then converted to an array in a out arena 
template<typename T>
struct ArenaList{
  Arena* arena; // For now store arena inside structure. 
  ListedStruct<T>* head;
  ListedStruct<T>* tail;

  T* PushElem();
};

template<typename T>
struct Stack{
  Array<T> mem;
  int index;
};

// Generic list manipulation, as long as the structure has a next pointer of equal type
template<typename T>
T* ListGet(T* start,int index){
   T* ptr = start;
   for(int i = 0; i < index; i++){
      if(ptr){
         ptr = ptr->next;
      }
   }
   return ptr;
}

template<typename T>
int ListIndex(T* start,T* toFind){
   int i = 0;
   FOREACH_LIST(T*,ptr,start){
      if(ptr == toFind){
         break;
      }
      i += 1;
   }
   return i;
}

template<typename T>
T* ListRemove(T* start,T* toRemove){ // Returns start of new list. ToRemove is still valid afterwards
   if(start == toRemove){
      return start->next;
   } else {
      T* previous = nullptr;
      FOREACH_LIST(T*,ptr,start){
         if(ptr == toRemove){
            previous->next = ptr->next;
         }
         previous = ptr;
      }

      return start;
   }
}

// For now, we are not returning the "deleted" node.
// TODO: add a free node list and change this function
template<typename T,typename Func>
T* ListRemoveOne(T* start,Func compareFunction){ // Only removes one and returns.
   if(compareFunction(start)){
      return start->next;
   } else {
      T* previous = nullptr;
      FOREACH_LIST(T*,ptr,start){
         if(compareFunction(ptr)){
            previous->next = ptr->next;
         }
         previous = ptr;
      }

      return start;
   }
}

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
T* ListInsertEnd(T* head,T* toAdd){
   T* last = head;
   FOREACH_LIST(T*,ptr,head){
      last = ptr;
   }
   Assert(last->next == nullptr);
   last->next = toAdd;
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
Array<IndexedStruct<T>> IndexArray(Array<T> array,Arena* out){
   Array<IndexedStruct<T>> res = PushArray<IndexedStruct<T>>(out,array.size);

   for(int i = 0; i < array.size; i++){
      CAST_AND_DEREF(res[i],T) = array[i];
      res[i].index = i;
   }

   return res;
}

template<typename T>
Array<T*> ListToArray(T* head,Arena* out){
  DynamicArray<T*> arr = StartArray<T*>(out);

  int i = 0;
  FOREACH_LIST_INDEXED(T*,ptr,head,i){
    *arr.PushElem() = ptr;
  }

  return EndArray(arr);
}

template<typename T>
int Size(ArenaList<T>* list){
  ListedStruct<T>* ptr = list->head;

  int count = 0;
  while(ptr){
    count += 1;
    ptr = ptr->next;
  }
  return count;
}

template<typename T>
ArenaList<T>* PushArenaList(Arena* out){
  ArenaList<T>* res = PushStruct<ArenaList<T>>(out);
  *res = {};
  res->arena = out;
  
  return res;
}

// TODO: Remove this and use PushElem member function
template<typename T>
T* PushListElement(ArenaList<T>* list){
  Arena* out = list->arena;
  ListedStruct<T>* s = PushStruct<ListedStruct<T>>(out);
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
  return PushListElement(this);
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
Array<T> PushArrayFromList(Arena* out,ArenaList<T>* list){
  DynamicArray<T> arr = StartArray<T>(out);

  FOREACH_LIST(ListedStruct<T>*,iter,list->head){
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

template<typename T>
Array<T> PushArrayFromSet(Arena* out,Set<T>* set){
  DynamicArray<T> arr = StartArray<T>(out);

  for(auto pair : set->map){
    T* ptr = arr.PushElem();
    *ptr = pair.first;
  }

  Array<T> res = EndArray(arr);
  return res;
}

template<typename T>
Array<T> PushArrayFromSet(Arena* out,TrieSet<T>* set){
  DynamicArray<T> arr = StartArray<T>(out);

  for(auto pair : set->map){
    T* ptr = arr.PushElem();
    *ptr = pair.first;
  }

  Array<T> res = EndArray(arr);
  return res;
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
Stack<T>* PushStack(Arena* out,int size){
  Stack<T>* stack = PushStruct<Stack<T>>(out);
  *stack = {};

  stack->mem = PushArray<T>(out,size);

  return stack;
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

#include <type_traits>

template<typename T,typename Func>
Array<typename std::result_of<Func(T)>::type> Map(Array<T> array,Arena* out,Func f){
  using ST = typename std::result_of<Func(T)>::type;
  
  DynamicArray<ST> arr = StartArray<ST>(out);
  for(T& t : array){
    *arr.PushElem() = f(t);
  }
  return EndArray(arr);
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

#ifndef INCLUDED_UTILS_HPP
#define INCLUDED_UTILS_HPP

// Utilities that depend on memory

#include "utilsCore.hpp"
#include "memory.hpp"

Optional<Array<String>> GetAllFilesInsideDirectory(String dirPath,Arena* arena);

String EscapeString(String toEscape,char spaceSubstitute,Arena* out);

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
   T* last = nullptr;
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
Array<IndexedStruct<T>> IndexArray(Array<T> array,Arena* arena){
   Array<IndexedStruct<T>> res = PushArray<IndexedStruct<T>>(arena,array.size);

   for(int i = 0; i < array.size; i++){
      CAST_AND_DEREF(res[i],T) = array[i];
      res[i].index = i;
   }

   return res;
}

template<typename T>
Array<T*> ListToArray(T* head,int size,Arena* arena){
   Array<T*> arr = PushArray<T*>(arena,size);

   int i = 0;
   FOREACH_LIST_INDEXED(T*,ptr,head,i){
      arr[i] = ptr;
   }

   return arr;
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
ArenaList<T>* PushArenaList(Arena* arena){
  ArenaList<T>* res = PushStruct<ArenaList<T>>(arena);
  *res = {};
  res->arena = arena;
  
  return res;
}

template<typename T>
T* PushListElement(ArenaList<T>* list){
  Arena* arena = list->arena;
  ListedStruct<T>* s = PushStruct<ListedStruct<T>>(arena);
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
Array<T> PushArrayFromList(Arena* arena,ArenaList<T>* list){
  Byte* mark = MarkArena(arena);

  FOREACH_LIST(ListedStruct<T>*,iter,list->head){
    T* ptr = PushStruct<T>(arena);
    *ptr = iter->elem;
  }

  Array<T> res = PointArray<T>(arena,mark);
  return res;
}

template<typename T>
Array<T> PushArrayFromSet(Arena* arena,Set<T>* set){
  Byte* mark = MarkArena(arena);

  for(auto& pair : set->map){
    T* ptr = PushStruct<T>(arena);
    *ptr = pair.first;
  }

  Array<T> res = PointArray<T>(arena,mark);
  return res;
}

template<typename T,typename P>
Hashmap<T,P>* PushHashmapFromList(ArenaList<Pair<T,P>>* list){
  Arena* arena = list->arena;
  int size = Size(list);
  
  Hashmap<T,P>* map = PushHashmap<T,P>(arena,size);
  
  FOREACH_LIST(auto*,iter,list->head){ // auto = ListedStruct<Pair<T,P>> (auto used because the ',' in Pair<T,P> does not play well with the foreach macro
    map->Insert(iter->elem.first,iter->elem.second);
  }

  return map;
}

template<typename T,typename P>
Array<Pair<T,P>> PushArrayFromList(ArenaList<Pair<T,P>>* list){
  Arena* arena = list->arena;
  int size = Size(list);
  
  Array<Pair<T,P>> arr = PushArray<Pair<T,P>>(arena,size);

  int i = 0;
  FOREACH_LIST_INDEXED(auto*,iter,list->head,i){
    arr[i] = {iter->elem.first,iter->elem.second};
  }

  return arr;
}

template<typename T>
Stack<T>* PushStack(Arena* arena,int size){
  Stack<T>* stack = PushStruct<Stack<T>>(arena);
  *stack = {};

  stack->mem = PushArray<T>(arena,size);

  return stack;
}

#endif // INCLUDED_UTILS_HPP

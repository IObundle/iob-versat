#ifndef INCLUDED_UTILS_HPP
#define INCLUDED_UTILS_HPP

// Utilities that depend on memory

#include "utilsCore.hpp"
#include "memory.hpp"

// A templated type for carrying the index in an array
// A performant design would allocate a separate array because these functions copy data around.
// Use these for prototyping, easing of debugging and stuff
template<typename T>
struct IndexedStruct : public T{
   int index;
};

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

Optional<Array<String>> GetAllFilesInsideDirectory(String dirPath,Arena* arena);

template<typename T>
struct ArenaList{
  ListedStruct<T>* head;
  ListedStruct<T>* tail;
};

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

  return res;
}

template<typename T>
T* PushListElement(Arena* arena,ArenaList<T>* list){
  ListedStruct<T>* s = PushStruct<ListedStruct<T>>(arena);

  if(!list->head){
    list->head = s;
    list->tail = s;
  } else {
    list->tail->next = s;
    list->tail = s;
  }

  return s;
}

template<typename T>
Array<T> PushArrayFromList(Arena* arena,ArenaList<T>* list){
  Byte* mark = MarkArena(arena);

  FOREACH_LIST(T*,iter,list->head){
    T* ptr = PushStruct<T>(arena);
    *ptr = iter;
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

#endif // INCLUDED_UTILS_HPP

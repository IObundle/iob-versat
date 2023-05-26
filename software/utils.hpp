#ifndef INCLUDED_UTILS_HPP
#define INCLUDED_UTILS_HPP

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
   FOREACH_LIST_INDEXED(ptr,head,i){
      arr[i] = ptr;
   }

   return arr;
}

#endif // INCLUDED_UTILS_HPP

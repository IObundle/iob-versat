#pragma once

#include "utilsCore.hpp"
#include "versat.hpp"
#include "merge.hpp"

//#include "repr.hpp"

// TODO: Maybe it would be best if all these functions received function pointers and forward declared everything. Otherwise any small change requires a full recompilation

// TODO: I would like to automatize these procedures. It would be useful to know exactly how many fields a given representation contains so that we could format array representation in a readable format. See for example the representation for an array of InstanceInfo.

String BinaryRepr(int number,Arena* out);

String UniqueRepr(FUInstance* inst,Arena* out); // Returns a representation that uniquely identifies the instance. Not necessarily useful for outputing
String Repr(FUInstance* inst,GraphDotFormat format,Arena* out);
String Repr(PortInstance* inPort,PortInstance* outPort,GraphDotFormat format,Arena* out);
String Repr(FUDeclaration* decl,Arena* out);
String Repr(FUDeclaration** decl,Arena* out);
String Repr(PortInstance* port,GraphDotFormat format,Arena* memory);
String Repr(MergeEdge* node,GraphDotFormat format,Arena* memory);
String Repr(PortEdge* node,GraphDotFormat format,Arena* memory);
String Repr(MappingNode* node,Arena* memory);
String Repr(StaticId* id,Arena* out);
String Repr(StaticData* data,Arena* out);
String Repr(PortNode* portNode,Arena* out);
String Repr(EdgeNode* node,Arena* out);
String Repr(Wire* wire,Arena* out);
String Repr(InstanceInfo* info,Arena* out);
String Repr(String* str,Arena* out);
String Repr(int* i,Arena* out);
String Repr(long int* i,Arena* out);
String Repr(bool* b,Arena* out);
String Repr(TypeStructInfo* info,Arena* out);
String Repr(InstanceNode* node,Arena* out);

String PushIntTableRepresentation(Arena* out,Array<int> values,int digitSize = 0);

template<typename T>
String Repr(Optional<T>* opt,Arena* out){
  if(opt->has_value()){
    return Repr(&opt->value(),out);
  } else {
    return PushString(out,"-");
  }
}

template<typename T>
String Repr(Array<T>* arr,Arena* out){
  if(arr == nullptr){
    return PushString(out,"()");
  }

  Byte* mark = MarkArena(out);
  PushString(out,"(");
  bool first = true;
  for(T& t : *arr){
    if(!first) PushString(out,",");
    Repr(&t,out);
    first = false;
  }
  PushString(out,")");

  return PointArena(out,mark);
}

template<typename T,typename P>
String Repr(Pair<T,P>* pair,Arena* out){
  Byte* mark = MarkArena(out);
  PushString(out,"<");
  Repr(&pair->first,out);
  PushString(out,":");
  Repr(&pair->second,out);
  PushString(out,">");
  return PointArena(out,mark);
}

String Repr(Optional<int>* opt,Arena* out);

// TODO: Rework these to get a array of fields and then format them accordingly. Check PrintAll(Array<InstanceInfo>...) impl.
template<typename T>
void PrintAll(Array<T>* arr,Arena* temp){
  for(T& elem : arr){
    BLOCK_REGION(temp);

    String repr = Repr(&elem,temp);
    printf("%.*s\n",UNPACK_SS(repr));
  }
}

// TODO: Rework these to get a array of fields and then format them accordingly. Check PrintAll(Array<InstanceInfo>...) impl.
template<typename T>
void PrintAll(T* listLike,Arena* temp){
  FOREACH_LIST(T*,iter,listLike){
    BLOCK_REGION(temp);

    String repr = Repr(*iter,temp);
    printf("  %.*s\n",UNPACK_SS(repr));
  }
}

template<typename T,typename P>
Array<String> ReprAll(Hashmap<T,P>* map,Arena* out,Arena* temp){
  BLOCK_REGION(temp);

  int maxLeftSize = 0;
  int maxRightSize = 0;

  Array<Pair<String,String>> allReprs = PushArray<Pair<String,String>>(temp,map->nodesUsed);
  int index = 0;
  for(Pair<T,P>& elem : map){
    String first = Repr(&elem.first,temp);
    String second = Repr(&elem.second,temp);
    allReprs[index] = {first,second};
    index += 1;

    maxLeftSize = std::max(maxLeftSize,first.size);
    maxRightSize = std::max(maxRightSize,second.size);
  }

  Array<String> repr = PushArray<String>(out,allReprs.size);

  int i = 0;
  for(Pair<String,String> p : allReprs){
    repr[i] = PushString(out,"%*.*s - %*.*s\n",maxLeftSize,UNPACK_SS(p.first),maxRightSize,UNPACK_SS(p.second));
  }

  return repr;
}

template<typename T,typename P>
void PrintAll(Hashmap<T,P>* map,Arena* temp){
  BLOCK_REGION(temp);

  int maxLeftSize = 0;
  int maxRightSize = 0;

  Array<Pair<String,String>> allReprs = PushArray<Pair<String,String>>(temp,map->nodesUsed);
  int index = 0;
  for(Pair<T,P>& elem : map){
    String first = Repr(&elem.first,temp);
    String second = Repr(&elem.second,temp);
    allReprs[index] = {first,second};
    index += 1;

    maxLeftSize = std::max(maxLeftSize,first.size);
    maxRightSize = std::max(maxRightSize,second.size);
  }

  for(Pair<String,String> p : allReprs){
    printf("%*.*s - %*.*s\n",maxLeftSize,UNPACK_SS(p.first),maxRightSize,UNPACK_SS(p.second));
  }
}

void PrintAll(FILE* file,Array<String> fields,Array<Array<String>> content,Arena* temp);
void PrintAll(Array<String> fields,Array<Array<String>> content,Arena* temp);

#include "repr.hpp" // Implements the GetFields and GetRepr functions

template<typename T>
Array<Array<String>> ReprAll(Array<T> arr,Arena* out){
  int size = arr.size;
  
  Array<Array<String>> strings = PushArray<Array<String>>(out,size);
  for(int i = 0; i < arr.size; i++){
    strings[i] = GetRepr(&arr[i],out);
  }

  return strings;
}

template<typename T>
void PrintAll(FILE* file,Array<T> arr,Arena* temp){
  BLOCK_REGION(temp);

  int size = arr.size;
  Array<String> fields = GetFields((T){},temp);
  Array<Array<String>> strings = ReprAll(arr,temp);

  PrintAll(file,fields,strings,temp);
}

template<typename T>
void PrintAll(Array<T> arr,Arena* temp){
  BLOCK_REGION(temp);

  int size = arr.size;
  Array<String> fields = GetFields((T){},temp);
  Array<Array<String>> strings = ReprAll(arr,temp);

  PrintAll(fields,strings,temp);
}

static inline void PrintAll(Array<int> arr,Arena* temp){
  BLOCK_REGION(temp);
  int size = arr.size;
  Array<String> values = PushArray<String>(temp,size);

  int maxSize = 0;
  for(int i = 0; i < values.size; i++){
    values[i] = PushString(temp,"%d",arr[i]);
    maxSize = std::max(maxSize,values[i].size);
  }

  for(int i = 0; i < values.size; i++){
    printf("%*.*s\n",maxSize,UNPACK_SS(values[i]));
  }
}

#ifndef INCLUDED_TEXTUAL_REPRESENTATION
#define INCLUDED_TEXTUAL_REPRESENTATION

#include "utilsCore.hpp"
#include "versat.hpp"
#include "merge.hpp"

//#include "repr.hpp"

// TODO: Maybe it would be best if all these functions received function pointers and forward declared everything. Otherwise any small change requires a full recompilation

// TODO: I would like to automatize these procedures. It would be useful to know exactly how many fields a given representation contains so that we could format array representation in a readable format. See for example the representation for an array of InstanceInfo.

String UniqueRepr(FUInstance* inst,Arena* arena); // Returns a representation that uniquely identifies the instance. Not necessarily useful for outputing
String Repr(FUInstance* inst,GraphDotFormat format,Arena* arena);
String Repr(PortInstance* in,PortInstance* out,GraphDotFormat format,Arena* arena);
String Repr(FUDeclaration* decl,Arena* arena);
String Repr(FUDeclaration** decl,Arena* arena);
String Repr(PortInstance* port,GraphDotFormat format,Arena* memory);
String Repr(MergeEdge* node,GraphDotFormat format,Arena* memory);
String Repr(PortEdge* node,GraphDotFormat format,Arena* memory);
String Repr(MappingNode* node,Arena* memory);
String Repr(StaticId* id,Arena* arena);
String Repr(StaticData* data,Arena* arena);
String Repr(PortNode* portNode,Arena* arena);
String Repr(EdgeNode* node,Arena* arena);
String Repr(Wire* wire,Arena* arena);
String Repr(InstanceInfo* info,Arena* arena);
String Repr(String* str,Arena* arena);
String Repr(int* i,Arena* arena);
String Repr(long int* i,Arena* arena);
String Repr(bool* b,Arena* arena);
String Repr(SizedConfig* config,Arena* arena);
String Repr(TypeStructInfo* info,Arena* arena);
String Repr(InstanceNode* node,Arena* arena);

String PushIntTableRepresentation(Arena* arena,Array<int> values,int digitSize = 0);

template<typename T>
String Repr(Optional<T>* opt,Arena* arena){
  if(opt->has_value()){
    return Repr(&opt->value(),arena);
  } else {
    return PushString(arena,"-");
  }
}

template<typename T,typename P>
String Repr(Pair<T,P>* pair,Arena* arena){
  Byte* mark = MarkArena(arena);
  PushString(arena,"<");
  Repr(&pair->first,arena);
  PushString(arena,":");
  Repr(&pair->second,arena);
  PushString(arena,">");
  return PointArena(arena,mark);
}

String Repr(Optional<int>* opt,Arena* arena);

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

void PrintAll(Array<String> fields,Array<Array<String>> content,Arena* temp);

// Would like to make this PrintAll automatic, would be helpful in the future. 
// TODO: Have two types of functions, Repr that returns a readable string and GetRepr that returns an array of strings for each field
//Array<String> GetRepr(InstanceInfo info,Arena* arena);
//void PrintAll(Array<InstanceInfo> arr,Arena* temp);

#include "repr.hpp" // Implements the GetFields and GetRepr functions

template<typename T>
void PrintAll(Array<T> arr,Arena* temp){
  BLOCK_REGION(temp);

  int size = arr.size;
  Array<String> fields = GetFields((T){},temp);
  Array<Array<String>> strings = PushArray<Array<String>>(temp,size);

  for(int i = 0; i < arr.size; i++){
    strings[i] = GetRepr(&arr[i],temp);
  }

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

#endif // INCLUDED_TEXTUAL_REPRESENTATION

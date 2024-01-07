#ifndef INCLUDED_TEXTUAL_REPRESENTATION
#define INCLUDED_TEXTUAL_REPRESENTATION

#include "versat.hpp"
#include "merge.hpp"

//#include "repr.hpp"

// TODO: Maybe it would be best if all these functions received function pointers and forward declared everything. Otherwise any small change requires a full recompilation

// TODO: I would like to automatize these procedures. It would be useful to know exactly how many fields a given representation contains so that we could format array representation in a readable format. See for example the representation for an array of InstanceInfo.

String UniqueRepr(FUInstance* inst,Arena* arena); // Returns a representation that uniquely identifies the instance. Not necessarily useful for outputing
String Repr(FUInstance* inst,GraphDotFormat format,Arena* arena);
String Repr(PortInstance in,PortInstance out,GraphDotFormat format,Arena* arena);
String Repr(FUDeclaration* decl,Arena* arena);
String Repr(PortInstance port,GraphDotFormat format,Arena* memory);
String Repr(MergeEdge node,GraphDotFormat format,Arena* memory);
String Repr(PortEdge node,GraphDotFormat format,Arena* memory);
String Repr(MappingNode node,Arena* memory);
String Repr(StaticId id,Arena* arena);
String Repr(StaticData data,Arena* arena);
String Repr(PortNode portNode,Arena* arena);
String Repr(EdgeNode node,Arena* arena);
String Repr(Wire wire,Arena* arena);
String Repr(InstanceInfo info,Arena* arena);
String Repr(String str,Arena* arena);
String Repr(int i,Arena* arena);
String Repr(SizedConfig config,Arena* arena);

String PushIntTableRepresentation(Arena* arena,Array<int> values,int digitSize = 0);

template<typename T>
String Repr(Optional<T> opt,Arena* arena){
  if(opt.has_value()){
    return Repr(opt.value(),arena);
  } else {
    return PushString(arena,"(opt empty)");
  }
}

String Repr(Optional<int> opt,Arena* arena);

// TODO: Rework these to get a array of fields and then format them accordingly. Check PrintAll(Array<InstanceInfo>...) impl.
template<typename T>
void PrintAll(Array<T> arr,Arena* temp){
  for(T& elem : arr){
    BLOCK_REGION(temp);

    String repr = Repr(elem,temp);
    printf("%.*s\n",UNPACK_SS(repr));
  }
}

template<typename T,typename P>
void PrintAll(Hashmap<T,P>* map,Arena* temp){
  for(Pair<T,P>& elem : map){
    BLOCK_REGION(temp);

    String first = Repr(elem.first,temp);
    String second = Repr(elem.second,temp);
    printf("%.*s:%.*s\n",UNPACK_SS(first),UNPACK_SS(second));
  }
}

// Would like to make this PrintAll automatic, would be helpful in the future. 
// TODO: Have two types of functions, Repr that returns a readable string and GetRepr that returns an array of strings for each field
Array<String> GetRepr(InstanceInfo info,Arena* arena);
void PrintAll(Array<InstanceInfo> arr,Arena* temp);

#endif // INCLUDED_TEXTUAL_REPRESENTATION

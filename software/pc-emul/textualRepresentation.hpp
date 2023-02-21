#ifndef INCLUDED_TEXTUAL_REPRESENTATION
#define INCLUDED_TEXTUAL_REPRESENTATION

#include "versatPrivate.hpp"

String UniqueRepr(FUInstance* inst,Arena* arena); // Returns a representation that uniquely identifies the instance. Not necessarily useful for outputing
String Repr(FUInstance* inst,GraphDotFormat format,Arena* arena);
String Repr(PortInstance in,PortInstance out,GraphDotFormat format,Arena* arena);
String Repr(FUDeclaration* decl,Arena* arena);
String Repr(PortInstance port,GraphDotFormat format,Arena* memory);
String Repr(MergeEdge node,GraphDotFormat format,Arena* memory);
String Repr(PortEdge node,GraphDotFormat format,Arena* memory);
String Repr(MappingNode node,Arena* memory);

String PushIntTableRepresentation(Arena* arena,Array<int> values,int digitSize = 0);

#endif // INCLUDED_TEXTUAL_REPRESENTATION

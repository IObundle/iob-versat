#ifndef INCLUDED_TEXTUAL_REPRESENTATION
#define INCLUDED_TEXTUAL_REPRESENTATION

#include "versatPrivate.hpp"

SizedString UniqueRepr(FUInstance* inst,Arena* arena); // Returns a representation that uniquely identifies the instance. Not necessarily useful for outputing
SizedString Repr(FUInstance* inst,GraphDotFormat format,Arena* arena);
SizedString Repr(PortInstance in,PortInstance out,GraphDotFormat format,Arena* arena);
SizedString Repr(FUDeclaration* decl,Arena* arena);
SizedString Repr(PortInstance port,GraphDotFormat format,Arena* memory);
SizedString Repr(MappingNode node,Arena* memory);

#endif // INCLUDED_TEXTUAL_REPRESENTATION

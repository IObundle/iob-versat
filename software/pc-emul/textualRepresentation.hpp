#ifndef INCLUDED_TEXTUAL_REPRESENTATION
#define INCLUDED_TEXTUAL_REPRESENTATION

#include "versatPrivate.hpp"

SizedString Repr(FUInstance* inst,Arena* arena,bool printDelay = false);
SizedString Repr(FUDeclaration* decl,Arena* arena);
SizedString Repr(PortInstance port,Arena* memory);
SizedString Repr(MappingNode* node,Arena* memory);

#endif // INCLUDED_TEXTUAL_REPRESENTATION

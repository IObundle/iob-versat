#pragma once

#include "merge.hpp"
#include "codeGeneration.hpp"

// TODO: Maybe it would be best if all these functions received function pointers and forward declared everything. Otherwise any small change requires a full recompilation

String UniqueRepr(FUInstance* inst,Arena* out); // Returns a representation that uniquely identifies the instance. Not necessarily useful for outputing

String Repr(FUInstance* inst,GraphDotFormat format,Arena* out);
String Repr(PortInstance* inPort,PortInstance* outPort,GraphDotFormat format,Arena* out);
String Repr(FUDeclaration* decl,Arena* out);
String Repr(PortInstance* port,GraphDotFormat format,Arena* out);
String Repr(MergeEdge* node,GraphDotFormat format,Arena* out);
String Repr(Edge* node,GraphDotFormat format,Arena* out);
String Repr(MappingNode* node,Arena* out);

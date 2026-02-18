#pragma once

#include "versat.hpp"

struct MergeEdge;
struct Edge;
struct MappingNode;

String Repr(FUInstance* inst,GraphDotFormat format,Arena* out);
String Repr(PortInstance* inPort,PortInstance* outPort,GraphDotFormat format,Arena* out);
String Repr(FUDeclaration* decl,Arena* out);
String Repr(PortInstance* port,GraphDotFormat format,Arena* out);
String Repr(MergeEdge* node,GraphDotFormat format,Arena* out);
String Repr(Edge* node,GraphDotFormat format,Arena* out);
String Repr(MappingNode* node,Arena* out);

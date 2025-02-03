#include "textualRepresentation.hpp"

#include "declaration.hpp"

String UniqueRepr(FUInstance* inst,Arena* out){
  FUDeclaration* decl = inst->declaration;
  String str = PushString(out,"%.*s_%.*s_%d",UNPACK_SS(decl->name),UNPACK_SS(inst->name),inst->id);
  return str;
}

String Repr(FUInstance* inst,GraphDotFormat format,Arena* out){
  auto mark = StartString(out);

  FUInstance* instance = (FUInstance*) inst;

  bool expl  = format & GRAPH_DOT_FORMAT_EXPLICIT;
  bool name  = format & GRAPH_DOT_FORMAT_NAME;
  bool type  = format & GRAPH_DOT_FORMAT_TYPE;
  bool id    = format & GRAPH_DOT_FORMAT_ID;
  bool delay = format & GRAPH_DOT_FORMAT_DELAY;

  bool buffer = (HasVariableDelay(inst->declaration) || inst->declaration == BasicDeclaration::fixedBuffer);

  if(expl && name){
    PushString(out,"Name:");
  }
  if(name){
    PushString(out,"%.*s",UNPACK_SS(inst->name));
  }
  if(expl && type){
    PushString(out,"\\nType:");
  }
  if(type){
    PushString(out,"%.*s",UNPACK_SS(inst->declaration->name));
  }
  if(expl && id){
    PushString(out,"\\nId:");
  }
  if(id){
    PushString(out,"%d",inst->id);
  }
  if(expl && delay){
    if(buffer){
      PushString(out,"\\nBuffer:");
    } else {
      PushString(out,"\\nDelay:");
    }
  }
  if(delay){
    if(buffer){
      PushString(out,"%d",instance->bufferAmount);
    } else {
      PushString(out,"0");
    }
  }

  String res = EndString(mark);
  return res;
}

String Repr(FUDeclaration* decl,Arena* out){
  if(decl == nullptr){
    return PushString(out,"(null)");
  }
  String res = PushString(out,"%.*s",UNPACK_SS(decl->name));
  return res;
}

String Repr(PortInstance* inPort,PortInstance* outPort,GraphDotFormat format,Arena* out){
  auto mark = StartString(out);

  bool expl = format & GRAPH_DOT_FORMAT_EXPLICIT;
  bool lat  = format & GRAPH_DOT_FORMAT_LATENCY;

  Repr(inPort,format,out);

  if(expl && lat){
    PushString(out,"\\nLat:");
  }
  if(lat){
    PushString(out,"%d",inPort->inst->declaration->GetOutputLatencies()[inPort->port]);
  }

  PushString(out,"\\n->\\n");
  Repr(outPort,format,out);
  if(expl && lat){
    PushString(out,"\\nLat:");
  }
  if(lat){
    PushString(out,"%d",outPort->inst->declaration->GetInputDelays()[outPort->port]);
  }

  String res = EndString(mark);
  return res;
}

String Repr(PortInstance* port,GraphDotFormat format,Arena* out){
  auto mark = StartString(out);

  Repr(port->inst,GRAPH_DOT_FORMAT_NAME,out);

  bool expl = format & GRAPH_DOT_FORMAT_EXPLICIT;

  if(expl){
    PushString(out,"_Port:");
  }
  PushString(out,":%d",port->port);

  String res = EndString(mark);
  return res;
}

String Repr(Edge* edge,GraphDotFormat format,Arena* out){
  auto mark = StartString(out);

  format |= GRAPH_DOT_FORMAT_NAME;

  Repr(&edge->units[0],format,out);
  PushString(out," -- ");
  Repr(&edge->units[1],format,out);

  String res = EndString(mark);
  return res;
}

String Repr(MergeEdge* node,GraphDotFormat format,Arena* out){
  auto mark = StartString(out);

  format |= GRAPH_DOT_FORMAT_NAME;

  Repr(node->instances[0],format,out);
  PushString(out," -- ");
  Repr(node->instances[1],format,out);

  String name = EndString(mark);

  return name;
}

String Repr(MappingNode* node,Arena* out){
  String name = {};
  GraphDotFormat format = GRAPH_DOT_FORMAT_NAME;

  if(node->type == MappingNode::NODE){
    name = Repr(&node->nodes,format,out);
  } else if(node->type == MappingNode::EDGE){
    Edge e0 = node->edges[0];
    Edge e1 = node->edges[1];

    auto mark = StartString(out);
    Repr(&e0,format,out);
    PushString(out," // ");
    Repr(&e1,format,out);
    name = EndString(mark);
  } else {
    NOT_IMPLEMENTED("Implement as needed");
  }

  return name;
}

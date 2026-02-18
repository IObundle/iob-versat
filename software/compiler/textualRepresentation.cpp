#include "textualRepresentation.hpp"

#include "merge.hpp"
#include "declaration.hpp"

String Repr(FUInstance* inst,GraphDotFormat format,Arena* out){
  TEMP_REGION(temp,out);
  auto builder = StartString(temp);

  FUInstance* instance = (FUInstance*) inst;

  bool expl  = format & GraphDotFormat_EXPLICIT;
  bool name  = format & GraphDotFormat_NAME;
  bool type  = format & GraphDotFormat_TYPE;
  bool id    = format & GraphDotFormat_ID;
  bool delay = format & GraphDotFormat_DELAY;

  bool buffer = (inst->declaration == BasicDeclaration::variableBuffer || inst->declaration == BasicDeclaration::fixedBuffer);

  if(expl && name){
    builder->PushString("Name:");
  }
  if(name){
    builder->PushString("%.*s",UN(inst->name));
  }
  if(expl && type){
    builder->PushString("\\nType:");
  }
  if(type){
    builder->PushString("%.*s",UN(inst->declaration->name));
  }
  if(expl && id){
    builder->PushString("\\nId:");
  }
  if(id){
    builder->PushString("%d",inst->id);
  }
  if(expl && delay){
    if(buffer){
      builder->PushString("\\nBuffer:");
    } else {
      builder->PushString("\\nDelay:");
    }
  }
  if(delay){
    if(buffer){
      builder->PushString("%d",instance->bufferAmount);
    } else {
      builder->PushString("0");
    }
  }

  String res = EndString(out,builder);
  return res;
}

String Repr(FUDeclaration* decl,Arena* out){
  if(decl == nullptr){
    return PushString(out,"(null)");
  }
  String res = PushString(out,"%.*s",UN(decl->name));
  return res;
}

String Repr(PortInstance* inPort,PortInstance* outPort,GraphDotFormat format,Arena* out){
  TEMP_REGION(temp,out);
  auto builder = StartString(temp);

  bool expl = format & GraphDotFormat_EXPLICIT;
  bool lat  = format & GraphDotFormat_LATENCY;

  builder->PushString(Repr(inPort,format,temp));

  if(expl && lat){
    builder->PushString("\\nLat:");
  }
  if(lat){
    builder->PushString("%d",inPort->inst->declaration->GetOutputLatencies()[inPort->port]);
  }

  builder->PushString("\\n->\\n");
  builder->PushString(Repr(outPort,format,temp));
  if(expl && lat){
    builder->PushString("\\nLat:");
  }
  if(lat){
    builder->PushString("%d",outPort->inst->declaration->GetInputDelays()[outPort->port]);
  }

  String res = EndString(out,builder);
  return res;
}

String Repr(PortInstance* port,GraphDotFormat format,Arena* out){
  TEMP_REGION(temp,out);
  auto builder = StartString(temp);

  builder->PushString(Repr(port->inst,GraphDotFormat_NAME,temp));

  bool expl = format & GraphDotFormat_EXPLICIT;

  if(expl){
    builder->PushString("_Port:");
  }
  builder->PushString(":%d",port->port);

  String res = EndString(out,builder);
  return res;
}

String Repr(Edge* edge,GraphDotFormat format,Arena* out){
  TEMP_REGION(temp,out);
  auto builder = StartString(temp);

  format = (GraphDotFormat) ((int) format | (int)GraphDotFormat_NAME);

  builder->PushString(Repr(&edge->units[0],format,temp));
  builder->PushString(" -- ");
  builder->PushString(Repr(&edge->units[1],format,temp));

  String res = EndString(out,builder);
  return res;
}

String Repr(MergeEdge* node,GraphDotFormat format,Arena* out){
  TEMP_REGION(temp,out);
  auto builder = StartString(temp);

  format = (GraphDotFormat) ((int) format | (int)GraphDotFormat_NAME);

  builder->PushString(Repr(node->instances[0],format,temp));
  builder->PushString(" -- ");
  builder->PushString(Repr(node->instances[1],format,temp));

  String name = EndString(out,builder);

  return name;
}

String Repr(MappingNode* node,Arena* out){
  String name = {};
  GraphDotFormat format = GraphDotFormat_NAME;

  if(node->type == MappingNode::NODE){
    name = Repr(&node->nodes,format,out);
  } else if(node->type == MappingNode::EDGE){
    TEMP_REGION(temp,out);
    auto builder = StartString(temp);

    Edge e0 = node->edges[0];
    Edge e1 = node->edges[1];

    builder->PushString(Repr(&e0,format,temp));
    builder->PushString(" // ");
    builder->PushString(Repr(&e1,format,temp));
    name = EndString(out,builder);
  } else {
    NOT_IMPLEMENTED("Implement as needed");
  }

  return name;
}

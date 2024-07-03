#include "textualRepresentation.hpp"
#include "memory.hpp"
#include "utils.hpp"
#include "utilsCore.hpp"

// For now hardcoded for 32 bits.
String BinaryRepr(int number,int bitsize,Arena* out){
  Byte* buffer = PushBytes(out,bitsize);

  for(int i = 0; i < bitsize; i++){
    buffer[i] = GET_BIT(number,31 - i) ? '1' : '0';
  }
  
  String res = {};
  res.data = (char*) buffer;
  res.size = bitsize;
  return res;
}

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

  bool buffer = (inst->declaration == BasicDeclaration::buffer || inst->declaration == BasicDeclaration::fixedBuffer);

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
      //PushString(out,"%d",inst->baseDelay); //TODO: Broken 
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

String Repr(FUDeclaration** decl,Arena* out){
  return Repr(*decl,out);
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
    PushString(out,"%d",inPort->inst->declaration->baseConfig.outputLatencies[inPort->port]);
  }

  PushString(out,"\\n->\\n");
  Repr(outPort,format,out);
  if(expl && lat){
    PushString(out,"\\nLat:");
  }
  if(lat){
    PushString(out,"%d",outPort->inst->declaration->baseConfig.inputDelays[outPort->port]);
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

String Repr(PortEdge* edge,GraphDotFormat format,Arena* out){
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
    PortEdge e0 = node->edges[0];
    PortEdge e1 = node->edges[1];

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

String PushIntTableRepresentation(Arena* out,Array<int> values,int digitSize){
  int maxDigitSize = 0;

  for(int val : values){
    maxDigitSize = std::max(maxDigitSize,NumberDigitsRepresentation(val));
  }

  if(digitSize > maxDigitSize){
    maxDigitSize = digitSize;
  }

  int valPerLine = 80 / (maxDigitSize + 1); // +1 for spaces
  auto mark = StartString(out);
  for(int i = 0; i < values.size; i++){
    if((i % valPerLine == 0) && i != 0){
      PushString(out,"\n");
    }

    PushString(out,"%.*d ",maxDigitSize,values[i]);
  }

  String res = EndString(mark);
  return res;
}

String Repr(StaticId* id,Arena* out){
  auto mark = StartString(out);

  Repr(id->parent,out);
  PushString(out," %.*s",UNPACK_SS(id->name));

  String res = EndString(mark);
  return res;
}

String Repr(PortNode* portNode,Arena* out){
  auto mark = StartString(out);

  PushString(out,"%.*s",UNPACK_SS(portNode->node->inst->name));
  PushString(out,":%d",portNode->port);

  String res = EndString(mark);
  return res;
}

String Repr(EdgeNode* node,Arena* out){
  auto mark = StartString(out);

  Repr(&node->node0,out);
  PushString(out," -> ");
  Repr(&node->node1,out);

  String res = EndString(mark);
  return res;
}

String Repr(Wire* wire,Arena* out){
  auto mark = StartString(out);

  PushString(out,wire->name);
  PushString(out,":%d",wire->bitSize);

  String res = EndString(mark);
  return res;
}

String Repr(int* i,Arena* out){
  if(i == nullptr){
    return PushString(out,STRING("(null)"));
  }

  return PushString(out,"%d",*i);
}

String Repr(long int* i,Arena* out){
  if(i == nullptr){
    return PushString(out,STRING("(null)"));
  }
  return PushString(out,"%ld",*i);
}

String Repr(bool* b,Arena* out){
  if(b == nullptr){
    return PushString(out,STRING("(null)"));
  }

  return PushString(out,"%c",*b ? '1' : '0');
}

String Repr(char** ch,Arena* out){
  if(*ch == nullptr){
    return PushString(out,STRING("(null)"));
  }
  return PushEscapedString(out,STRING(*ch),' ');
}

String Repr(String* str,Arena* out){
  return PushString(out,*str);
}

String Repr(ExternalMemoryInterfaceTemplate<int>* ext, Arena* out){
  if(ext == nullptr){
    return PushString(out,STRING("(null)"));
  }

  switch(ext->type){
  case TWO_P:{
    return PushString(out,STRING("TWO_P"));
  } break;
  case DP:{
    return PushString(out,STRING("TWO_P"));
  } break;
  }

  NOT_POSSIBLE("Unreachable");
  return {};
}

#if 0
String Repr(InstanceInfo* info,Arena* out){
  auto mark = StartString(out);

  PushString(out,"[");
  Repr(info->decl,out);
  PushString(out,"]");
  PushString(out,"-");
  PushString(out,info->fullName);
  PushString(out,":");
  Repr(&info->configPos,out);
  PushString(out,":");
  Repr(&info->statePos,out);
  PushString(out,":");
  Repr(&info->memMapped,out);

  String res = PointArena(out,mark);
  return res;
}
#endif

String Repr(TypeStructInfoElement* elem,Arena* out){
  NOT_IMPLEMENTED("Implement if needed");
  return {};
  //return PushString(out,"[%.*s]%.*s",UNPACK_SS(elem->type),UNPACK_SS(elem->name));
}

String Repr(TypeStructInfo* info,Arena* out){
  auto mark = StartString(out);
  Repr(&info->name,out);
  PushString(out,"\n");
  for(TypeStructInfoElement& elem : info->entries){
    Repr(&elem,out);
    PushString(out,"\n");
  }
  return EndString(mark);
}

String Repr(InstanceNode* node,Arena* out){
  return Repr(node->inst,GRAPH_DOT_FORMAT_NAME,out);
}

String Repr(Opt<int>* opt,Arena* out){
  if(opt->has_value()){
    return Repr(&opt->value(),out);
  } else {
    return PushString(out,"-");
  }
}

void PrintAll(FILE* file,Array<String> fields,Array<Array<String>> content,Arena* temp){
  BLOCK_REGION(temp);

  int fieldSize = fields.size;
  Array<int> maxSize = PushArray<int>(temp,fieldSize);
  
  Memset(maxSize,0);

  // Max size of fields name
  for(int i = 0; i < fields.size; i++){
    maxSize[i] = std::max(maxSize[i],fields[i].size);
  }
  // Max size of content
  for(int i = 0; i < content.size; i++){
    for(int ii = 0; ii < fieldSize; ii++){
      maxSize[ii] = std::max(maxSize[ii],content[i][ii].size);
    }
  }

  for(int i = 0; i < fieldSize; i++){
    fprintf(file,"%*.*s ",maxSize[i],UNPACK_SS(fields[i]));
  }
  fprintf(file,"\n");
  for(int i = 0; i < content.size; i++){
    for(int ii = 0; ii < fieldSize; ii++){
      fprintf(file,"%*.*s ",maxSize[ii],UNPACK_SS(content[i][ii]));
    }
    fprintf(file,"\n");
  }
  for(int i = 0; i < fieldSize; i++){
    fprintf(file,"%*.*s ",maxSize[i],UNPACK_SS(fields[i]));
  }
  fprintf(file,"\n");
}

void PrintAll(Array<String> fields,Array<Array<String>> content,Arena* temp){
  PrintAll(stdout,fields,content,temp);
}



#include "textualRepresentation.hpp"

#include "parser.hpp"

String UniqueRepr(FUInstance* inst,Arena* arena){
   FUDeclaration* decl = inst->declaration;
   String str = PushString(arena,"%.*s_%.*s_%d",UNPACK_SS(decl->name),UNPACK_SS(inst->name),inst->id);
   return str;
}

String Repr(FUInstance* inst,GraphDotFormat format,Arena* arena){
   Byte* mark = MarkArena(arena);

   ComplexFUInstance* instance = (ComplexFUInstance*) inst;

   bool expl  = format & GRAPH_DOT_FORMAT_EXPLICIT;
   bool name  = format & GRAPH_DOT_FORMAT_NAME;
   bool type  = format & GRAPH_DOT_FORMAT_TYPE;
   bool id    = format & GRAPH_DOT_FORMAT_ID;
   bool delay = format & GRAPH_DOT_FORMAT_DELAY;

   bool buffer = (inst->declaration == BasicDeclaration::buffer || inst->declaration == BasicDeclaration::fixedBuffer);

   if(expl && name){
      PushString(arena,"Name:");
   }
   if(name){
      PushString(arena,"%.*s",UNPACK_SS(inst->name));
   }
   if(expl && type){
      PushString(arena,"\\nType:");
   }
   if(type){
      PushString(arena,"%.*s",UNPACK_SS(inst->declaration->name));
   }
   if(expl && id){
      PushString(arena,"\\nId:");
   }
   if(id){
      PushString(arena,"%d",inst->id);
   }
   if(expl && delay){
      if(buffer){
         PushString(arena,"\\nBuffer:");
      } else {
         PushString(arena,"\\nDelay:");
      }
   }
   if(delay){
      if(buffer){
         PushString(arena,"%d",instance->bufferAmount);
      } else {
         PushString(arena,"%d",inst->baseDelay);
      }
   }

   String res = PointArena(arena,mark);
   return res;
}

String Repr(FUDeclaration* decl,Arena* arena){
   String res = PushString(arena,"%.*s",UNPACK_SS(decl->name));
   return res;
}

String Repr(PortInstance in,PortInstance out,GraphDotFormat format,Arena* arena){
   Byte* mark = MarkArena(arena);

   bool expl = format & GRAPH_DOT_FORMAT_EXPLICIT;
   bool lat  = format & GRAPH_DOT_FORMAT_LATENCY;

   Repr(in,format,arena);

   if(expl && lat){
      PushString(arena,"\\nLat:");
   }
   if(lat){
      PushString(arena,"%d",in.inst->declaration->outputLatencies[in.port]);
   }

   PushString(arena,"\\n->\\n");
   Repr(out,format,arena);
   if(expl && lat){
      PushString(arena,"\\nLat:");
   }
   if(lat){
      PushString(arena,"%d",out.inst->declaration->inputDelays[out.port]);
   }

   String res = PointArena(arena,mark);
   return res;
}

String Repr(PortInstance port,GraphDotFormat format,Arena* arena){
   Byte* mark = MarkArena(arena);

   Repr(port.inst,GRAPH_DOT_FORMAT_NAME,arena);

   bool expl = format & GRAPH_DOT_FORMAT_EXPLICIT;

   if(expl){
      PushString(arena,"_Port:");
   }
   PushString(arena,":%d",port.port);

   String res = PointArena(arena,mark);
   return res;
}

String Repr(PortEdge edge,GraphDotFormat format,Arena* arena){
   Byte* mark = MarkArena(arena);

   format |= GRAPH_DOT_FORMAT_NAME;

   Repr(edge.units[0],format,arena);
   PushString(arena," -- ");
   Repr(edge.units[1],format,arena);

   String res = PointArena(arena,mark);
   return res;
}

String Repr(MergeEdge node,GraphDotFormat format,Arena* arena){
   Byte* mark = MarkArena(arena);

   format |= GRAPH_DOT_FORMAT_NAME;

   Repr(node.instances[0],format,arena);
   PushString(arena," -- ");
   Repr(node.instances[1],format,arena);

   String name = PointArena(arena,mark);

   return name;
}

String Repr(MappingNode node,Arena* arena){
   String name = {};
   GraphDotFormat format = GRAPH_DOT_FORMAT_NAME;

   if(node.type == MappingNode::NODE){
      name = Repr(node.nodes,format,arena);
   } else if(node.type == MappingNode::EDGE){
      PortEdge e0 = node.edges[0];
      PortEdge e1 = node.edges[1];

      Byte* mark = MarkArena(arena);
      Repr(e0,format,arena);
      PushString(arena," // ");
      Repr(e1,format,arena);
      name = PointArena(arena,mark);
   } else {
      NOT_IMPLEMENTED;
   }

   return name;
}

String PushIntTableRepresentation(Arena* arena,Array<int> values,int digitSize){
   int maxDigitSize = 0;

   for(int val : values){
      maxDigitSize = std::max(maxDigitSize,NumberDigitsRepresentation(val));
   }

   if(digitSize > maxDigitSize){
      maxDigitSize = digitSize;
   }

   int valPerLine = 80 / (maxDigitSize + 1); // +1 for spaces
   Byte* mark = MarkArena(arena);
   for(int i = 0; i < values.size; i++){
      if((i % valPerLine == 0) && i != 0){
         PushString(arena,"\n");
      }

      PushString(arena,"%.*d ",maxDigitSize,values[i]);
   }

   String res = PointArena(arena,mark);
   return res;
}

String Repr(StaticId id,Arena* arena){
   Byte* mark = MarkArena(arena);

   Repr(id.parent,arena);
   PushString(arena," %.*s",UNPACK_SS(id.name));

   String res = PointArena(arena,mark);
   return res;
}

String Repr(StaticData data,Arena* arena){
   Byte* mark = MarkArena(arena);

   Repr(data.decl,arena);
   PushString(arena," (%d)",data.offset);

   String res = PointArena(arena,mark);
   return res;
}

String Repr(PortNode portNode,Arena* arena){
   Byte* mark = MarkArena(arena);

   PushString(arena,"%.*s",UNPACK_SS(portNode.node->inst->name));
   PushString(arena,":%d",portNode.port);

   String res = PointArena(arena,mark);
   return res;
}

String Repr(EdgeNode node,Arena* arena){
   Byte* mark = MarkArena(arena);

   Repr(node.node0,arena);
   PushString(arena," -> ");
   Repr(node.node1,arena);

   String res = PointArena(arena,mark);
   return res;
}







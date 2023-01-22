#include "textualRepresentation.hpp"

#include "parser.hpp"

SizedString UniqueRepr(FUInstance* inst,Arena* arena){
   FUDeclaration* decl = inst->declaration;
   SizedString str = PushString(arena,"%.*s_%.*s_%d",UNPACK_SS(decl->name),UNPACK_SS(inst->name),inst->id);
   return str;
}

SizedString Repr(FUInstance* inst,GraphDotFormat format,Arena* arena){
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

   SizedString res = PointArena(arena,mark);
   return res;
}

SizedString Repr(FUDeclaration* decl,Arena* arena){
   Byte* mark = MarkArena(arena);

   PushString(arena,"%.*s",UNPACK_SS(decl->name));

   SizedString res = PointArena(arena,mark);
   return res;
}

SizedString Repr(PortInstance in,PortInstance out,GraphDotFormat format,Arena* arena){
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

   SizedString res = PointArena(arena,mark);
   return res;
}

SizedString Repr(PortInstance port,GraphDotFormat format,Arena* arena){
   Byte* mark = MarkArena(arena);

   Repr(port.inst,GRAPH_DOT_FORMAT_NAME | GRAPH_DOT_FORMAT_ID,arena);
   PushString(arena,"_");

   bool expl = format & GRAPH_DOT_FORMAT_EXPLICIT;

   if(expl){
      PushString(arena,"Port:");
   }
   PushString(arena,"%d",port.port);

   SizedString res = PointArena(arena,mark);
   return res;
}

SizedString Repr(MappingNode node,Arena* arena){
   SizedString name = {};
   GraphDotFormat format = GRAPH_DOT_FORMAT_NAME | GRAPH_DOT_FORMAT_ID;
   if(node.type == MappingNode::NODE){
      Byte* mark = MarkArena(arena);

      Repr(node.nodes.instances[0],format,arena);
      PushString(arena," -- ");
      Repr(node.nodes.instances[1],format,arena);

      name = PointArena(arena,mark);
   } else if(node.type == MappingNode::EDGE){
      PortEdge e0 = node.edges[0];
      PortEdge e1 = node.edges[1];

      Byte* mark = MarkArena(arena);
      Repr(e0.units[0],format,arena);
      PushString(arena," -- ");
      Repr(e0.units[1],format,arena);
      PushString(arena,"/");
      Repr(e1.units[0],format,arena);
      PushString(arena," -- ");
      Repr(e1.units[1],format,arena);
      name = PointArena(arena,mark);
   } else {
      NOT_IMPLEMENTED;
   }

   return name;
}











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
      PushString(arena,"%.*s\\n",UNPACK_SS(inst->name));
   }
   if(expl && type){
      PushString(arena,"Type:");
   }
   if(type){
      PushString(arena,"%.*s\\n",UNPACK_SS(inst->declaration->name));
   }
   if(expl && id){
      PushString(arena,"Id:");
   }
   if(id){
      PushString(arena,"%d\\n",inst->id);
   }
   if(expl && delay){
      if(buffer){
         PushString(arena,"Buffer:");
      } else {
         PushString(arena,"Delay:");
      }
   }
   if(delay){
      if(buffer){
         PushString(arena,"%d\\n",instance->bufferAmount);
      } else {
         PushString(arena,"%d\\n",inst->baseDelay);
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
   if(node.type == MappingNode::NODE){
      FUInstance* n0 = node.nodes.instances[0];
      FUInstance* n1 = node.nodes.instances[1];

      name = PushString(arena,"%.*s -- %.*s",UNPACK_SS(n0->name),UNPACK_SS(n1->name));
   } else if(node.type == MappingNode::EDGE){
      PortEdge e0 = node.edges[0];
      PortEdge e1 = node.edges[1];

      Byte* mark = MarkArena(arena);
      Repr(e0.units[0],GRAPH_DOT_FORMAT_NAME,arena);
      PushString(arena," -- ");
      Repr(e0.units[1],GRAPH_DOT_FORMAT_NAME,arena);
      PushString(arena,"/");
      Repr(e1.units[0],GRAPH_DOT_FORMAT_NAME,arena);
      PushString(arena," -- ");
      Repr(e1.units[1],GRAPH_DOT_FORMAT_NAME,arena);
      name = PointArena(arena,mark);
   } else {
      NOT_IMPLEMENTED;
   }

   return name;
}











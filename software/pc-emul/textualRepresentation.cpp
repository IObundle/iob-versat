#include "textualRepresentation.hpp"

#include "parser.hpp"

SizedString Repr(FUInstance* inst,Arena* arena,bool printDelay){
   Byte* mark = MarkArena(arena);

   PushString(arena,"%.*s_%d",UNPACK_SS(inst->name),inst->id);

   if(printDelay){
      #if 0
      if(inst->declaration == versat->buffer && inst->config){
         PushString(arena,"_%d",inst->config[0]);
      } else if(inst->declaration == versat->fixedBuffer && inst->parameters.size > 0){
         Tokenizer tok(inst->parameters,".()",{"#("});

         while(!tok.Done()){
            Token amountParam = tok.NextToken();

            if(CompareString(amountParam,"AMOUNT")){
               tok.AssertNextToken("(");

               Token number = tok.NextToken();

               tok.AssertNextToken(")");

               PushString(arena,"_%.*s",UNPACK_SS(number));
               break;
            }
         }

      } else {
      #endif
         PushString(arena,"_%d",inst->baseDelay);
      #if 0
      }
      #endif
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

SizedString Repr(PortInstance port,Arena* memory){
   FUInstance* inst = port.inst;
   FUDeclaration* decl = inst->declaration;

   Byte* mark = MarkArena(memory);

   Repr(inst,memory);
   //PushString(memory,"_%d:",port.port);
   //Repr(decl,memory);

   SizedString res = PointArena(memory,mark);
   return res;
}

SizedString Repr(MappingNode* node,Arena* memory){
   SizedString name = {};
   if(node->type == MappingNode::NODE){
      FUInstance* n0 = node->nodes.instances[0];
      FUInstance* n1 = node->nodes.instances[1];

      name = PushString(memory,"%.*s -- %.*s",UNPACK_SS(n0->name),UNPACK_SS(n1->name));
   } else if(node->type == MappingNode::EDGE){
      PortEdge e0 = node->edges[0];
      PortEdge e1 = node->edges[1];

      Byte* mark = MarkArena(memory);
      Repr(e0.units[0],memory);
      PushString(memory," -- ");
      Repr(e0.units[1],memory);
      PushString(memory,"/");
      Repr(e1.units[0],memory);
      PushString(memory," -- ");
      Repr(e1.units[1],memory);
      name = PointArena(memory,mark);
   } else {
      NOT_IMPLEMENTED;
   }

   return name;
}











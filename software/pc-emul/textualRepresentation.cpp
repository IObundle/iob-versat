#include "textualRepresentation.hpp"

#include "parser.hpp"

SizedString Repr(Versat* versat,FUInstance* inst,Arena* arena,bool printDelay){
   Byte* mark = MarkArena(arena);

   PushString(arena,"%.*s_%d",UNPACK_SS(inst->name),inst->id);

   if(printDelay){
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
         PushString(arena,"_%d",inst->baseDelay);
      }
   }

   SizedString res = PointArena(arena,mark);

   return res;
}

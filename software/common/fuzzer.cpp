#include "parser.hpp"

// TODO: Let's first finalize the biggest problems with the Tokenizer before finishing implementing the Fuzzing facility.
#if 0
typedef void (*FuzzingFunction)(Token,Arena*);
#define FUZZING_FUNCTION(NAME) \
static void NAME(Token token,Arena* out)

FUZZING_FUNCTION(NoRightSpace){
   PushString(out,"%.*s",UNPACK_SS(token.text));
}

FUZZING_FUNCTION(DeleteOneChar){
   token..text.size -= 1;
   PushString(out,"%.*s ",UNPACK_SS(token.text));
}

static FuzzingFunction functionTable[] = {
   NoRightSpace,
   DeleteOneChar
};

String FuzzText(String formattedExample,Arena* out,int seed){
   Tokenizer tok(formattedExample,"",{});

   srand(seed);
   int probabilityMark = rand();
   printf("%d/%d\n",probabilityMark,RAND_MAX);

   Byte* mark = MarkArena(out);
   while(!tok.Done()){
      Token token = tok.NextToken();
      if(rand() <= probabilityMark){
         int functionToCall = rand() % ARRAY_SIZE(functionTable);

         functionTable[functionToCall](token,out);
      } else {
         PushString(out,"%.*s ",UNPACK_SS(token));
      }
   }
   Assert(tok.Done());
   String res = PointArena(out,mark);

   return res;
}
#endif

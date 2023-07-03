//#include "debug.hpp"

#include <cstdlib>

#include "parser.hpp"

typedef void (*FuzzingFunction)(Token,Arena*);
#define FUZZING_FUNCTION(NAME) \
static void NAME(Token token,Arena* arena)

FUZZING_FUNCTION(NoRightSpace){
   PushString(arena,"%.*s",UNPACK_SS(token));
}

FUZZING_FUNCTION(DeleteOneChar){
   token.size -= 1;
   PushString(arena,"%.*s ",UNPACK_SS(token));
}

static FuzzingFunction functionTable[] = {
   NoRightSpace,
   DeleteOneChar
};

String FuzzText(String formattedExample,Arena* arena,int seed){
   Tokenizer tok(formattedExample,"",{});

   srand(seed);
   int probabilityMark = rand();
   printf("%d/%d\n",probabilityMark,RAND_MAX);

   Byte* mark = MarkArena(arena);
   while(!tok.Done()){
      Token token = tok.NextToken();
      if(rand() <= probabilityMark){
         int functionToCall = rand() % ARRAY_SIZE(functionTable);

         functionTable[functionToCall](token,arena);
      } else {
         PushString(arena,"%.*s ",UNPACK_SS(token));
      }
   }
   Assert(tok.Done());
   String res = PointArena(arena,mark);

   return res;
}

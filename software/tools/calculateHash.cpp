#include <cstdio>

#include "memory.hpp"
#include "parser.hpp"
#include "utils.hpp"
#include "utilsCore.hpp"

Arena arenaInst;
Arena* arena = &arenaInst;

inline u64 HashString(String data,u64 hash){
  for(int i = 0; i < data.size; i++){
    hash = ((hash << 5) + hash) + data[i];
  }

  return hash;
}

// 0 - exe name
// 1+ - pair of files to calculate the hash
int main(int argc,const char* argv[]){
  if(argc < 2){
    fprintf(stderr,"Need at least 2 arguments: <exe> [list of filepaths] \n");
    return -1;
  }

  arenaInst = InitArena(Megabyte(64));
  Arena inst1 = InitArena(Megabyte(64));
  contextArenas[0] = &inst1;
  Arena inst2 = InitArena(Megabyte(64));
  contextArenas[1] = &inst2;
  
  // djb2
  u64 hash = 5381;
  i32 amountOfTokens = 0;
  for(i32 i = 0; i < argc - 1; i++){
    const char* filepath = argv[i + 1];

    BLOCK_REGION(arena);

    String content = PushFile(arena,filepath);

    Tokenizer tok(content,"`~!@#$%^&*()_-+=[{]}\\|;:'\",<.>/?",{""});

    while(!tok.Done()){
      Token token = tok.NextToken();

      for(int i = 0; i < token.size; i++){
        hash = ((hash << 5) + hash) + token.data[i];
      }
      amountOfTokens += 1;
    }
  }

  printf("%d:%lu\n",amountOfTokens,hash);

  return 0;
}

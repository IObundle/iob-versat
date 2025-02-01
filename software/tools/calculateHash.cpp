#include <cstdio>

#include "parser.hpp"
#include "utils.hpp"
#include "utilsCore.hpp"

Arena arenaInst;
Arena* arena = &arenaInst;

u64 HashString(String data){
  // djb2
  u64 hash = 5381;

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

  u64 hash = 0;
  i32 amountOfTokens = 0;
  for(i32 i = 0; i < argc - 1; i++){
    const char* filepath = argv[i + 1];

    BLOCK_REGION(arena);

    String content = PushFile(arena,filepath);

    Tokenizer tok(content,"[]()#@!~&|^+=-*/%;><?:",{""});

    //u64 n = 0;
    u64 fileHash = 0;

    while(!tok.Done()){
      Token token = tok.NextToken();

      fileHash = fileHash ^ HashString(token);
      amountOfTokens += 1;
    }

    hash = hash ^ fileHash; 
  }

  printf("%d:%lu\n",amountOfTokens,hash);

  return 0;
}

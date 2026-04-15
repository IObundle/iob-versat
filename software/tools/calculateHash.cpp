#include <cstdio>

#include "memory.hpp"
#include "utils.hpp"
#include "utilsCore.hpp"
#include "parser.hpp"

Arena arenaInst;
Arena* arena = &arenaInst;

inline u64 HashString(String data,u64 hash){
  for(int i = 0; i < data.size; i++){
    hash = ((hash << 5) + hash) + data[i];
  }

  return hash;
}

String PushFile(Arena* arena,String path){
  // Care since this is stored on a static buffer
  const char* asCStr = StaticFormat("%.*s",UN(path));
  FILE* file = fopen(asCStr,"r");

  long int size = GetFileSize(file);

  AlignArena(arena,alignof(void*));

  Byte* mem = PushBytes(arena,size);
  int amountRead = fread(mem,sizeof(Byte),size,file);

  if(amountRead != size){
    fprintf(stderr,"Memory PushFile failed to read entire file\n");
    exit(-1);
  }

  String res = {};
  res.size = size;
  res.data = (const char*) mem;

  return res;
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
  
  for(int i = 0; i < 8; i++){
    singleUseCasesArenas[i] = InitArena(Megabyte(1));
  }

  TEMP_REGION(temp,nullptr);

  FREE_ARENA(parsing);
  auto TokenizeFunction = [](const char* start,const char* end) -> TokenizeResult{
    TokenizeResult res = ParseWhitespace(start,end);
    res |= ParseComments(start,end);
    res |= ParseSymbols(start,end);
    res |= ParseNumber(start,end);
    res |= ParseIdentifier(start,end);

    return res;
  };
  
  // djb2
  u64 hash = 5381;
  i32 amountOfTokens = 0;
  for(i32 i = 0; i < argc - 1; i++){
    const char* filepath = argv[i + 1];

    BLOCK_REGION(arena);

    String content = PushFile(arena,filepath);

    const char* ptr = content.data;
    const char* end = content.data + content.size;

    printf("%p %p\n",ptr,end);

    while(ptr < end){
      TokenizeResult res = TokenizeFunction(ptr,end);
      TokenType type = res.token.type;

      bool doHash = true;
      if(type == TokenType_WHITESPACE || type == TokenType_COMMENT || type == TokenType_EOF){
        doHash = false;
      }

      int size = res.bytesParsed;
      if(size == 0){
        size = 1;
      }

      if(doHash){
        String asStr = String(ptr,size);
      
        for(int i = 0; i < asStr.size; i++){
          hash = ((hash << 5) + hash) + asStr[i];
        }
        amountOfTokens += 1;
      }

      ptr += size;
    }
  }

  printf("HASH_RESULT:%d:%lu\n",amountOfTokens,hash);

  return 0;
}

#include "debug.hpp"

#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <dlfcn.h>
#include <ftw.h>
#include <cstdlib>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <fcntl.h>
#include <execinfo.h>

#include "parser.hpp"
#include "memory.hpp"
#include "utilsCore.hpp"

struct Addr2LineConnection{
  int writePipe;
  int readPipe;
  int pid;
};

struct Location{
  String functionName;
  String fileName;
  int line;
};

static Arena debugArenaInst = {};
bool debugFlag = false;
Arena* debugArena = &debugArenaInst;

static SignalHandler old_SIGUSR1 = nullptr;
static SignalHandler old_SIGSEGV = nullptr;
static SignalHandler old_SIGABRT = nullptr;

// TODO: Currently we only allow one function to be registered. This is problematic because we already register the printstacktrace function and therefore other pieces of code should not use this (like testbench.hpp) otherwise we lose stack traces. We should keep a list of functions to call and just register a master function. Not doing it for now because do not know if I'll keep testbench or if we ever gonna use this more than these two times 
void SetDebugSignalHandler(SignalHandler func){
   old_SIGUSR1 = signal(SIGUSR1, func);
   old_SIGSEGV = signal(SIGSEGV, func);
   old_SIGABRT = signal(SIGABRT, func);
}

// TODO: There is zero error checking in these functions. User might not have the addr2line program
static Addr2LineConnection StartAddr2Line(){
  // Perspective of the parent
  int pipeWrite[2]; // Parent -> child. 0 is read (child), 1 is write (parent)
  int pipeRead[2];  // Child -> parent. 0 is read (parent), 1 is write (child)

  int res = pipe(pipeWrite);
  if(res == -1){
    fprintf(stderr,"Failed to create write pipe\n");
    exit(0);
  }

  //res = pipe(pipeRead);
  res = pipe2(pipeRead,O_NONBLOCK);
  if(res == -1){
    fprintf(stderr,"Failed to create read pipe\n");
    exit(0);
  }

  char exePathBuffer[4096]; // Important to init to zero, readlink does not insert a '\0'
  int size = readlink("/proc/self/exe",exePathBuffer,4096);

  if(size < 0){
    printf("Error using readlink\n");
  } else {
    exePathBuffer[size] = '\0';
  }

  //printf("%s\n",exePathBuffer);

  pid_t pid = fork();

  if(pid < 0){
    printf("Error calling fork errno: %d\n",errno);
    exit(-1);
  } else if(pid == 0){
    close(pipeWrite[1]);
    close(pipeRead[0]);

    dup2(pipeWrite[0],STDIN_FILENO);
    dup2(pipeRead[1],STDOUT_FILENO);
    //close(STDERR_FILENO);

    char* args[] = {(char*) "addr2line",
                    (char*) "-C",
                    (char*) "-f",
                    (char*) "-i",
                    //(char*) "-p",
                    (char*) "-e",
                    (char*) exePathBuffer,
                    nullptr};

    execvp("addr2line",(char* const*) args);
    fprintf(stderr,"Error calling execvp for addr2line: %s\n",strerror(errno));
    exit(0);
  } else {
    Addr2LineConnection con = {};

    close(pipeRead[1]);
    close(pipeWrite[0]);

    con.readPipe = pipeRead[0];
    con.writePipe = pipeWrite[1];
    con.pid = pid;
    
    return con;
  }

  printf("Failed to initialize addr2line\n");
  return (Addr2LineConnection){};
}

// Note: We cannot use Assert here otherwise we might enter a infinite loop. Only the C assert
static Array<Location> CollectStackTrace(Arena* out,Arena* temp){
  void* addrBuffer[100];

  Addr2LineConnection connection = StartAddr2Line();
  Addr2LineConnection* con = &connection;
  
  int lines = backtrace(addrBuffer, 100);
  char** strings = backtrace_symbols(addrBuffer, lines);
  if (strings == NULL) {
    printf("Error getting stack trace\n");
  }

  defer{ free(strings); };
  //printf("Lines: %d\n",lines);
  
  for(int i = 0; i < lines; i++){
    String line = STRING(strings[i]);

    Tokenizer tok(line,"()",{});

    Token name = tok.NextToken();
    assert(CompareString(tok.NextToken(),"("));
    Token offset = tok.NextFindUntil(")");
    assert(CompareString(tok.NextToken(),")"));

    if(offset.size <= 0){
      continue;
    }
    
    region(temp){
      String toWrite = PushString(temp,"%.*s\n",UNPACK_SS(offset));
      int written = write(con->writePipe,toWrite.data,toWrite.size);
      if(written != toWrite.size){
        printf("Failed to write: (%d/%d)\n",written,toWrite.size);
      }
    }
  }
  
  //printf("Gonna close write pipe\n");
  {
  int result = close(con->writePipe);
  if(result != 0) printf("Error closing write pipe: %d\n",errno);
  }
  
  int bufferSize = Kilobyte(64);
  Byte* buffer = PushBytes(temp,bufferSize);
  int amountRead = 0;

  while(1){
    int result = read(con->readPipe,&buffer[amountRead],bufferSize - amountRead);

    //printf("Result: %d\n",result);
    if(result > 0) {
      amountRead += result;
    } else if(result == 0){
      break;
    } else{
      if(!(errno == EAGAIN || errno == EWOULDBLOCK)){
        printf("Errno: %d\n",errno);
      }
    }

    usleep(1); // Give some time for addr2line to work
  }

  int lineCount = 0;
  for(int i = 0; i < amountRead; i++){
    if(buffer[i] == '\n'){
      lineCount += 1;
    }
  }
  
  String content = {(char*) buffer,amountRead};
  Array<Location> result = PushArray<Location>(out,lineCount / 2);

  //printf("%.*s\n",UNPACK_SS(content));
  
  Tokenizer tok(content,":",{"\n"});
  tok.keepWhitespaces = true;

  int index = 0;
  while(!tok.Done()){
    String functionName = tok.NextFindUntil("\n");
    assert(CompareString(tok.NextToken(),"\n"));
    String fileName = tok.NextFindUntil(":");
    assert(CompareString(tok.NextToken(),":"));
    String lineString = tok.NextToken();
    tok.NextFindUntil("\n");
    assert(CompareString(tok.NextToken(),"\n"));

    if(CompareString(functionName,"??") || CompareString(fileName,"??") || CompareString(lineString,"?")){
      continue;
    }

    result[index].functionName = functionName;
    result[index].fileName = fileName;
    result[index].line = ParseInt(lineString);
    
    index += 1;
  }
  result.size = index;

  return result;
}

#include <filesystem>
namespace fs = std::filesystem;

// TODO: Same function is being used in versatCompiler.cpp
static String GetAbsolutePath(const char* path,Arena* arena){
  fs::path canonical = fs::weakly_canonical(path);
  String res = PushString(arena,"%s",canonical.c_str());
  return res;
}

void PrintStacktrace(){
  const String rootPath = STRING(ROOT_PATH); // ROOT_PATH must have the pathname to the top of the source code folder (the largest subpath common to all code files)

  BLOCK_REGION(debugArena);

  Arena tempInst = SubArena(debugArena,Kilobyte(128));
  Arena* temp = &tempInst;
  
  Array<Location> traces = CollectStackTrace(debugArena,temp);
  int size = traces.size;
  
  Array<String> canonical = PushArray<String>(temp,size);
  for(int i = 0; i < size; i++){
    canonical[i] = GetAbsolutePath(StaticFormat("%.*s",UNPACK_SS(traces[i].fileName)),temp);
  }

  int maxSize = 0;
  for(String& str : canonical){
    if(str.size > rootPath.size){
      String subString = {str.data,rootPath.size}; // Only works to remove the common path because we set rootPath to the largest common subpath that we have.
      if(CompareString(subString,rootPath)){
        str.data += rootPath.size;
        str.size -= rootPath.size;
      }
    }
    maxSize = std::max(maxSize,str.size);

    //printf("%.*s\n",UNPACK_SS(str));
  }

  for(int i = 0; i < size; i++){
    String str = canonical[i];
    printf("#%-2d %.*s",i,UNPACK_SS(str));

    for(int j = 0; j < maxSize - str.size; j++){
      printf(" ");
    }

    printf(" %.*s",UNPACK_SS(traces[i].functionName));
    printf(":%d\n",traces[i].line);
  }
}

static void SignalPrintStacktrace(int sign){
  // Install old handlers first so that any bug afterwards does not cause an infinite loop
  switch(sign){
  case SIGUSR1:{
    old_SIGUSR1(sign);
  }break;
  case SIGSEGV:{
    old_SIGSEGV(sign);
  }break;
  case SIGABRT:{
    old_SIGABRT(sign);
  }break;
  default: NOT_IMPLEMENTED;
  }

  printf("\nProgram encountered an error. Stack trace:\n");
  PrintStacktrace();
  printf("\n");
}

void InitDebug(){
  static bool init = false;
  if(init){
    return;
  }
  init = true;

  debugArenaInst = InitArena(Megabyte(1));
  SetDebugSignalHandler(SignalPrintStacktrace);
}


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
#include <sys/ptrace.h>
#include <termios.h>
#include <fcntl.h>
#include <execinfo.h>
#include <link.h>

#include "parser.hpp"
#include "memory.hpp"
#include "utils.hpp"
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
bool currentlyDebugging = false;

bool CurrentlyDebugging(){
  static bool init = false;
  static bool value;
  
  if(init){
    return value;
  }

  init = true;
  char buf[Kilobyte(16)];

  int fd = open("/proc/self/status", O_RDONLY);
  if (fd == -1)
    return value;

  int amount = read(fd,buf,sizeof(buf) - 1);
  close(fd);

  buf[amount] = '\0';

  Tokenizer tok(STRING(buf,amount),":",{});
  while(!tok.Done()){
    Token val = tok.NextToken();

    if(CompareString(val,"TracerPid")){
      tok.AssertNextToken(":");

      Token pidToken = tok.NextToken();

      int number = ParseInt(pidToken);
      if(number == 0){
        value = false;
      } else {
        value = true;
      }
      break;
    }
  }
  
  return value;
}

static SignalHandler old_SIGUSR1 = nullptr;
static SignalHandler old_SIGSEGV = nullptr;
static SignalHandler old_SIGABRT = nullptr;
static SignalHandler old_SIGILL = nullptr;

// TODO: Currently we only allow one function to be registered. This is problematic because we already register the printstacktrace function and therefore other pieces of code should not use this (like testbench.hpp) otherwise we lose stack traces. We should keep a list of functions to call and just register a master function. Not doing it for now because do not know if I'll keep testbench or if we ever gonna use this more than these two times 
void SetDebugSignalHandler(SignalHandler func){
  old_SIGUSR1 = signal(SIGUSR1, func);
  old_SIGSEGV = signal(SIGSEGV, func);
  old_SIGABRT = signal(SIGABRT, func);
  old_SIGILL  = signal(SIGILL, func);
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
    fprintf(stderr,"Error using readlink\n");
  } else {
    exePathBuffer[size] = '\0';
  }

  //printf("%s\n",exePathBuffer);

  pid_t pid = fork();

  if(pid < 0){
    fprintf(stderr,"Error calling fork errno: %d\n",errno);
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

  fprintf(stderr,"Failed to initialize addr2line\n");
  return (Addr2LineConnection){};
}

// Note: We cannot use Assert here otherwise we might enter a infinite loop. Only the C assert
static Array<Location> CollectStackTrace(Arena* out,Arena* temp){
  BLOCK_REGION(temp);

  void* addrBuffer[100];
  
  Addr2LineConnection connection = StartAddr2Line();
  Addr2LineConnection* con = &connection;
  
  int lines = backtrace(addrBuffer, 100);
  char** strings = backtrace_symbols(addrBuffer, lines);
  if (strings == NULL) {
    fprintf(stderr,"Error getting stack trace\n");
  }
  defer{ free(strings); };
  
  Array<size_t> fileAddresses = PushArray<size_t>(temp,lines);
  
  for(int i = 0; i < lines; i++){
    Dl_info info;
    link_map* link_map;
    if(dladdr1(addrBuffer[i],&info,(void**) &link_map,RTLD_DL_LINKMAP)){
      fileAddresses[i] = ((size_t) addrBuffer[i]) - link_map->l_addr - 1; // - 1 to get the actual instruction, PC points to the next one
    }
  }
  
  //printf("Lines: %d\n",lines);
  for(int i = 0; i < lines; i++){
    String line = STRING(strings[i]);
    Tokenizer tok(line,"()",{});
    String first = tok.NextFindUntil("(").value();

    if(!(Contains(first,"versat") || Contains(first,"./versat"))){  // A bit hardcoded but appears to work fine
      continue;
    }
    
    region(temp){
      String toWrite = PushString(temp,"%lx\n",fileAddresses[i]);
      //printf("%.*s\n",UNPACK_SS(toWrite));
      int written = write(con->writePipe,toWrite.data,toWrite.size);
      if(written != toWrite.size){
        fprintf(stderr,"Failed to write: (%d/%d)\n",written,toWrite.size);
      }
    }
  }
  
  //printf("Gonna close write pipe\n");
  {
    int result = close(con->writePipe);
    if(result != 0) fprintf(stderr,"Error closing write pipe: %d\n",errno);
  }
  
  int bufferSize = Kilobyte(64);
  Byte* buffer = PushBytes(out,bufferSize);
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
        fprintf(stderr,"Errno: %d\n",errno);
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

  Tokenizer tok(content,":",{});
  tok.keepWhitespaces = true;

  int index = 0;
  while(!tok.Done()){
    Token line = tok.PeekRemainingLine();
    tok.AdvancePeekBad(line);
    
    String functionName = line;

    Opt<Token> token = tok.NextFindUntil(":");

    Assert(token.has_value());
    
    String fileName = token.value();
    assert(CompareString(tok.NextToken(),":"));
    Token lineString = tok.NextToken();

    tok.AdvanceRemainingLine();
    //line = tok.PeekRemainingLine();
    //tok.AdvancePeekBad(line);
    //printf("FN: %.*s\n",UNPACK_SS(functionName));
    //printf("fN: %.*s\n",UNPACK_SS(fileName));
    //printf("ls: %.*s\n",UNPACK_SS(lineString));
    if(CompareString(functionName,"??") || CompareString(fileName,"??") || CompareString(lineString,"?")){
      continue;
    }

    result[index].functionName = TrimWhitespaces(functionName);
    result[index].fileName = TrimWhitespaces(fileName);
    result[index].line = ParseInt(lineString);
    
    index += 1;
  }
  result.size = index;

  return result;
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

#if 1
  // Remove long repeated paths so it displays nicely on the terminal
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
#endif

  for(int i = 0; i < size; i++){
    String str = canonical[i];
    fprintf(stderr,"#%-2d %.*s",i,UNPACK_SS(str));

    for(int j = 0; j < maxSize - str.size; j++){
      fprintf(stderr," ");
    }

    fprintf(stderr," %.*s",UNPACK_SS(traces[i].functionName));
    fprintf(stderr,":%d\n",traces[i].line);
  }
}

static void SignalPrintStacktrace(int sign){
  // Need to Install old handlers first so that any bug afterwards does not cause an infinite loop
  switch(sign){
  case SIGUSR1:{
    signal(SIGUSR1,old_SIGUSR1);
  }break;
  case SIGSEGV:{
    signal(SIGSEGV,old_SIGSEGV);
  }break;
  case SIGABRT:{
    signal(SIGABRT,old_SIGABRT);
  }break;
  case SIGILL:{
    signal(SIGILL,old_SIGILL);
  }break;
  }

  fprintf(stderr,"\nProgram encountered an error. Stack trace:\n");
  PrintStacktrace();
  fprintf(stderr,"\n");
  fflush(stdout);
  fflush(stderr);
  
  raise(sign);
}

void InitDebug(){
  static bool init = false;
  if(init){
    return;
  }

  init = true;
  currentlyDebugging = CurrentlyDebugging();
  
  debugArenaInst = InitArena(Megabyte(256));
  SetDebugSignalHandler(SignalPrintStacktrace);
}


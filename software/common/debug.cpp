// TODO: Very important to make sure that we can still compile Versat even if this BFD library does not exist/not found.
//       We do not want to force user to deal with compilation issues just to have callstack reporting (which might not even be needed if the program does not encounter an error anyway.


#if 0
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

  Tokenizer tok(String(buf,amount),":",{});
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
// NOTE: If any error occurs, we just want to prevent stacktraces from being displayed. We do not want to close the program just because it is failing to launch addr2line
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
    exit(0);
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

// Note: We cannot use Assert here otherwise we might enter a infinite loop. Only the C assert (and we should make it debug only, all this code must be signal safe.)
static Array<Location> CollectStackTrace(Arena* out){
  void* addrBuffer[100];
  
  Addr2LineConnection connection = StartAddr2Line();
  Addr2LineConnection* con = &connection;
  
  int lines = backtrace(addrBuffer, 100);
  char** strings = backtrace_symbols(addrBuffer, lines);
  if (strings == NULL) {
    fprintf(stderr,"Error getting stack trace\n");
  }
  defer{ free(strings); };
  
  Array<size_t> fileAddresses = PushArray<size_t>(out,lines);
  
  for(int i = 0; i < lines; i++){
    Dl_info info;
    link_map* link_map;
    if(dladdr1(addrBuffer[i],&info,(void**) &link_map,RTLD_DL_LINKMAP)){
      fileAddresses[i] = ((size_t) addrBuffer[i]) - link_map->l_addr - 1; // - 1 to get the actual instruction, PC points to the next one
    }
  }
  
  //printf("Lines: %d\n",lines);
  for(int i = 0; i < lines; i++){
    String line = strings[i];
    Tokenizer tok(line,"()",{});
    String first = tok.NextFindUntil("(").value();

    if(!(Contains(first,"versat") || Contains(first,"./versat"))){  // A bit hardcoded but appears to work fine
      continue;
    }
    
    region(out){
      String toWrite = PushString(out,"%lx\n",fileAddresses[i]);
      //printf("%.*s\n",UN(toWrite));
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
  BLOCK_REGION(debugArena);
  
  Array<Location> traces = CollectStackTrace(debugArena);
  int size = traces.size;
  
  Array<String> canonical = PushArray<String>(debugArena,size);
  for(int i = 0; i < size; i++){
    canonical[i] = GetAbsolutePath(traces[i].fileName,debugArena);
  }

#if 1
  const String rootPath = ROOT_PATH; // ROOT_PATH must have the pathname to the top of the source code folder (the largest subpath common to all code files)

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
    maxSize = MAX(maxSize,str.size);

    //printf("%.*s\n",UN(str));
  }
#endif

  for(int i = 0; i < size; i++){
    String str = canonical[i];
    fprintf(stderr,"#%-2d %.*s",i,UN(str));

    for(int j = 0; j < maxSize - str.size; j++){
      fprintf(stderr," ");
    }

    fprintf(stderr," %.*s",UN(traces[i].functionName));
    fprintf(stderr,":%d\n",traces[i].line);
  }
}

// TODO: We shouldn't call fprintf, since stdio is not signal safe.
//       Do not know exactly how much of this is a problem, but nevertheless it is the right thing to do.
//       We can still call write to handle output this way.
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

void InitDebug(const char* exeName){
  static bool init = false;
  if(init){
    return;
  }

  init = true;
  currentlyDebugging = CurrentlyDebugging();
  
  debugArenaInst = InitArena(Megabyte(256));
  SetDebugSignalHandler(SignalPrintStacktrace);
}

#else 
#include "debug.hpp"

#include <sys/wait.h>
#include <execinfo.h>
#include <link.h>
#include <fcntl.h>

#include "utils.hpp"

// These defines are needed by bfd
#define PACKAGE 1
#define PACKAGE_VERSION 1
#include <bfd.h>
#include <cxxabi.h>

typedef void (*SignalHandler)(int sig);
 
struct Location{
  String functionName;
  String fileName;
  u32 line;
};

Arena* debugArena = nullptr;
bool debugFlag = false;
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

  u32 amount = read(fd,buf,sizeof(buf) - 1);
  close(fd);

  buf[amount] = '\0';

  String content = String(buf,amount);

  const char* lineStart = content.data;
  u32 i = 0;
  bool endEarly = false;
  while(!endEarly && (i < amount && content[i] != '\0')){
    while(i < amount && (content[i] != '\n' && content[i] != '\0')){
      i++;
    }

    const char* lineEnd = &content.data[i - 1];
    String asStr = String(lineStart,(u32) (lineEnd - lineStart + 1));
    
    if(Contains(asStr,"TracerPid:")){
      const char* pidStart = lineStart + 10;
      while(*pidStart == ' ' || *pidStart == '\t'){
        pidStart += 1;
      }

      if(*pidStart == '0'){
        value = false;
      } else {
        value = true;
      }

      endEarly = true;
      break;
    }

    i += 1;
    lineStart = &content.data[i];
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

static Array<Location> CollectStackTrace(Arena*,int offset);

void PrintStacktrace(int offset = 2){
  BLOCK_REGION(debugArena);

  Array<Location> traces = CollectStackTrace(debugArena,offset);

  traces.size -= 1;

  // TODO: The above is to eliminate the '_start' line which makes sense but do not know why this is needed.
  //       We are getting a utility function that we do not call which might be a problem on our end.
  //       Regardless check this when nothing too major to do.
  traces = Offset(traces,1);

  Array<Location> reversed = Reverse(traces,debugArena);

#if 1
  const String rootPath = ROOT_PATH; // ROOT_PATH must have the pathname to the top of the source code folder (the largest subpath common to all code files)

  // Remove long repeated paths so it displays nicely on the terminal
  int maxSize = 0;
  for(Location& loc : reversed){
    if(loc.fileName.size > rootPath.size){
      String subString = {loc.fileName.data,rootPath.size}; // Only works to remove the common path because we set rootPath to the largest common subpath that we have.
      if(CompareString(subString,rootPath)){
        loc.fileName.data += rootPath.size;
        loc.fileName.size -= rootPath.size;
      }
    }
    maxSize = MAX(maxSize,loc.fileName.size);

    //printf("%.*s\n",UN(str));
  }
#endif

  maxSize += 3; // Some more spacing between files and function names

  traces = reversed;
  int size = traces.size;
  
  for(int i = 0; i < size; i++){
    String str = traces[i].fileName;
    
    fprintf(stderr,"#%-2d %.*s",i,UN(str));

    for(u32 j = 0; j < maxSize - str.size; j++){
      fprintf(stderr," ");
    }

    fprintf(stderr," %.*s",UN(traces[i].functionName));
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
  PrintStacktrace(3);
  fprintf(stderr,"\n");
  fflush(stdout);
  fflush(stderr);
  
  raise(sign);
}

// NOTE: Mostly a copy from the github repo, to check if it works and if we can build upon it.
//       For a proper implementation, I do not want to use the bfd_map_over_sections.
//       I want to see if we can make this faster so that we can call these functions multiple times
//       in a normal run of the program. We will probably want cache stuff aggressevely to reduce the amount
//       of times we end up calling into this.
//       I want to build debug stuff that relies on callstack information more reliably and I want this function to
//       at least be fast enough that we do not notice any slow down from using it.

static bfd* abfd;
static bfd_vma pc;
static bool found = false;
static const char *filename;
static const char *functionname;
static unsigned int line;
static unsigned int discriminator;
static asymbol **syms;

static void FindAddressHelper(bfd *abfd,asection *section,void *data){
  // TODO: Different sections might give different results, but in those cases we probably do not care because must likely the function does not belong to our code. 
  if (found){
    return;
  }
  
  bfd_vma vma = section->vma;
  if(pc < vma) {
    return;
  }
  
  bfd_size_type size = bfd_section_size(section);
  if(pc >= vma + size) {
    return;
  }

  found = bfd_find_nearest_line_discriminator (abfd, section, syms, pc - vma,
                                               &filename, &functionname,
                                               &line, &discriminator);
}

static void InitSymtab(bfd *abfd,Arena* out){
  if((bfd_get_file_flags(abfd) & HAS_SYMS) == 0){
    return;
  }

  long bytesRequired = bfd_get_symtab_upper_bound(abfd);
  bool dynamic = false;
  if(bytesRequired == 0) {
    bytesRequired = bfd_get_dynamic_symtab_upper_bound(abfd);
    dynamic = true;
  }
  if(bytesRequired < 0){
    // TODO: Error, and do not know if we need to abort or if we can do something about it
  }

  auto mark = MarkArena(out);
  
  syms = (asymbol **) PushBytes(out,bytesRequired);
  long symcount;
  if(dynamic) {
    symcount = bfd_canonicalize_dynamic_symtab(abfd,syms);
  } else {
    symcount = bfd_canonicalize_symtab(abfd,syms);
  }
  if(symcount < 0){
    // TODO: Error, and do not know if we need to abort or if we can do something about it
  }

  if(symcount == 0 && !dynamic && (bytesRequired = bfd_get_dynamic_symtab_upper_bound (abfd)) > 0){
    PopMark(mark);
      
    syms = (asymbol **) PushBytes(out,bytesRequired);
    symcount = bfd_canonicalize_dynamic_symtab (abfd, syms);
  }
}

void InitDebug(const char* exeName){
  static bool init = false;
  if(init){
    return;
  }

  init = true;
  currentlyDebugging = CurrentlyDebugging();
  
  static Arena debugArenaInst = InitArena(Megabyte(1));
  debugArena = &debugArenaInst;
  SetDebugSignalHandler(SignalPrintStacktrace);

  abfd = bfd_openr(exeName,NULL);
  if(abfd == nullptr){
    printf("Failed bfd_open call\n");
    return;
  }
  abfd->flags |= BFD_DECOMPRESS;

  if (bfd_check_format (abfd, bfd_archive)) {
    printf("Cannot get address from archive\n");
  }
  
  char **matching;
  // NOTE: This fuction needs to be called in order to fill the abfd.
  if(! bfd_check_format_matches (abfd, bfd_object, &matching)){
    printf("Failed check\n");
  }
  
  InitSymtab(abfd,debugArena);
}

static Array<Location> CollectStackTrace(Arena* out,int offset = 1){
  TEMP_REGION(temp,out);

  void* addrBuffer[100];
  int lines = backtrace(addrBuffer, 100);

  // Offset and reduce amount by 1 so that we do not collect this function as part of the callstack
  Array<Location> result = PushArray<Location>(out,lines - offset);
  void** goodAddr = addrBuffer + offset;

  int goodInserted = 0;
  for(int i = 0; i < lines - 1; i++){
    Dl_info info;
    link_map* link_map;
    if(dladdr1(goodAddr[i],&info,(void**) &link_map,RTLD_DL_LINKMAP)){
      pc = ((size_t) goodAddr[i]) - link_map->l_addr - 1; // - 1 to get the actual instruction, PC points to the next one
      found = false;
      bfd_map_over_sections(abfd,FindAddressHelper, NULL);
      if(found){
        int stat;

        size_t size = 4000;
        char* buffer = (char*) PushBytes(temp,size);

        const char* name = abi::__cxa_demangle(functionname,buffer,&size,&stat);

        Assert(stat != -1 && stat != -3);
        
        if(stat != 0 || name == nullptr){
          result[goodInserted].functionName = PushString(out,"%s",functionname);
        } else {
          result[goodInserted].functionName = PushString(out,"%s",name);
        }

        result[goodInserted].fileName = PushString(out,"%s",filename);
        result[goodInserted].line = line;
        goodInserted += 1;
      }
    }
  }

  result.size = goodInserted;
  return result;
};
#endif

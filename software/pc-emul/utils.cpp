#include "utils.hpp"

#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/ptrace.h>

#include <cstdio>
#include <cstring>
#include <cstdarg>

bool CurrentlyDebugging(){
   static bool init = false;
   static bool value;

   if(!init){
      init = true;
      if(ptrace(PTRACE_TRACEME,0,1,0) < 0){
         value = true;
      } else {
         ptrace(PTRACE_DETACH,0,1,0);
      }
   }

   return value;
}

char* StaticFormat(const char* format,...){
   static const int BUFFER_SIZE = 1024*4;
   static char buffer[BUFFER_SIZE];

   va_list args;
   va_start(args,format);

   int written = vsprintf(buffer,format,args);

   va_end(args);

   Assert(written < BUFFER_SIZE);

   return buffer;
}

// Misc
void FlushStdout(){
   fflush(stdout);
}

long int GetFileSize(FILE* file){
   long int mark = ftell(file);

   fseek(file,0,SEEK_END);
   long int size = ftell(file);

   fseek(file,mark,SEEK_SET);

   return size;
}

char* GetCurrentDirectory(){
   static char buffer[PATH_MAX];
   buffer[0] = '\0';
   getcwd(buffer,PATH_MAX);
   return buffer;
}

void MakeDirectory(const char* path){
   mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

FILE* OpenFileAndCreateDirectories(const char* path,const char* format){
   char buffer[PATH_MAX];
   memset(buffer,0,PATH_MAX);

   for(int i = 0; path[i]; i++){
      buffer[i] = path[i];

      if(path[i] == '/'){
         DIR* dir = opendir(buffer);
         if(!dir && errno == ENOENT){
            MakeDirectory(buffer);
         }
         if(dir){
            closedir(dir);
         }
      }
   }

   FILE* file = fopen(buffer,format);
   Assert(file);
   return file;
}

String PathGoUp(char* pathBuffer){
   String content = STRING(pathBuffer);

   int i = content.size - 1;
   for(; i >= 0; i--){
      if(content[i] == '/'){
         break;
      }
   }

   if(content[i] != '/'){
      return content;
   }

   pathBuffer[i] = '\0';
   content.size = i;

   return content;
}




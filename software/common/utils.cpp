#include "utils.hpp"

#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/ptrace.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <cerrno>

#include <time.h>

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
   buffer[written] = '\0';
  
   return buffer;
}

Time GetTime(){
   timespec time;
   clock_gettime(CLOCK_MONOTONIC, &time);

   Time t = {};
   t.seconds = time.tv_sec;
   t.microSeconds = time.tv_nsec * 1000;

   return t;
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

String ExtractFilenameOnly(String filepath){
   int i;
   for(i = filepath.size - 1; i >= 0; i--){
      if(filepath[i] == '/'){
         break;
      }
   }

   String res = {};
   res.data = &filepath.data[i + 1];
   res.size = filepath.size - (i + 1);
   return res;
}

char* GetCurrentDirectory(){
  // TODO: Maybe receive arena and remove use of static buffer
  static char buffer[PATH_MAX];
   buffer[0] = '\0';
   char* res = getcwd(buffer,PATH_MAX);

   return res;
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
   if(file == nullptr){
     printf("Failed to open file: %s\n",path);
     exit(-1);
   }

   return file;
}

void CreateDirectories(const char* path){
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

Optional<Array<String>> GetAllFilesInsideDirectory(String dirPath,Arena* arena){
   DIR* dir = opendir(StaticFormat("%.*s",UNPACK_SS(dirPath))); // Make sure it's zero terminated

   if(dir == nullptr){
      return {};
   }

   // Count number of files
   int amount = 0;
   while(1){      
      dirent* d = readdir(dir);

      if(d == nullptr){
         break;
      }
      
      if(d->d_name[0] == '.' && d->d_name[1] == '\0'){
         continue;
      }
      if(d->d_name[0] == '.' && d->d_name[1] == '.' && d->d_name[2] == '\0'){
         continue;
      }

      amount += 1;
   }

   Array<String> arr = PushArray<String>(arena,amount);

   rewinddir(dir);

   int index = 0;
   while(1){      
      dirent* d = readdir(dir);

      if(d == nullptr){
         break;
      }

      if(d->d_name[0] == '.' && d->d_name[1] == '\0'){
         continue;
      }
      if(d->d_name[0] == '.' && d->d_name[1] == '.' && d->d_name[2] == '\0'){
         continue;
      }

      arr[index] = PushString(arena,"%s",d->d_name);
      index += 1;
   }
   
   return arr;
}


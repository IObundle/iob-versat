#include "utils.hpp"

#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "stdio.h"
#include "string.h"

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

SizedString PathGoUp(char* pathBuffer){
   SizedString content = MakeSizedString(pathBuffer);

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




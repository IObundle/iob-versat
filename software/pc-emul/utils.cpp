#include "utils.hpp"

#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "stdio.h"
#include "string.h"

bool operator<(const SizedString& lhs,const SizedString& rhs){
   for(int i = 0; i < std::min(lhs.size,rhs.size); i++){
      if(lhs[i] < rhs[i]){
         return true;
      }
      if(lhs[i] > rhs[i]){
         return false;
      }
   }

   if(lhs.size < rhs.size){
      return true;
   }

   return false;
}

bool operator==(const SizedString& lhs,const SizedString& rhs){
   bool res = CompareString(lhs,rhs);

   return res;
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




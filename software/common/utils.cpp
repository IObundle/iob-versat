#include "utils.hpp"

#include <dirent.h>

#include <filesystem>
namespace fs = std::filesystem;

Optional<Array<String>> GetAllFilesInsideDirectory(String dirPath,Arena* out){
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

   Array<String> arr = PushArray<String>(out,amount);

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

      arr[index] = PushString(out,"%s",d->d_name);
      index += 1;
   }
   
   return arr;
}

String PushEscapedString(Arena* out,String toEscape,char spaceSubstitute){
  Byte* mark = MarkArena(out);

  for(int i = 0; i < toEscape.size; i++){
    switch(toEscape[i]){
    case '\a': PushString(out,STRING("\\a")); break;
    case '\b': PushString(out,STRING("\\b")); break;
    case '\r': PushString(out,STRING("\\r")); break;
    case '\f': PushString(out,STRING("\\f")); break;
    case '\t': PushString(out,STRING("\\t")); break;
    case '\n': PushString(out,STRING("\\n")); break;
    case '\0': PushString(out,STRING("\\0")); break;
    case '\v': PushString(out,STRING("\\v")); break;
    case ' ': *PushBytes(out,1) = spaceSubstitute; break;
    default:  *PushBytes(out,1) = toEscape[i]; break;
    }
  }

  String res = PointArena(out,mark);
  return res;
}

void PrintEscapedString(String toEscape,char spaceSubstitute){
  for(int i = 0; i < toEscape.size; i++){
    switch(toEscape[i]){
    case '\a': puts("\\a"); break;
    case '\b': puts("\\b"); break;
    case '\r': puts("\\r"); break;
    case '\f': puts("\\f"); break;
    case '\t': puts("\\t"); break;
    case '\n': puts("\\n"); break;
    case '\0': puts("\\0"); break;
    case '\v': puts("\\v"); break;
    case ' ':  printf("%c",spaceSubstitute); break;
    default:   printf("%c",toEscape[i]); break;
    }
  }
}

String GetAbsolutePath(const char* path,Arena* out){
  fs::path canonical = fs::weakly_canonical(path);
  String res = PushString(out,"%s",canonical.c_str());
  return res;
}

Array<int> GetNonZeroIndexes(Array<int> arr,Arena* out){
  Byte* mark = MarkArena(out);
  for(int i = 0; i < arr.size; i++){
    if(arr[i])
      *PushStruct<int>(out) = i;
  }

  return PointArray<int>(out,mark);
}

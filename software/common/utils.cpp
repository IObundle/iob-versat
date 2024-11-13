#include "utils.hpp"

#include <dirent.h>

#include <filesystem>
namespace fs = std::filesystem;

FILE* OpenFileAndCreateDirectories(String path,const char* format,FilePurpose purpose){
  char buffer[PATH_MAX];
  memset(buffer,0,PATH_MAX);

  for(int i = 0; i < path.size; i++){
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

  FILE* file = OpenFile(path,format,purpose);
  if(file == nullptr){
    printf("Failed to open file (%d): %.*s\n",errno,UNPACK_SS(path));
    DEBUG_BREAK();
    exit(-1);
  }
  
  return file;
}

Opt<Array<String>> GetAllFilesInsideDirectory(String dirPath,Arena* out){
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
  auto mark = StartString(out);

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

  String res = EndString(mark);
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
  DynamicArray<int> array = StartArray<int>(out);
  for(int i = 0; i < arr.size; i++){
    if(arr[i])
      *array.PushElem() = i;
  }

  return EndArray(array);
}

String JoinStrings(Array<String> strings,String separator,Arena* out){
  if(strings.size == 1){
    return PushString(out,strings[0]);
  }

  bool first = true;
  auto builder = StartString(out);
  for(String str : strings){
    if(first){
      first = false;
    } else {
      PushString(out,separator);
    }

    PushString(out,str);
  }

  return EndString(builder);
}

#include "utils.hpp"
#include "memory.hpp"
#include "utilsCore.hpp"

//#include "parser.hpp"
#include "signal.h"
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
    printf("Failed to open file (%d): %.*s\n",errno,UN(path));
    NOT_IMPLEMENTED("Probably better to return null and let code handle it");
    exit(-1);
  }
  
  return file;
}

String TrimWhitespaces(String in){
  if(in.data == nullptr || in.size == 0){
    return in;
  }
  
  const char* start = in.data;
  const char* end = &in.data[in.size-1];

  while(std::isspace(*start) && start < end) ++start;
  while(std::isspace(*end) && end > start) --end;

  String res = {};
  res.data = start;
  res.size = end - start + 1;

  return res;
}

static Array<String> SplitPath(String path,Arena* out){
  Array<String> res = Split(path,'/',out);
  for(String& s : res){
    s = TrimWhitespaces(s);
  }
  return res;
}

String GetCommonPath(String path1,String path2,Arena* out){
  TEMP_REGION(temp,out);

  // TODO: This function is currently kinda hardcoded to expect both inputs to start in the same folder.
  //       This is also important for the return path, because I want the result of this function to start in the same folder.
  //       If paths differ on their beginning we could fix this function by first finding the absolute path of each and start from there, but at that point I would need to see what would we do about the format of the result for that case
  
  Array<String> split1 = SplitPath(path1,temp);
  Array<String> split2 = SplitPath(path2,temp);

  auto builder = StartString(temp);
  for(int i = 0; i < MIN(split1.size,split2.size); i++){
    if(i != 0){
      builder->PushString("/");
    }
    if(CompareString(split1[i],split2[i])){
      builder->PushString(split1[i]);
    }
  }

  return EndString(out,builder);
}

String OS_NormalizePath(String in,Arena* out){
  fs::path path(CS(in));

  path = fs::absolute(path);
  path = path.lexically_normal();

  String res = PushString(out,"%s",path.c_str());

  if(res.data[res.size - 1] == '/'){
    res.size -= 1;
  }

  return res;
}

Opt<Array<String>> GetAllFilesInsideDirectory(String dirPath,Arena* out){
   DIR* dir = opendir(StaticFormat("%.*s",UN(dirPath))); // Make sure it's zero terminated

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
  TEMP_REGION(temp,out);

  auto builder = StartString(out);

  for(int i = 0; i < toEscape.size; i++){
    switch(toEscape[i]){
    case '\a': builder->PushString("\\a"); break;
    case '\b': builder->PushString("\\b"); break;
    case '\r': builder->PushString("\\r"); break;
    case '\f': builder->PushString("\\f"); break;
    case '\t': builder->PushString("\\t"); break;
    case '\n': builder->PushString("\\n"); break;
    case '\0': builder->PushString("\\0"); break;
    case '\v': builder->PushString("\\v"); break;
    case ' ': builder->PushChar(spaceSubstitute); break;
    default:  builder->PushChar(toEscape[i]); break;
    }
  }

  String res = EndString(out,builder);
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

String GetAbsolutePath(String path,Arena* out){
  char buffer[PATH_MAX+1];
  char* ptr = realpath(StaticFormat("%.*s",UN(path)),buffer);

  String res = PushString(out,"%s",ptr);
  return res;
}

String ReprMemorySize(size_t val,Arena* out){
  if(val < 1024){
    return PushString(out,"%d bytes",(int) val);
  } else if(val < Megabyte(1)){
    int leftover = val % 1024;
    return PushString(out,"%d.%.3d kilobytes",(int) (val / 1024),leftover);
  } else if(val < Gigabyte(1)){
    int leftover = val % (1024 * 1024);
    return PushString(out,"%d.%.3d megabytes",(int) (val / (1024 * 1024)),leftover);
  } else {
    int leftover = val % (1024 * 1024 * 1024);
    return PushString(out,"%d.%.3d gigabytes",(int) (val / (1024 * 1024 * 1024)),leftover);
  }
  NOT_IMPLEMENTED("");
  return {};
}

String JoinStrings(Array<String> strings,String separator,Arena* out){
  TEMP_REGION(temp,out);
  bool first = true;
  auto builder = StartString(temp);
  for(String str : strings){
    if(first){
      first = false;
    } else {
      builder->PushString(separator);
    }

    builder->PushString(str);
  }

  return EndString(out,builder);
}

String JoinStrings(ArenaList<String>* strings,String separator,Arena* out){
  TEMP_REGION(temp,out);
  bool first = true;
  auto builder = StartString(temp);
  for(String str : strings){
    if(first){
      first = false;
    } else {
      builder->PushString(separator);
    }

    builder->PushString(str);
  }

  return EndString(out,builder);
}

GenericArenaListIterator IterateArenaList(void* list,int sizeOfType,int alignmentOfType){
  GenericArenaListIterator res = {};

  SingleLink<int>* head = *(SingleLink<int>**) list;

  res.ptr = head;
  res.sizeOfType = sizeOfType;
  res.alignmentOfType = alignmentOfType;

  return res;
}

bool HasNext(GenericArenaListIterator iter){
  return (iter.ptr != nullptr);
}

void* Next(GenericArenaListIterator& iter){
  char* view = (char*) iter.ptr;
  view += sizeof(void*);
  
  iter.ptr = iter.ptr->next;
  
  return view;
}

GenericArenaDoubleListIterator IterateArenaDoubleList(void* list,int sizeOfType,int alignmentOfType){
  GenericArenaDoubleListIterator res = {};

  DoubleLink<int>* head = *(DoubleLink<int>**) list;

  res.ptr = head;
  res.sizeOfType = sizeOfType;
  res.alignmentOfType = alignmentOfType;

  return res;
}

bool HasNext(GenericArenaDoubleListIterator iter){
  return (iter.ptr != nullptr);
}

void* Next(GenericArenaDoubleListIterator& iter){
  char* view = (char*) iter.ptr;
  view += sizeof(void*) * 2;
  
  iter.ptr = iter.ptr->next;
  
  return view;
}

#include "filesystem.hpp"
#include "memory.hpp"
#include "utils.hpp"

#include <dirent.h>

#include "embeddedData.hpp"
#include "utilsCore.hpp"

static Arena storeFileInfoArena = {};
static ArenaList<FileInfo>* storeFileInfo;

const char* FilePurpose_Name(FilePurpose p){
  FULL_SWITCH(p){
  case FilePurpose_VERILOG_COMMON_CODE: return "VERILOG_COMMON_CODE";
  case FilePurpose_VERILOG_CODE: return "VERILOG_CODE";
  case FilePurpose_VERILOG_INCLUDE: return "VERILOG_INCLUDE";
  case FilePurpose_MAKEFILE: return "MAKEFILE";
  case FilePurpose_SOFTWARE: return "SOFTWARE";
  case FilePurpose_SCRIPT: return "SCRIPT";
  case FilePurpose_MISC: return "MISC";
  case FilePurpose_DEBUG_INFO: return "DEBUG_INFO";
  case FilePurpose_READ_CONTENT: return "READ_CONTENT";
  }
  NOT_POSSIBLE();
}

static void CheckOrInitArena(){
  if(storeFileInfoArena.mem == nullptr){
    storeFileInfoArena = InitArena(Megabyte(1)); // Simple and effective, more robust code would probably change to a growable arena or something similar.
    storeFileInfo = PushList<FileInfo>(&storeFileInfoArena);
  }
}

FILE* OpenFile(String filepath,const char* mode,FilePurpose purpose){
  CheckOrInitArena();

  char pathBuffer[4096]; // Because C calls require null terminated strings, need to copy to some space to append null terminator

  if(filepath.size >= 4095){
    NOT_IMPLEMENTED("Not sure if worth to implement something here or not");
    return nullptr;
  }

  int size = MIN(filepath.size,4095);
  strncpy(pathBuffer,filepath.data,size);
  pathBuffer[size] = '\0';

  FileOpenMode fileMode = FileOpenMode_NULL;
  if(mode[0] == '\0'){
    Assert("[OpenFile] Mode is empty: %s");
  } else if(mode[0] == 'w' && mode[1]  == '\0'){
    fileMode = FileOpenMode_WRITE;
  } else if(mode[0] == 'r' && mode[1]  == '\0'){
    fileMode = FileOpenMode_READ;
  } else if((mode[0] == 'w' && mode[1]  == 'r' && mode[2]  == '\0')||
            (mode[0] == 'r' && mode[1]  == 'w' && mode[2]  == '\0')){
    Assert(false); // NOTE: No code uses this and for now we are stopping it. 
    //fileMode = FileOpenMode_READ_WRITE;
  }

  Assert(fileMode);
  
  FILE* file = fopen(pathBuffer,mode);

  FileInfo* info = storeFileInfo->PushElem();
  info->filepath = PushString(&storeFileInfoArena,"%s",pathBuffer); // Use pathBuffer instead of filepath to make sure that we got the actual path used for the call
  info->mode = fileMode;
  info->purpose = purpose;
  info->wasOpenSucessful = (file != nullptr);

  return file;
}

Array<FileInfo> CollectAllFilesInfo(Arena* out){
  return PushArray(out,storeFileInfo);
}

struct {
  Arena* arena;
  TrieMap<String,FileContent>* fileContents;
  ArenaList<FileContent>* stringFiles;
  int currentId;
} FILE_State;

String PushFile(Arena* out,FILE* file){
  String res;
  long int size = GetFileSize(file);

  AlignArena(out,alignof(void*));

  Byte* mem = PushBytes(out,size + 1);
  int amountRead = fread(mem,sizeof(Byte),size,file);

  if(amountRead != size){
    fprintf(stderr,"Memory PushFile failed to read entire file\n");
    exit(-1);
  }

  res.size = size;
  res.data = (const char*) mem;
  mem[size] = '\0';

  return res;
}

String PushFile(Arena* out,String filepath){
  return PushFile(out,StaticFormat("%.*s",UN(filepath)));
}

//TODO: Replace return with Optional. Handle errors
String PushFile(Arena* out,const char* filepath){
  FILE* file = OpenFile(filepath,"r",FilePurpose_READ_CONTENT);
  DEFER_CLOSE_FILE(file);
  
  if(!file){
    String res = {};
    printf("Failed to open file: %s\n",filepath);
    NOT_IMPLEMENTED("Need to return opt and let code handle this instead. TODO");
    res.size = -1;
    return res;
  }

  String res = PushFile(out,file);

  return res;
}

void FILE_Init(){
  static Arena arenaInst = InitArena(Megabyte(4));
  FILE_State.arena = &arenaInst;
  FILE_State.fileContents = PushTrieMap<String,FileContent>(FILE_State.arena);
  FILE_State.stringFiles = PushList<FileContent>(FILE_State.arena);

  for(FileContent& file : defaultVerilogUnits){
    file.id.id = FILE_State.currentId++;
    file.state = FileContentState_OK;
  }

  for(FileContent& file : defaultVerilogFiles){
    file.id.id = FILE_State.currentId++;
    file.state = FileContentState_OK;
  }
}

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

FileContent GetContentsOfFile(String filepath,FilePurpose purpose){
  for(FileContent file : defaultVerilogUnits){
    if(filepath == file.fileName){
      return file;
    }
  }

  for(FileContent file : defaultVerilogFiles){
    if(filepath == file.fileName){
      return file;
    }
  }

  FileContent* alreadyOpened = FILE_State.fileContents->Get(filepath);

  if(alreadyOpened){
    return *alreadyOpened;
  }

  String savedPath = PushString(FILE_State.arena,filepath);

  FileContent toInsert = {};
  toInsert.fileName = GetFilename(savedPath);
  toInsert.originalRelativePath = "";
  toInsert.commonFolder = "";
  toInsert.state = FileContentState_OK;

  FILE* file = OpenFile(filepath,"r", purpose);
  if(!file){
    toInsert.content = "";
    toInsert.state = FileContentState_FAILED_TO_LOAD;
  } else {
    String content = PushFile(FILE_State.arena,file);
    toInsert.content = content;
  }

  FILE_State.fileContents->Insert(savedPath,toInsert);
  
  return toInsert;
}

FileContent NullFileContent = {};

FileContent FILE_FileContentsFromId(FILE_Handle id){
  for(FileContent file : defaultVerilogUnits){
    if(file.id.id == id.id){
      return file;
    }
  }

  for(FileContent file : defaultVerilogFiles){
    if(file.id.id == id.id){
      return file;
    }
  }

  for(Pair<String,FileContent> p : FILE_State.fileContents){
    if(p.second.id.id == id.id){
      return p.second;
    }
  }

  return NullFileContent;
}

FileContent FILE_GetFileContentFromString(String content){
  FileContent* file = FILE_State.stringFiles->PushElem();

  file->content = content;
  file->id.id = FILE_State.currentId++;

  return *file;
}

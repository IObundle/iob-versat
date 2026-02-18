#include "filesystem.hpp"
#include "utils.hpp"

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
  } END_SWITCH()
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

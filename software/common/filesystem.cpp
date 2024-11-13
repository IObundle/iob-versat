#include "filesystem.hpp"
#include "utils.hpp"

static Arena storeFileInfoArena = {};
static ArenaList<FileInfo>* storeFileInfo;

static void CheckOrInitArena(){
  if(storeFileInfoArena.mem == nullptr){
    storeFileInfoArena = InitArena(Megabyte(1)); // Simple and effective, more robust code would probably change to a growable arena or something similar.
    storeFileInfo = PushArenaList<FileInfo>(&storeFileInfoArena);
  }
}

FILE* OpenFile(String filepath,const char* mode,FilePurpose purpose){
  CheckOrInitArena();

  char pathBuffer[4096]; // Because C calls require null terminated strings, need to copy to some space to append null terminator

  if(filepath.size >= 4095){
    DEBUG_BREAK(); // Is larger than 4096 a possibility that we need to handle? Probably some bug or something
    return nullptr;
  }

  int size = std::min(filepath.size,4095);
  strncpy(pathBuffer,filepath.data,size);
  pathBuffer[size] = '\0';

  FileOpenMode fileMode;
  if(mode[0] == '\0'){
    Assert("[OpenFile] Mode is empty: %s");
  } else if(mode[0] == 'w' && mode[1]  == '\0'){
    fileMode = FileOpenMode_WRITE;
  } else if(mode[0] == 'r' && mode[1]  == '\0'){
    fileMode = FileOpenMode_READ;
  } else if((mode[0] == 'w' && mode[1]  == 'r' && mode[2]  == '\0')||
            (mode[0] == 'r' && mode[1]  == 'w' && mode[2]  == '\0')){
    fileMode = FileOpenMode_READ_WRITE;
  }
 
  FILE* file = fopen(pathBuffer,mode);

  FileInfo* info = storeFileInfo->PushElem();
  info->filepath = PushString(&storeFileInfoArena,"%s",pathBuffer); // Use pathBuffer instead of filepath to make sure that we got the actual path using for the call
  info->mode = fileMode;
  info->purpose = purpose;
  info->wasOpenSucessful = (file != nullptr);

  return file;
}

Array<FileInfo> CollectAllFilesInfo(Arena* out){
  return PushArrayFromList(out,storeFileInfo);
}

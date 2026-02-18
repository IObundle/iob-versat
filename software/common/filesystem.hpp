#pragma once

#include "utilsCore.hpp"

struct Arena;

// The main goal of this file is to provide a one place for which every file creation goes through. That way it is easier to keep track of how many files are operated on, the reason, and the ability to put debug code and see which portions of the code are opening files and so on. Furthermore it is helpful when implementing certain features knowing all the files our program created.

enum FilePurpose{
  FilePurpose_VERILOG_COMMON_CODE,
  FilePurpose_VERILOG_CODE,
  FilePurpose_VERILOG_INCLUDE,
  FilePurpose_MAKEFILE,
  FilePurpose_SOFTWARE,
  FilePurpose_SCRIPT,
  FilePurpose_MISC,
  FilePurpose_READ_CONTENT,
  FilePurpose_DEBUG_INFO
};

const char* FilePurpose_Name(FilePurpose p);

enum FileOpenMode{
  FileOpenMode_NULL = 0,
  FileOpenMode_READ,
  FileOpenMode_WRITE,
  FileOpenMode_READ_WRITE // Add more as necessary
};

struct FileInfo{
  String filepath;
  FileOpenMode mode;
  FilePurpose purpose;
  bool wasOpenSucessful;
};

FILE* OpenFile(String filepath,const char* mode,FilePurpose purpose);
Array<FileInfo> CollectAllFilesInfo(Arena* out);

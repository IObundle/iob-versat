#pragma once

#include "utilsCore.hpp"
#include "memory.hpp"

struct AddressGenDef;
struct FUDeclaration;

enum VersatOperationMode{
  VersatOperationMode_GENERATE_ACCELERATOR,
  VersatOperationMode_GENERATE_TESTBENCH
};

struct Options{
  Array<String> verilogFiles;
  Array<String> extraSources;
  Array<String> includePaths;
  Array<String> unitFolderPaths;
  
  String hardwareOutputFilepath;
  String softwareOutputFilepath;
  String debugPath;

  String prefixIObPort;
  
  String generetaSourceListName; // TODO: Not yet implemented
  
  String specificationFilepath;
  String topName;
  int databusDataSize; // AXI_DATA_W

  bool addInputAndOutputsToTop;
  bool debug;
  bool shadowRegister;
  bool architectureHasDatabus;
  bool useFixedBuffers;
  bool generateFSTFormat;
  bool disableDelayPropagation;
  bool useDMA;
  bool exportInternalMemories;
  
  bool extraIOb;
  bool useSymbolAddress; // If the system removes the LSB bits of the address (alignment info) and if we must generate code to account for that.

  VersatOperationMode opMode;
};

enum GraphDotFormat : int;

struct DebugState{
  GraphDotFormat dotFormat;
  bool outputGraphs;
  bool outputConsolidationGraphs;
  bool outputVCD;
};

extern Options globalOptions;
extern DebugState globalDebug;

// Basically any data that is allocated once and preferably read-only just dump it in here.
extern Arena* globalPermanent;

extern Pool<FUDeclaration> globalDeclarations;

Options DefaultOptions(Arena* out);

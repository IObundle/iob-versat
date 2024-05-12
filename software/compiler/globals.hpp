#pragma once

#include "utils.hpp"

struct Options{
  Array<String> verilogFiles;
  Array<String> extraSources;
  Array<String> includePaths;
  Array<String> unitPaths;
  
  String hardwareOutputFilepath;
  String softwareOutputFilepath;
  String verilatorRoot;

  const char* specificationFilepath;
  const char* topName;
  int addrSize; // AXI_ADDR_W - used to be bitSize
  int dataSize; // AXI_DATA_W

  bool addInputAndOutputsToTop;
  bool debug;
  bool shadowRegister;
  bool architectureHasDatabus;
  bool useFixedBuffers;
  bool generateFSTFormat;
  bool noDelayPropagation;
  bool useDMA;
  bool exportInternalMemories;
};

struct DebugState{
  uint dotFormat;
  bool outputGraphs;
  bool outputAccelerator;
  bool outputVersat;
  bool outputVCD;
  bool outputAcceleratorInfo;
  bool useFixedBuffers;
};

extern Options globalOptions;
extern DebugState globalDebug;
extern Arena* globalPermanent;

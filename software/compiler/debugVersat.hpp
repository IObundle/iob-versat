#pragma once

//#include <functional>
#include <cstdio>

#include "debug.hpp"
#include "utils.hpp"

#include "delayCalculation.hpp"

struct Accelerator;
struct Arena;
struct FUInstance;
struct Edge;

// Useful to have a bit of memory for debugging
extern Arena* debugArena;

// ============================================================================
// Graph printing

// When adding new colors, also add info in the .cpp file
// TODO: We probably want to move this to meta data
enum Color{
  Color_BLACK,
  Color_BLUE,
  Color_RED,
  Color_GREEN,
  Color_YELLOW
};

struct GraphPrintingNodeInfo{
  String name;
  String content;
  Color color;
};

struct GraphPrintingEdgeInfo{
  String content;
  Color color;
  String firstNode;
  String secondNode;
};

struct GraphPrintingContent{
  String graphLabel;
  Array<GraphPrintingNodeInfo> nodes;
  Array<GraphPrintingEdgeInfo> edges;
};

struct GraphInfo{
  String content;
  Color color;
};

typedef GraphInfo (*NodeContent)(FUInstance*,Arena* out);
typedef GraphInfo (*EdgeContent)(Edge*,Arena* out);

extern NodeContent defaultNodeContent;
extern EdgeContent defaultEdgeContent;

GraphPrintingContent GeneratePrintingContent(Accelerator* accel,NodeContent nodeFunction,EdgeContent edgeFunction,Arena* out);

Color DefaultNodeColor(FUInstance* node);
GraphPrintingContent GenerateDefaultPrintingContent(Accelerator* accel,Arena* out);

String GenerateDotGraph(GraphPrintingContent content,Arena* out);

void OutputContentToFile(String filepath,String content);

// ============================================================================
// Obtain debug path to output files to

// folderName can be empty string, file is created inside debug folder directly. 
String PushDebugPath(Arena* out,String folderName,String fileName);

// ============================================================================
// Debug path - Global way of defining a path tree for debug info

struct DebugPathMarker{
  DebugPathMarker(String name);
  ~DebugPathMarker();
};

#define DEBUG_PATH(NAME) DebugPathMarker _marker(__LINE__)(NAME)

String GetDebugRegionFilepath(String filename,Arena* out);

void DebugRegionOutputDotGraph(Accelerator* accel,String fileName);
void DebugRegionOutputLatencyGraph(Accelerator* accel,NodeDelay* nodeDelay,PortDelay* portDelay,EdgeDelay* edgeToDelay,String fileName);

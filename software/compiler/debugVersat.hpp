#pragma once

#include <functional>
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

typedef std::function<GraphInfo(FUInstance*,Arena* out)> NodeContent;
typedef std::function<GraphInfo(Edge*,Arena* out)> EdgeContent;

extern NodeContent defaultNodeContent;
extern EdgeContent defaultEdgeContent;

GraphPrintingContent GeneratePrintingContent(Accelerator* accel,NodeContent nodeFunction,EdgeContent edgeFunction,Arena* out);

Color DefaultNodeColor(FUInstance* node);
GraphPrintingContent GenerateDefaultPrintingContent(Accelerator* accel,Arena* out);

String GenerateDotGraph(GraphPrintingContent content,Arena* out);

void OutputDebugDotGraph(Accelerator* accel,String fileName);
void OutputDebugDotGraph(Accelerator* accel,String fileName,FUInstance* highlight);
void OutputDebugDotGraph(Accelerator* accel,String fileName,Set<FUInstance*>* highlight);

GraphPrintingContent GenerateLatencyDotGraph(AccelInfoIterator top,Array<int> orderToIndex,Array<DelayInfo> nodeLatencyByOrder,Array<DelayInfo> edgeLatency,Arena* out);

void OutputContentToFile(String filepath,String content);

// ============================================================================
// Obtain debug path to output files to

// folderName can be empty string, file is created inside debug folder directly. 
String PushDebugPath(Arena* out,String folderName,String fileName);

// For this overload, no string can be empty, otherwise error
String PushDebugPath(Arena* out,String folderName,String subFolder,String fileName);

// ============================================================================
// Debug path - Global way of defining a path tree for debug info

struct DebugPathMarker{
  DebugPathMarker(String name);
  ~DebugPathMarker();
};

#define DEBUG_PATH(NAME) DebugPathMarker _marker(__LINE__)(NAME)

String GetDebugRegionFilepath(String filename,Arena* out);

void DebugRegionOutputDotGraph(Accelerator* accel,String fileName);
void DebugRegionLatencyGraph(AccelInfoIterator top,Array<int> orderToIndex,Array<DelayInfo> nodeLatencyByOrder,Array<DelayInfo> edgeLatency,String filename);

#pragma once

#include <functional>

#include "utils.hpp"

struct Accelerator;
struct FUInstance;
struct Edge;

// When adding new colors, also add 
enum Color{
  Color_BLACK,
  Color_BLUE,
  Color_RED,
  Color_GREEN,
  Color_YELLOW
};

struct GraphPrintingNodeInfo{
  //String outputName;
  String name;
  String content;
  Color color;
};

struct GraphPrintingEdgeInfo{
  String content;
  Color color;
  String first;
  String second;
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

GraphPrintingContent GeneratePrintingContent(Accelerator* accel,NodeContent nodeFunction,EdgeContent edgeFunction,Arena* out,Arena* temp);

Color DefaultNodeColor(FUInstance* node);
GraphPrintingContent GenerateDefaultPrintingContent(Accelerator* accel,Arena* out,Arena* temp);

String GenerateDotGraph(Accelerator* accel,GraphPrintingContent content,Arena* out,Arena* temp);

void OutputDebugDotGraph(Accelerator* accel,String fileName,Arena* temp);
void OutputDebugDotGraph(Accelerator* accel,String fileName,FUInstance* highlight,Arena* temp);
void OutputDebugDotGraph(Accelerator* accel,String fileName,Set<FUInstance*>* highlight,Arena* temp);

#pragma once

#include <functional>

#include "utils.hpp"

struct Accelerator;
struct FUInstance;
struct Edge;

// When adding new colors, also add 
enum GraphPrintingColor{
  GraphPrintingColor_BLACK,
  GraphPrintingColor_BLUE,
  GraphPrintingColor_RED,
  GraphPrintingColor_GREEN,
  GraphPrintingColor_YELLOW
};

struct GraphPrintingNodeInfo{
  //String outputName;
  String name;
  String content;
  GraphPrintingColor color;
};

struct GraphPrintingEdgeInfo{
  String content;
  GraphPrintingColor color;
  String first;
  String second;
};

struct GraphPrintingContent{
  String graphLabel;
  Array<GraphPrintingNodeInfo> nodes;
  Array<GraphPrintingEdgeInfo> edges;
};

typedef std::function<Pair<String,GraphPrintingColor>(FUInstance*,Arena* out)> NodeContent;
typedef std::function<Pair<String,GraphPrintingColor>(Edge*,Arena* out)> EdgeContent;

extern NodeContent defaultNodeContent;
extern EdgeContent defaultEdgeContent;

GraphPrintingContent GeneratePrintingContent(Accelerator* accel,NodeContent nodeFunction,EdgeContent edgeFunction,Arena* out,Arena* temp);

GraphPrintingColor DefaultNodeColor(FUInstance* node);
GraphPrintingContent GenerateDefaultPrintingContent(Accelerator* accel,Arena* out,Arena* temp);

String GenerateDotGraph(Accelerator* accel,GraphPrintingContent content,Arena* out,Arena* temp);


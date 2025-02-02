#pragma once

#include <functional>

#include "utils.hpp"

struct Accelerator;
struct FUInstance;
struct Edge;

// When adding new colors, also add info in the .cpp file
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
  String first; // Name of first node
  String second; // Name of second node
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

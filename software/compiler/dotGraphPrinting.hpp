#pragma once

#include <functional>

#include "utils.hpp"

struct Accelerator;
struct InstanceNode;
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

typedef std::function<Pair<String,GraphPrintingColor>(InstanceNode*,Arena* out)> NodeContent;
typedef std::function<Pair<String,GraphPrintingColor>(Edge*,Arena* out)> EdgeContent;

//typedef Pair<String,GraphPrintingColor> (*NodeContent)(InstanceNode*,Arena* out);
//typedef Pair<String,GraphPrintingColor> (*EdgeContent)(Edge*,Arena* out);

extern NodeContent defaultNodeContent;
extern EdgeContent defaultEdgeContent;

GraphPrintingContent GeneratePrintingContent(Accelerator* accel,NodeContent nodeFunction,EdgeContent edgeFunction,Arena* out,Arena* temp);

GraphPrintingColor DefaultNodeColor(InstanceNode* node);
GraphPrintingContent GenerateDefaultPrintingContent(Accelerator* accel,Arena* out,Arena* temp);

String GenerateDotGraph(Accelerator* accel,GraphPrintingContent content,Arena* out,Arena* temp);


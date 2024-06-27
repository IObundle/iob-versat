#pragma once

#include "utils.hpp"

struct Accelerator;
struct InstanceNode;

enum GraphPrintingColor{
  GraphPrintingColor_BLACK,
  GraphPrintingColor_BLUE,
  GraphPrintingColor_RED,
  GraphPrintingColor_GREEN
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

GraphPrintingColor DefaultNodeColor(InstanceNode* node);
GraphPrintingContent GenerateDefaultPrintingContent(Accelerator* accel,Arena* out,Arena* temp);

String GenerateDotGraph(Accelerator* accel,GraphPrintingContent content,Arena* out,Arena* temp);


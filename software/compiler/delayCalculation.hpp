#pragma once

#include "dotGraphPrinting.hpp"
#include "utils.hpp"
#include "accelerator.hpp"
#include "configurations.hpp"

// TODO: Maybe it would be best if we just made the change to versat to do delay calculation in a port by port basis instead of just looking at the units themselves. This would extract a little bit more performace, use less hardware since we can have better fixed buffer allocations and we could even simplify a bit of the code, since the "out" unit already requires port based delay calculations.

typedef Hashmap<Edge,DelayInfo> EdgeDelay;
typedef Hashmap<PortInstance,DelayInfo> PortDelay;
typedef Hashmap<FUInstance*,DelayInfo> NodeDelay;

struct SimpleEdge{
  int outIndex;
  int outPort;
  int inIndex;
  int inPort;
};

typedef Hashmap<SimpleEdge,DelayInfo> SimpleEdgeDelay;
typedef Hashmap<SimplePortInstance,DelayInfo> SimplePortDelay;
typedef Hashmap<int,DelayInfo> SimpleNodeDelay;

struct CalculateDelayResult{
  EdgeDelay* edgesDelay;
  PortDelay* portDelay;
  NodeDelay* nodeDelay;
};

struct SimpleCalculateDelayResult{
  Array<DelayInfo> nodeDelayByOrder; // Indexed by order
  Array<DelayInfo> edgesDelay;
  Array<Array<DelayInfo>> inputPortDelayByOrder;
};

Array<int> ExtractInputDelays(Accelerator* accel,CalculateDelayResult delays,int mimimumAmount,Arena* out,Arena* temp);
Array<int> ExtractOutputLatencies(Accelerator* accel,CalculateDelayResult delays,Arena* out,Arena* temp);

SimpleCalculateDelayResult CalculateDelay(AccelInfoIterator top,Arena* out,Arena* temp);

// TODO: This is very bad performance wise. We are internally creating an AccelInfo and then extracting the values from the delay calculating.
//       Also, cannot call this function for merge accelerators, most of them will lead to errors.
//       Need to simplefy further (Replace CalculateDelayResult with the simple version).
CalculateDelayResult CalculateDelay(Accelerator* accel,Arena* out);

GraphPrintingContent GenerateLatencyDotGraph(AccelInfoIterator top,Array<int> orderToIndex,Array<DelayInfo> nodeDelay,Array<DelayInfo> edgeDelay,Arena* out,Arena* temp);

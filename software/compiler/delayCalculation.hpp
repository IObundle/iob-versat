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

// Nodes indexed by order, edges indexed by index returned from EdgeIterator
struct SimpleCalculateDelayResult{
  Array<DelayInfo> nodeBaseLatencyByOrder;
  Array<DelayInfo> edgesExtraDelay;
  Array<Array<DelayInfo>> inputPortBaseLatencyByOrder;
};

SimpleCalculateDelayResult CalculateDelay(AccelInfoIterator top,Arena* out);

// TODO: This is very bad performance wise. We are internally creating an AccelInfo and then extracting the values from the delay calculating.
//       Also, cannot call this function for merge accelerators, most of them will lead to errors.
//       Need to simplefy further (Replace CalculateDelayResult with the simple version).
CalculateDelayResult CalculateDelay(Accelerator* accel,Arena* out);

GraphPrintingContent GenerateLatencyDotGraph(AccelInfoIterator top,Array<int> orderToIndex,Array<DelayInfo> nodeLatencyByOrder,Array<DelayInfo> edgeLatency,Arena* out);

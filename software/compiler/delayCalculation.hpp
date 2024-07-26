#pragma once

#include "dotGraphPrinting.hpp"
#include "utils.hpp"
#include "accelerator.hpp"
#include "configurations.hpp"

typedef Hashmap<Edge,DelayInfo> EdgeDelay;
typedef Hashmap<PortInstance,DelayInfo> PortDelay;
typedef Hashmap<FUInstance*,DelayInfo> NodeDelay;

struct CalculateDelayResult{
  EdgeDelay* edgesDelay;
  PortDelay* portDelay;
  NodeDelay* nodeDelay;
};

Array<int> ExtractInputDelays(Accelerator* accel,CalculateDelayResult delays,int mimimumAmount,Arena* out,Arena* temp);
Array<int> ExtractOutputLatencies(Accelerator* accel,CalculateDelayResult delays,Arena* out,Arena* temp);

CalculateDelayResult CalculateDelay(Accelerator* accel,DAGOrderNodes order,Arena* out);
CalculateDelayResult CalculateDelay(Accelerator* accel,DAGOrderNodes order,Array<Partition> partitions,Arena* out);

// Nodes that can change the time where they start outputting data
Array<FUInstance*> GetNodesWithOutputDelay(Accelerator* accel,Arena* out);

CalculateDelayResult CalculateGlobalInitialLatency(Accelerator* accel);

void OutputDelayDebugInfo(Accelerator* accel,Arena* temp);

GraphPrintingContent GenerateDelayDotGraph(Accelerator* accel,CalculateDelayResult delay,Arena* out,Arena* temp);

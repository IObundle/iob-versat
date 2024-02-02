#pragma once

#include "versat.hpp"
#include "verilogParsing.hpp"

int ExternalMemoryByteSize(ExternalMemoryInterface* inter);
int ExternalMemoryByteSize(Array<ExternalMemoryInterface> interfaces); // Size of a simple memory mapping.

// TODO: Instead of versatComputedValues, could return something like a FUDeclaration
//       (or something that FUDeclaration would be composed off)
//       
VersatComputedValues ComputeVersatValues(Versat* versat,Accelerator* accel,bool useDMA);

#ifndef INCLUDED_VERSAT_ACCELERATOR_STATS
#define INCLUDED_VERSAT_ACCELERATOR_STATS

#include "versat.hpp"

int MemorySize(Array<ExternalMemoryInterface> interfaces); // Size of a simple memory mapping.
int NumberUnits(Accelerator* accel);

// TODO: Instead of versatComputedValues, could return something like a FUDeclaration
//       (or something that FUDeclaration would be composed off)
//       
VersatComputedValues ComputeVersatValues(Versat* versat,Accelerator* accel,bool useDMA);

#endif // INCLUDED_VERSAT_ACCELERATOR_STATS

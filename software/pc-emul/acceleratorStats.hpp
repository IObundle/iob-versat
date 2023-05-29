#ifndef INCLUDED_VERSAT_ACCELERATOR_STATS
#define INCLUDED_VERSAT_ACCELERATOR_STATS

#include "versatPrivate.hpp"

int MemorySize(Array<ExternalMemoryInterface> interfaces); // Size of a simple memory mapping.

int NumberUnits(Accelerator* accel);

VersatComputedValues ComputeVersatValues(Versat* versat,Accelerator* accel);

#endif // INCLUDED_VERSAT_ACCELERATOR_STATS
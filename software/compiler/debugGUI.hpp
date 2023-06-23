#ifndef INCLUDED_VERSAT_DEBUG_GUI
#define INCLUDED_VERSAT_DEBUG_GUI

#include "type.hpp"

struct Versat;
struct Accelerator;

void SetDebugValue(Value val);
void SetDebugAccelerator(Accelerator* accel);

void DebugGUI();
void DebugVersat(Versat* versat);

void DebugAccelerator(Accelerator* accel);

#if 0
void DebugAcceleratorPersist(Accelerator* accel);
void DebugAcceleratorStart(Accelerator* accel);
void DebugAcceleratorEnd(Accelerator* accel);
#endif

void DebugValue(Value val);

//void DebugAcceleratorRun(Accelerator* accel,int cycle);

#endif // INCLUDED_VERSAT_DEBUG_GUI

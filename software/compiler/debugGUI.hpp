#ifndef INCLUDED_VERSAT_DEBUG_GUI
#define INCLUDED_VERSAT_DEBUG_GUI

#include "type.hpp"

struct Versat;
struct Accelerator;

#ifdef VERSAT_DEBUG

void SetDebugValue(Value val);
void SetDebugAccelerator(Accelerator* accel);

void DebugGUI();
void DebugVersat(Versat* versat);

void DebugAccelerator(Accelerator* accel);

void DebugValue(Value val);

#endif

#endif // INCLUDED_VERSAT_DEBUG_GUI

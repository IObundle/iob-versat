#ifndef INCLUDED_VERSAT_DEBUG_GUI
#define INCLUDED_VERSAT_DEBUG_GUI

#include "type.hpp"

struct Versat;
struct Accelerator;

#ifdef x86
#define DebugGUI(...) ((void)0)
#define DebugAccelerator(...) ((void)0)
#define DebugAcceleratorPersist(...) ((void)0)
#define DebugAcceleratorStart(...) ((void)0)
#define DebugAcceleratorEnd(...) ((void)0)

#define DebugWindowValue(...) ((void)0)
#define DebugWindowAccelerator(...) ((void)0)

#define DebugValue(...) ((void)0)
#else

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
#endif

//void DebugAcceleratorRun(Accelerator* accel,int cycle);

#endif // INCLUDED_VERSAT_DEBUG_GUI

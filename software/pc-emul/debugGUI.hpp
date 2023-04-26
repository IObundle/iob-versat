#ifndef INCLUDED_VERSAT_DEBUG_GUI
#define INCLUDED_VERSAT_DEBUG_GUI

#include "type.hpp"
#include "versatPrivate.hpp"

#include <optional>

template<typename T>
using Optional = std::optional<T>;

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
void DebugGUI();
void DebugVersat(Versat* versat);

void DebugAccelerator(Accelerator* accel);
void DebugAcceleratorPersist(Accelerator* accel);
void DebugAcceleratorStart(Accelerator* accel);
void DebugAcceleratorEnd(Accelerator* accel);

void DebugWindowValue(const char* label,Value val);
void DebugWindowAccelerator(const char* label,Accelerator* accel);

void DebugValue(const char* windowName,Value val);
#endif

//void DebugAcceleratorRun(Accelerator* accel,int cycle);

#endif // INCLUDED_VERSAT_DEBUG_GUI

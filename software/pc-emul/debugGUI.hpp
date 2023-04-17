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
void DebugGUI(Arena* arena);
void DebugVersat(Versat* versat);

void DebugAccelerator(Accelerator* accel,Arena* arena);
void DebugAcceleratorPersist(Accelerator* accel,Arena* arena);
void DebugAcceleratorStart(Accelerator* accel,Arena* arena);
void DebugAcceleratorEnd(Accelerator* accel,Arena* arena);

void DebugWindowValue(const char* label,Value val);
void DebugWindowAccelerator(const char* label,Accelerator* accel);

void DebugValue(const char* windowName,Value val,Arena* arena);
#endif

//void DebugAcceleratorRun(Accelerator* accel,Arena* arena,int cycle);

#endif // INCLUDED_VERSAT_DEBUG_GUI

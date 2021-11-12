#ifndef INCLUDED_UNIT_VERILOG_WRAPPERS
#define INCLUDED_UNIT_VERILOG_WRAPPERS

#include "versat.h"

#ifdef __cplusplus
#define EXPORT extern "C"
#else
#define EXPORT
#endif

EXPORT int      AddExtraSize();
EXPORT int32_t* AddInitializeFunction(FUInstance* inst);
EXPORT int32_t* AddStartFunction(FUInstance* inst);
EXPORT int32_t* AddUpdateFunction(FUInstance* inst);

#endif //INCLUDED_UNIT_VERILOG_WRAPPERS

#ifndef INCLUDED_UNIT_VERILOG_WRAPPERS
#define INCLUDED_UNIT_VERILOG_WRAPPERS

#include "versat.h"

#ifdef __cplusplus
#define EXPORT extern "C"
#else
#define EXPORT
#endif

#define INSTANTIATE_CLASS
#include "unitData.h"
#undef INSTANTIATE_CLASS

// Verilated units
EXPORT FU_Type RegisterAdd(Versat* versat);
EXPORT FU_Type RegisterReg(Versat* versat);
EXPORT FU_Type RegisterMem(Versat* versat,int addr_w);
EXPORT FU_Type RegisterVRead(Versat* versat);
EXPORT FU_Type RegisterVWrite(Versat* versat);

// Software units
EXPORT FU_Type RegisterDebug(Versat* versat);

#endif //INCLUDED_UNIT_VERILOG_WRAPPERS


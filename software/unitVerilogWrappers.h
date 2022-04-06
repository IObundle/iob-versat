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
EXPORT FUDeclaration* RegisterAdd(Versat* versat);
EXPORT FUDeclaration* RegisterReg(Versat* versat);
EXPORT FUDeclaration* RegisterMem(Versat* versat,int addr_w);
EXPORT FUDeclaration* RegisterVRead(Versat* versat);
EXPORT FUDeclaration* RegisterVWrite(Versat* versat);

// Software units
EXPORT FUDeclaration* RegisterDebug(Versat* versat);
EXPORT FUDeclaration* RegisterDelay(Versat* versat);
EXPORT FUDeclaration* RegisterCircuitInput(Versat* versat);
EXPORT FUDeclaration* RegisterCircuitOutput(Versat* versat);

#endif //INCLUDED_UNIT_VERILOG_WRAPPERS


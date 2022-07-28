#ifndef INCLUDED_UNIT_VERILOG_WRAPPERS
#define INCLUDED_UNIT_VERILOG_WRAPPERS

#if 0

#include "versat.hpp"

#define INSTANTIATE_CLASS
#include "unitData.hpp"
#undef INSTANTIATE_CLASS

// Verilated units
FUDeclaration* RegisterAdd(Versat* versat);
FUDeclaration* RegisterReg(Versat* versat);
FUDeclaration* RegisterPipelineRegister(Versat* versat);
FUDeclaration* RegisterConst(Versat* versat);
FUDeclaration* RegisterMem(Versat* versat,int addr_w);
FUDeclaration* RegisterVRead(Versat* versat);
FUDeclaration* RegisterVWrite(Versat* versat);
FUDeclaration* RegisterPipelineRegister(Versat* versat);
FUDeclaration* RegisterMerge(Versat* versat);
FUDeclaration* RegisterMuladd(Versat* versat);
FUDeclaration* RegisterMux2(Versat* versat);

FUDeclaration* RegisterDebug(Versat* versat);
#endif

#endif //INCLUDED_UNIT_VERILOG_WRAPPERS


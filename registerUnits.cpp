#include "versat.hpp"
#include "versatPrivate.hpp"

#define IMPLEMENT_VERILOG_UNITS
#include "wrapper.inc"

void RegisterSpecificUnits(Versat* versat){
   RegisterAllVerilogUnitsVerilog(versat);
}
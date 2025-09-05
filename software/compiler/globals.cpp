#include "globals.hpp"

#include "utils.hpp"
#include "symbolic.hpp"
#include "verilogParsing.hpp"
#include "VerilogEmitter.hpp"

Options globalOptions = {};
DebugState globalDebug = {};

Arena* globalPermanent;

Options DefaultOptions(Arena* out){
  Options res = {};
  res.databusDataSize = 32;

  res.useFixedBuffers = true;
  res.shadowRegister = true; 

  res.hardwareOutputFilepath = "./versatOutput/hardware";
  res.softwareOutputFilepath = "./versatOutput/software";
  
#ifdef USE_FST_FORMAT
  res.generateFSTFormat = 1;
#endif
  
  res.debugPath = PushString(out,"%s/debug",GetCurrentDirectory()); // By default
  PushNullByte(out);
  
  return res;
}

SymbolicExpression* SYM_one;
SymbolicExpression* SYM_zero;
SymbolicExpression* SYM_eight;
SymbolicExpression* SYM_dataW;
SymbolicExpression* SYM_addrW;
SymbolicExpression* SYM_axiAddrW;
SymbolicExpression* SYM_axiDataW;
SymbolicExpression* SYM_delayW;
SymbolicExpression* SYM_lenW;
SymbolicExpression* SYM_axiStrobeW;
SymbolicExpression* SYM_dataStrobeW;

Array<VerilogPortSpec> INT_IOb;
Array<VerilogPortSpec> INT_IObFormat;

void InitializeDefaultData(Arena* perm){
  SYM_zero = PushLiteral(perm,0);
  SYM_one = PushLiteral(perm,1);
  SYM_eight = PushLiteral(perm,8);
  SYM_dataW = PushVariable(perm,"DATA_W");
  SYM_addrW = PushVariable(perm,"ADDR_W");
  SYM_axiAddrW = PushVariable(perm,"AXI_ADDR_W");
  SYM_axiDataW = PushVariable(perm,"AXI_DATA_W");
  SYM_delayW = PushVariable(perm,"DELAY_W");
  SYM_lenW = PushVariable(perm,"LEN_W");

  SYM_axiStrobeW = SymbolicDiv(PushVariable(perm,"AXI_DATA_W"),SYM_eight,perm);
  SYM_dataStrobeW = SymbolicDiv(PushVariable(perm,"DATA_W"),SYM_eight,perm);

  static VerilogPortSpec iobDatabus[] = {
    {"databus_ready",SYM_one,WireDir_INPUT},
    {"databus_valid",SYM_one,WireDir_OUTPUT},
    {"databus_addr",SYM_axiAddrW,WireDir_OUTPUT},
    {"databus_rdata",SYM_axiDataW,WireDir_INPUT,SpecialPortProperties_IsShared},
    {"databus_wdata",SYM_axiDataW,WireDir_OUTPUT},
    {"databus_wstrb",SYM_axiStrobeW,WireDir_OUTPUT},
    {"databus_len",SYM_lenW,WireDir_OUTPUT},
    {"databus_last",SYM_one,WireDir_INPUT},
  };
  INT_IOb = {iobDatabus,ARRAY_SIZE(iobDatabus)};

  INT_IObFormat = AppendSuffix(INT_IOb,"_%d",perm);
}

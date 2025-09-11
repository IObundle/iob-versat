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

Array<VerilogPortSpec> INT_IOb;
Array<VerilogPortSpec> INT_IObFormat;

void InitializeDefaultData(Arena* perm){
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

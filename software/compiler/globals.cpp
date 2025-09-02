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
  res.disableDelayPropagation = true;

  res.hardwareOutputFilepath = STRING("./versatOutput/hardware");
  res.softwareOutputFilepath = STRING("./versatOutput/software");
  
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
Array<VerilogPortSpec> INT_DPFormat;
Array<VerilogPortSpec> INT_TPFormat;

void InitializeDefaultData(Arena* perm){
  SYM_zero = PushLiteral(perm,0);
  SYM_one = PushLiteral(perm,1);
  SYM_eight = PushLiteral(perm,8);
  SYM_dataW = PushVariable(perm,S8("DATA_W"));
  SYM_addrW = PushVariable(perm,S8("ADDR_W"));
  SYM_axiAddrW = PushVariable(perm,S8("AXI_ADDR_W"));
  SYM_axiDataW = PushVariable(perm,S8("AXI_DATA_W"));
  SYM_delayW = PushVariable(perm,S8("DELAY_W"));
  SYM_lenW = PushVariable(perm,S8("LEN_W"));

  SYM_axiStrobeW = SymbolicDiv(PushVariable(perm,S8("AXI_DATA_W")),SYM_eight,perm);
  SYM_dataStrobeW = SymbolicDiv(PushVariable(perm,S8("DATA_W")),SYM_eight,perm);

  static VerilogPortSpec iobDatabus[] = {
    {S8("databus_ready"),SYM_one,WireDir_INPUT},
    {S8("databus_valid"),SYM_one,WireDir_OUTPUT},
    {S8("databus_addr"),SYM_axiAddrW,WireDir_OUTPUT},
    {S8("databus_rdata"),SYM_axiDataW,WireDir_INPUT,SpecialPortProperties_IsShared},
    {S8("databus_wdata"),SYM_axiDataW,WireDir_OUTPUT},
    {S8("databus_wstrb"),SYM_axiStrobeW,WireDir_OUTPUT},
    {S8("databus_len"),SYM_lenW,WireDir_OUTPUT},
    {S8("databus_last"),SYM_one,WireDir_INPUT},
  };
  INT_IOb = {iobDatabus,ARRAY_SIZE(iobDatabus)};

  INT_IObFormat = AppendSuffix(INT_IOb,S8("_%d"),perm);
  
  static VerilogPortSpec dpFormat[] = {
    {S8("addr_%d_port_0"),SYM_addrW,WireDir_OUTPUT},
    {S8("out_%d_port_0"),SYM_dataW,WireDir_OUTPUT},
    {S8("in_%d_port_0"),SYM_dataW,WireDir_INPUT},
    {S8("enable_%d_port_0"),SYM_one,WireDir_OUTPUT},
    {S8("write_%d_port_0"),SYM_one,WireDir_OUTPUT},
    {S8("addr_%d_port_1"),SYM_addrW,WireDir_OUTPUT},
    {S8("out_%d_port_1"),SYM_dataW,WireDir_OUTPUT},
    {S8("in_%d_port_1"),SYM_dataW,WireDir_INPUT},
    {S8("enable_%d_port_1"),SYM_one,WireDir_OUTPUT},
    {S8("write_%d_port_1"),SYM_one,WireDir_OUTPUT},
  };
  INT_DPFormat = {dpFormat,ARRAY_SIZE(dpFormat)};

  static VerilogPortSpec tpFormat[] = {
    {S8("addr_out_%d"),SYM_addrW,WireDir_OUTPUT},
    {S8("addr_in_%d"),SYM_addrW,WireDir_OUTPUT},
    {S8("write_%d"),SYM_one,WireDir_OUTPUT},
    {S8("read_%d"),SYM_one,WireDir_OUTPUT},
    {S8("data_in_%d"),SYM_dataW,WireDir_INPUT},
    {S8("data_out_%d"),SYM_dataW,WireDir_OUTPUT}
  };
  INT_TPFormat = {tpFormat,ARRAY_SIZE(tpFormat)};

  
}

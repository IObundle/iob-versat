#include "globals.hpp"

#include "utils.hpp"

Options globalOptions = {};
DebugState globalDebug = {};

Arena* globalPermanent;

Options DefaultOptions(Arena* out){
  Options res = {};
  res.databusDataSize = 32;
  res.databusAddrSize = 32;

  res.useFixedBuffers = true;
  res.shadowRegister = true; 
  res.disableDelayPropagation = true;
  
#ifdef USE_FST_FORMAT
  res.generateFSTFormat = 1;
#endif
  
  res.debugPath = PushString(out,"%s/debug",GetCurrentDirectory()); // By default
  PushNullByte(out);
  
  return res;
}

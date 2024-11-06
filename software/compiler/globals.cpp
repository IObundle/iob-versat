#include "globals.hpp"

Options globalOptions = {};
DebugState globalDebug = {};
static Arena globalPermanentInst = {};
Arena* globalPermanent = &globalPermanentInst;

Options DefaultOptions(Arena* perm){
  Options res = {};
  res.databusDataSize = 32;
  res.databusAddrSize = 32;

  res.useFixedBuffers = true;
  res.shadowRegister = true; 
  res.disableDelayPropagation = true;

#ifdef USE_FST_FORMAT
  res.generateFSTFormat = 1;
#endif
  
  res.debugPath = PushString(perm,"%s/debug",GetCurrentDirectory()); // By default
  PushNullByte(perm);
  
  return res;
}

#include "globals.hpp"

Options globalOptions = {};
DebugState globalDebug = {};
static Arena globalPermanentInst = {};
Arena* globalPermanent = &globalPermanentInst;

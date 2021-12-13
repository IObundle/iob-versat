#include <stddef.h>
#include <stdio.h>
#include <math.h>

#include "unitVerilogWrappers.h"

#define INSTANTIATE_ARRAY
#include "unitData.h"
#undef INSTANTIATE_ARRAY

#define ARRAY_SIZE(array) sizeof(array) / sizeof(array[0])

EXPORT FU_Type RegisterAdd(Versat* versat){
   FUDeclaration decl = {};

   decl.name = "xadd";
   decl.nInputs = 2;
   decl.nOutputs = 1;
   decl.latency = 1;

   FU_Type type = RegisterFU(versat,decl);

   return type;
}

EXPORT FU_Type RegisterReg(Versat* versat){
   FUDeclaration decl = {};

   decl.name = "xreg";
   decl.nInputs = 1;
   decl.nOutputs = 1;
   decl.memoryMapBytes = 4;
   decl.type = (VERSAT_TYPE_SOURCE | VERSAT_TYPE_SINK | VERSAT_TYPE_IMPLEMENTS_DELAY);

   FU_Type type = RegisterFU(versat,decl);

   return type;
}

EXPORT FU_Type RegisterMem(Versat* versat,int addr_w){
   FUDeclaration decl = {};

   decl.name = "xmem #(.ADDR_W(10))";
   decl.nInputs = 2;
   decl.nOutputs = 2;
   decl.memoryMapBytes = (1 << 10) * 4;
   decl.type = (VERSAT_TYPE_SOURCE | VERSAT_TYPE_SINK | VERSAT_TYPE_IMPLEMENTS_DELAY | VERSAT_TYPE_SOURCE_DELAY);
   decl.latency = 3;

   FU_Type type = RegisterFU(versat,decl);

   return type;
}

EXPORT FU_Type RegisterVRead(Versat* versat){
   FUDeclaration decl = {};

   decl.name = "vread";
   decl.nInputs = 0;
   decl.nOutputs = 1;
   decl.doesIO = true;
   decl.type = (VERSAT_TYPE_SOURCE | VERSAT_TYPE_IMPLEMENTS_DELAY | VERSAT_TYPE_SOURCE_DELAY);
   decl.latency = 1;

   FU_Type type = RegisterFU(versat,decl);

   return type;
}

EXPORT FU_Type RegisterVWrite(Versat* versat){
   FUDeclaration decl = {};

   decl.name = "vwrite";
   decl.nInputs = 1;
   decl.doesIO = true;
   decl.type = (VERSAT_TYPE_SINK | VERSAT_TYPE_IMPLEMENTS_DELAY);

   FU_Type type = RegisterFU(versat,decl);

   return type;
}

EXPORT FU_Type RegisterDebug(Versat* versat){
   FUDeclaration decl = {};

   decl.name = "";
   decl.nInputs = 1;
   decl.extraDataSize = sizeof(int);
   decl.type = VERSAT_TYPE_SINK;

   FU_Type type = RegisterFU(versat,decl);

   return type;
}

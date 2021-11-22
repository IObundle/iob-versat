#include <stddef.h>
#include <stdio.h>
#include <math.h>

#include "unitVerilogWrappers.h"

#define INSTANTIATE_ARRAY
#include "unitData.h"
#undef INSTANTIATE_ARRAY

#define ARRAY_SIZE(array) sizeof(array) / sizeof(array[0])

EXPORT FU_Type RegisterAdd(Versat* versat){
   FU_Type type = RegisterFU(versat,"xadd",
                                    2, // n inputs
                                    1, // n outputs
                                    0, // Config
                                    NULL,
                                    0, // State
                                    NULL,
                                    0, // MemoryMapped
                                    false, // IO
                                    0, // Extra memory
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL);

   return type;
}

EXPORT FU_Type RegisterReg(Versat* versat){
   FU_Type type = RegisterFU(versat,"xreg",
                                    1, // n inputs
                                    1, // n outputs
                                    ARRAY_SIZE(regConfigWires), // Config
                                    regConfigWires,
                                    ARRAY_SIZE(regStateWires), // State
                                    regStateWires,
                                    4, // MemoryMapped
                                    false, // IO
                                    0, // Extra memory
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL);

   return type;
}

EXPORT FU_Type RegisterMem(Versat* versat,int addr_w){
   FU_Type type = RegisterFU(versat,"xmem",
                                    2, // n inputs
                                    2, // n outputs
                                    ARRAY_SIZE(memConfigWires), // Config
                                    memConfigWires, 
                                    0, // State
                                    NULL,
                                    (1 << addr_w) * 4, // MemoryMapped
                                    false, // IO
                                    0, // Extra memory
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL);

   return type;
}

EXPORT FU_Type RegisterVRead(Versat* versat){
   FU_Type type = RegisterFU(versat,"vread",
                                    0, // n inputs
                                    1, // n outputs
                                    ARRAY_SIZE(vreadConfigWires), // Config
                                    vreadConfigWires, 
                                    0, // State
                                    NULL,
                                    0, // MemoryMapped
                                    true, // IO
                                    0, // Extra memory
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL);

   return type;
}

EXPORT FU_Type RegisterVWrite(Versat* versat){
   FU_Type type = RegisterFU(versat,"vwrite",
                                    1, // n inputs
                                    0, // n outputs
                                    ARRAY_SIZE(vwriteConfigWires), // Config
                                    vwriteConfigWires, 
                                    0, // State
                                    NULL,
                                    0, // MemoryMapped
                                    true, // IO
                                    0, // Extra memory
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL);

   return type;
}

EXPORT FU_Type RegisterDebug(Versat* versat){
   FU_Type type = RegisterFU(versat,"",
                                    1, // n inputs
                                    0, // n outputs
                                    0, // Config
                                    NULL,
                                    0, // State
                                    NULL,
                                    0, // MemoryMapped
                                    false, // IO
                                    0, // Extra memory
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL);

   return type;
}
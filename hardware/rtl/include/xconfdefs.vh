// Address width
`define CONF_ADDR_W 8

// Total conf_reg bits
`define CONF_BITS (2*`nMEM*`MEMP_CONF_BITS + `nALU*`ALU_CONF_BITS + `nALULITE*`ALULITE_CONF_BITS + `nMUL*`MUL_CONF_BITS + `nMULADD*`MULADD_CONF_BITS + `nBS*`BS_CONF_BITS)

//
// CONFIGURATION CACHE MEMORY
//
// uncomment to use this module
//`define CONF_MEM_USE

// Address width 
`define CONF_MEM_ADDR_W 6 

//
// CONFIGURATION REGISTER 
//
// Bit map
`define CONF_MEM0A_B (`CONF_BITS-1)
`define CONF_ALU0_B (`CONF_MEM0A_B - 2*`nMEM*`MEMP_CONF_BITS)
`define CONF_ALULITE0_B (`CONF_ALU0_B - `nALU*`ALU_CONF_BITS)
`define CONF_MUL0_B (`CONF_ALULITE0_B - `nALULITE*`ALULITE_CONF_BITS)
`define CONF_MULADD0_B (`CONF_MUL0_B - `nMUL*`MUL_CONF_BITS)
`define CONF_BS0_B (`CONF_MULADD0_B - `nMULADD*`MULADD_CONF_BITS)

// Address width 
`define CONF_REG_ADDR_W 7

//
// Memory map
//
`define CONF_CLEAR `ADDR_W'd0
// FU configurations 
`define CONF_MEM0A `ADDR_W'd1
`define CONF_ALU0 (`CONF_MEM0A + 2*`nMEM*`MEMP_CONF_OFFSET)
`define CONF_ALULITE0 (`CONF_ALU0 + `nALU*`ALU_CONF_OFFSET)
`define CONF_MUL0 (`CONF_ALULITE0 + `nALULITE*`ALULITE_CONF_OFFSET)
`define CONF_MULADD0 (`CONF_MUL0  + `nMUL*`MUL_CONF_OFFSET)
`define CONF_BS0 (`CONF_MULADD0  + `nMULADD*`MULADD_CONF_OFFSET)
// Configuration cache memory
`define CONF_MEM `CONF_ADDR_W'h40

// Total conf_reg addresses
`define CONF_REG_OFFSET (`CONF_BS0  + `nBS*`BS_CONF_OFFSET)

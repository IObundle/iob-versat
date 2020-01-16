// EDIT: Number of functional units

`define nMEM      6
`define nALU      0
`define nALULITE  2
`define nMUL      0
`define nMULADD   2
`define nBS       0

// EDIT: please assign to log2(nMEM)
`define nMEM_W 3

// EDIT: Define width of memories
`define MEM_ADDR_W_DEF '{13, 13, 13, 13, 13, 13}

// Define name of memory init files
`define MEM_NAME_NCHARS 9
`define MEM_INIT_FILE_DEF '{"xmem0.hex", "xmem1.hex", "xmem2.hex", "xmem3.hex", "xmem4.hex", "xmem5.hex"}

// TODO: The defines below should be deprecated since we are using MEM_ADDR_W_DEF
// We should be able to calculate MEM_DATA_SZ using MEM_ADDR_W_DEF
`define MEM7_ADDR_W 13
`define MEM6_ADDR_W 13
`define MEM5_ADDR_W 13
`define MEM4_ADDR_W 13
`define MEM3_ADDR_W 13
`define MEM2_ADDR_W 13
`define MEM1_ADDR_W 13
`define MEM0_ADDR_W 13

// Define total size of memories
`define MEM_DATA_SZ (2**`MEM7_ADDR_W + 2**`MEM6_ADDR_W + 2**`MEM5_ADDR_W + 2**`MEM4_ADDR_W + 2**`MEM3_ADDR_W + 2**`MEM2_ADDR_W + 2**`MEM1_ADDR_W + 2**`MEM0_ADDR_W)

// Number of data bus entries
// (note 2 special entries for constants 0 and 1)
`define N         (2 + 2*`nMEM + `nALU + `nALULITE + `nMUL + `nMULADD + `nBS) //21

// EDIT: Number of bits required for N (please insert by hand as $clog2(`N))+1
`define N_W       6

// Data bus size
`define DATA_BITS (`N *`DATA_W) //internal data bus

// EDIT: Data memories address width
`define DADDR_W 13 // 2**13 * 8 = 8kB each mem, 64kB for 8 data mems

// Selection codes for data bus entries
`define sNONE       `N_W'd0       //only used by memory ports to select read mode
`define s0          `N_W'd1
`define s1          `N_W'd2
`define sMEM0A 	    `N_W'd3
`define sALU0       (`sMEM0A + 2*`nMEM)
`define sALULITE0   (`sALU0 + `nALU)
`define sMUL0       (`sALULITE0 + `nALULITE)
`define sMULADD0    (`sMUL0 + `nMUL)
`define sBS0        (`sMULADD0 + `nMULADD)
`define sADDR       (`sBS0 + `nBS) //only used by memory ports to select its own address


// Address width
`define ENG_ADDR_W (`nMEM_W+`DADDR_W)

//
// Data bus bit map
//
`define DATA_S0_B       (`DATA_BITS-1)
`define DATA_S1_B       (`DATA_S0_B - `DATA_W)
`define DATA_MEM0A_B    (`DATA_S1_B - `DATA_W )
`define DATA_ALU0_B     (`DATA_MEM0A_B - 2*`nMEM*`DATA_W)
`define DATA_ALULITE0_B (`DATA_ALU0_B - `nALU*`DATA_W)
`define DATA_MUL0_B     (`DATA_ALULITE0_B - `nALULITE*`DATA_W)
`define DATA_MULADD0_B  (`DATA_MUL0_B - `nMUL*`DATA_W )
`define DATA_BS0_B      (`DATA_MULADD0_B - `nMULADD*`DATA_W)


//
// Memory map
//

// memory array starts at address 0

// control and status
`define ENG_RUN_REG  (2**`ENG_ADDR_W-1) //last address
`define ENG_RDY_REG (2**`ENG_ADDR_W-2) //address before last


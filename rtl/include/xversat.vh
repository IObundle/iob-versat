//
// VERSAT DEFINES
//

// VERSAT DATA WIDTH
`define DATA_W 32 // bits

// NUMBER OF STAGES
`define nSTAGE 5

// Number of functional units in each stage

`define nMEM      6
`define nALU      0
`define nALULITE  2
`define nMUL      0
`define nMULADD   2
`define nBS       0

// Data memories address width
`define MEM_ADDR_W 10

//
// DO NOT EDIT BEYOND THIS POINT
//
// Number of data bus entries
// (note 2 special entries for constants 0 and 1)

`define nSTAGE_W $clog2(`nSTAGE)

// Number of bits to select memory
`define nMEM_W $clog2(`nMEM)

//`define ADDR_W (`nSTAGE_W+1+`nMEM_W+`MEM_ADDR_W)
//need to enter value by hand
`define ADDR_W 16

// Number of data bus entries
`define N         (2*`nMEM + `nALU + `nALULITE + `nMUL + `nMULADD + `nBS)
// Number of bits required for N
`define N_W       ($clog2(`N)+1)

// Data bus size
`define DATABUS_W (`N *`DATA_W) //internal data bus

//
// Data bus bit map
//
`define DATA_MEM0A_B    (`DATABUS_W-1)
`define DATA_ALU0_B     (`DATA_MEM0A_B - 2*`nMEM*`DATA_W)
`define DATA_ALULITE0_B (`DATA_ALU0_B - `nALU*`DATA_W)
`define DATA_MUL0_B     (`DATA_ALULITE0_B - `nALULITE*`DATA_W)
`define DATA_MULADD0_B  (`DATA_MUL0_B - `nMUL*`DATA_W )
`define DATA_BS0_B      (`DATA_MULADD0_B - `nMULADD*`DATA_W)

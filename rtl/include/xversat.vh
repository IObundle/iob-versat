//
// VERSAT DEFINES
//

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
`define PERIOD_W 10  //LOG2 of max period and duty cicle

//
// DO NOT EDIT BEYOND THIS POINT
//

//Number of bits required for nSTAGE
`define nSTAGE_W ($clog2(`nSTAGE))

// Number of bits to select memory
`define nMEM_W ($clog2(`nMEM))

//CONTROL ADDRESS WIDTH
//2 extra bits are to select xdata_eng/xconf and run/done
`define CTR_ADDR_W (`nSTAGE_W+2+`nMEM_W+`MEM_ADDR_W)

// Number of data bus entries
`define N         (2*`nMEM + `nALU + `nALULITE + `nMUL + `nMULADD + `nBS)
// Number of bits required for N
`define N_W       ($clog2(`N)+1)

// Data bus size
`define DATABUS_W (`N * DATA_W) //internal data bus

//
// Data bus bit map
//
`define DATA_MEM0A_B    (`DATABUS_W-1)
`define DATA_ALU0_B     (`DATA_MEM0A_B - 2*`nMEM*DATA_W)
`define DATA_ALULITE0_B (`DATA_ALU0_B - `nALU*DATA_W)
`define DATA_MUL0_B     (`DATA_ALULITE0_B - `nALULITE*DATA_W)
`define DATA_MULADD0_B  (`DATA_MUL0_B - `nMUL*DATA_W )
`define DATA_BS0_B      (`DATA_MULADD0_B - `nMULADD*DATA_W)

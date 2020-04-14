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
`define DADDR_W 13 // 2**13 * 8 = 8kB each mem, 64kB for 8 data mems

//
// DO NOT EDIT BEYOND THIS POINT
//
// Number of data bus entries
// (note 2 special entries for constants 0 and 1)

`define nSTAGE_W $clog2(nSTAGE)
`define ADDR_W (`nSTAGE_W+`nMEM_W+`DADDR_W)

// Number of bits to select memory
`define nMEM_W $clog2(nMEM)

// Number of data bus entries
`define N         (`nMEM + `nALU + `nALULITE + `nMUL + `nMULADD + `nBS)
// Number of bits required for N
`define N_W       $clog2(`N)

// Data bus size
`define DATABUS_W (`N *`DATA_W) //internal data bus

// Define total memory used 
`define MEM_DATA_SZ (nMEM * 2**`DADDR_W)

//
// MEMORY MAP
//

`define CTR_CTR_ADDR (1<<`ADDR_W)
`define CTR_CONF_ADDR (2'b11<<(`ADDR_W-1))

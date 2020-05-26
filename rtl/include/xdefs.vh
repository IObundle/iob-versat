//Number of bits required for nSTAGE
`define nSTAGE_W ($clog2(`nSTAGE))

// Number of bits to select memory
`define nMEM_W ($clog2(`nMEM))

//CONTROL ADDRESS WIDTH
//2 extra bits are to select xdata_eng/xconf and run/done
`define CTR_ADDR_W (`nSTAGE_W+2+`nMEM_W+`MEM_ADDR_W)

// Number of data bus entries
`define N         (2*`nMEM + `nVI + `nALU + `nALULITE + `nMUL + `nMULADD + `nMULADDLITE + `nBS)
// Number of bits required for N
`define N_W       ($clog2(`N)+1)

// Data bus size
`define DATABUS_W (`N * `DATAPATH_W) //internal data bus

//
// Data bus bit map
//
`define DATA_MEM0A_B    	(`DATABUS_W-1)
`define DATA_VI0_B      	(`DATA_MEM0A_B - 2*`nMEM*`DATAPATH_W)
`define DATA_ALU0_B     	(`DATA_VI0_B - `nVI*`DATAPATH_W)
`define DATA_ALULITE0_B 	(`DATA_ALU0_B - `nALU*`DATAPATH_W)
`define DATA_MUL0_B     	(`DATA_ALULITE0_B - `nALULITE*`DATAPATH_W)
`define DATA_MULADD0_B  	(`DATA_MUL0_B - `nMUL*`DATAPATH_W)
`define DATA_MULADDLITE0_B 	(`DATA_MULADD0_B - `nMULADD*`DATAPATH_W)
`define DATA_BS0_B              (`DATA_MULADDLITE0_B - `nMULADDLITE*`DATAPATH_W)

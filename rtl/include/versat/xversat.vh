// NUMBER OF STAGES
`define nSTAGE 5

// Datapath width
`define DATAPATH_W 16

// Number of functional units in each stage
`define nMEM      4
`define nVI       0
`define nVO       0
`define nALU      0
`define nALULITE  2
`define nMUL      0
`define nMULADD   2
`define nBS       0

// Configurable address widths
`define MEM_ADDR_W 10
`define PERIOD_W 10 //LOG2 of max period and duty cicle
`define CONF_MEM_ADDR_W 3 //log2(size of conf cache)

//MULADD combinational architecture
//`define MULADD_COMB

//use xconf_mem
//`define CONF_MEM_USE

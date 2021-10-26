// MULADD config bits:
// input selection bits = 2 * N_W
// fns = 1 bit
// iterations = MEM_ADDR_W
// period, delay = PERIOD_W
// shift = SHIFT_W
`define MULADD_FNS_W 1
`define SHIFT_W ($clog2(`DATAPATH_W)+1)
`define MULADD_CONF_BITS (2*`N_W + `MULADD_FNS_W + `MEM_ADDR_W + 2*`PERIOD_W + `SHIFT_W)

// MULADD functions
`define MULADD_MACC           `MULADD_FNS_W'd0
`define MULADD_MSUB           `MULADD_FNS_W'd1

//MULADD configuration offsets
`define MULADD_CONF_SELA      0
`define MULADD_CONF_SELB      1
`define MULADD_CONF_FNS       2
`define MULADD_CONF_ITER      3
`define MULADD_CONF_PER       4
`define MULADD_CONF_DELAY     5
`define MULADD_CONF_SHIFT     6
`define MULADD_CONF_OFFSET    7

//MULADD latency
`ifdef MULADD_COMB
 `define MULADD_LAT 1
`else
 `define MULADD_LAT 4
`endif

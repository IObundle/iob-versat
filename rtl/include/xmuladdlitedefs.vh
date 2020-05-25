// MULADDLITE config bits:
// input selection bits = 3 * N_W
// iterations = MEM_ADDR_W
// period, delay = PERIOD_W
// shift = SHIFT_W
// accumulate = 1 bit
`define SHIFT_W ($clog2(`DATAPATH_W)+1)
`define MULADDLITE_CONF_BITS (3*`N_W + `MEM_ADDR_W + 2*`PERIOD_W + `SHIFT_W + 3)

//MULADDLITE configuration offsets
`define MULADDLITE_CONF_SELA    0
`define MULADDLITE_CONF_SELB    1
`define MULADDLITE_CONF_SELC    2
`define MULADDLITE_CONF_ITER    3
`define MULADDLITE_CONF_PER     4
`define MULADDLITE_CONF_DELAY   5
`define MULADDLITE_CONF_SHIFT   6
`define MULADDLITE_CONF_ACCIN   7
`define MULADDLITE_CONF_ACCOUT  8
`define MULADDLITE_CONF_BATCH   9
`define MULADDLITE_CONF_OFFSET  10

//MULADDLITE latency
`define MULADDLITE_LAT 5

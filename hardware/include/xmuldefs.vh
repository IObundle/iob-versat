//
// MUL DEFINES
//

// MUL config bits:
// input selection bits = 2 * N_W
// fns = 2 bits
`define MUL_FNS_W 2
`define MUL_CONF_BITS (2*`N_W + 2)

// MUL functions
`define MUL_HI 2'd1
`define MUL_DIV2_LO 2'd2
`define MUL_DIV2_HI 2'd3

// MUL configuration offsets
`define MUL_CONF_SELA 	  0
`define MUL_CONF_SELB 	  1
`define MUL_CONF_FNS 	  2
`define MUL_CONF_OFFSET   3

// MUL latency
`define MUL_LAT 3

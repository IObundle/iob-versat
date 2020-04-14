//
// BS DEFINES
//

// BS config bits:
// input selection bits = 2 * N_W
// fns = 2 bits
`define BS_FNS_W 2
`define BS_CONF_BITS (2*`N_W + `BS_FNS_W)

// BS functions
`define BS_SHR_A   `BS_FNS_W'd0
`define BS_SHR_L   `BS_FNS_W'd1
`define BS_SHL     `BS_FNS_W'd2

//BS configuration offsets
`define BS_CONF_SELD `ADDR_W'd0
`define BS_CONF_SELS `ADDR_W'd1
`define BS_CONF_FNS `ADDR_W'd2
`define BS_CONF_OFFSET `ADDR_W'd4

//BS latency
`define BS_LAT 2

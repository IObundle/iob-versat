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
`define BS_CONF_SELD   {`VERSAT_ADDR_W{1'd0}}
`define BS_CONF_SELS   (`BS_CONF_SELD + 1'd1)
`define BS_CONF_FNS    (`BS_CONF_SELS + 1'd1)
`define BS_CONF_OFFSET (`BS_CONF_FNS  + 1'd1)

//BS latency
`define BS_LAT 2

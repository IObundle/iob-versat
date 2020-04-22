// MULADD config bits:
// input selection bits = 2 * N_W
// fns = 4 bits
`define MULADD_FNS_W 4
`define MULADD_CONF_BITS (3*`N_W + `MULADD_FNS_W + `PERIOD_W)

// MULADD functions
`define MULADD_MUL            `MULADD_FNS_W'd0
`define MULADD_MUL_DIV2       `MULADD_FNS_W'd1
`define MULADD_MACC_16Q16     `MULADD_FNS_W'd2
`define MULADD_MACC           `MULADD_FNS_W'd3
`define MULADD_MACC_DIV2      `MULADD_FNS_W'd4
`define MULADD_MSUB           `MULADD_FNS_W'd5
`define MULADD_MSUB_DIV2      `MULADD_FNS_W'd6
`define MULADD_MUL_LOW        `MULADD_FNS_W'd7
`define MULADD_MUL_LOW_MACC   `MULADD_FNS_W'd8


//MULADD configuration offsets
`define MULADD_CONF_SELA      0
`define MULADD_CONF_SELB      1
`define MULADD_CONF_SELO      2
`define MULADD_CONF_FNS       3
`define MULADD_CONF_DELAY     4
`define MULADD_CONF_OFFSET    5

//MULADD combinational architecture
//`define MULADD_COMB

//MULADD latency
`ifdef MULADD_COMB
 `define MULADD_LAT 1
`else
 `define MULADD_LAT 3
`endif

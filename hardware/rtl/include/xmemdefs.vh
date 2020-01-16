// MEM port config bits
// input selection  bits = N_W
// start width = DADDR_W
// incr width = DADDR_W
// iterations = DADDR_W
// shift width = DADDR_W
// period = PERIOD_W
// duty = PERIOD_W
// delay width = PERIOD_W
// reverse = 1
// ext = 1
`define PERIOD_W 10  //LOG2 of max period and duty cicle

// Number of config bits
`define MEMP_CONF_BITS (`N_W + 4*`DADDR_W + 3*`PERIOD_W + 1 + 1)

//MEM configuration offsets
`define MEMP_CONF_ITER `ADDR_W'd0
`define MEMP_CONF_PER `ADDR_W'd1
`define MEMP_CONF_DUTY `ADDR_W'd2
`define MEMP_CONF_SEL `ADDR_W'd3
`define MEMP_CONF_START `ADDR_W'd4
`define MEMP_CONF_SHIFT `ADDR_W'd5
`define MEMP_CONF_INCR `ADDR_W'd6
`define MEMP_CONF_DELAY `ADDR_W'd7
`define MEMP_CONF_RVRS `ADDR_W'd8
`define MEMP_CONF_EXT `ADDR_W'd9
`define MEMP_CONF_OFFSET `ADDR_W'd10

//MEM latency
`define MEMP_LAT 1


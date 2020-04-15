//main defines

//MEM_ADDR_W must be defined in xversat.vh 
`define PERIOD_W 10  //LOG2 of max period and duty cicle

// MEM port config bits
// input selection  bits = N_W
// start width = MEM_ADDR_W
// incr width = MEM_ADDR_W
// iterations = MEM_ADDR_W
// shift width = MEM_ADDR_W
// period = PERIOD_W
// duty = PERIOD_W
// delay width = PERIOD_W
// reverse = 1
// ext = 1
// mode = 00 = {en,we}

// Number of config bits
`define MEMP_CONF_BITS (`N_W + 4*`MEM_ADDR_W + 3*`PERIOD_W + 1 + 1)

//MEM configuration offsets
`define MEMP_CONF_ITER 4'd0
`define MEMP_CONF_PER 4'd1
`define MEMP_CONF_DUTY 4'd2
`define MEMP_CONF_SEL 4'd3
`define MEMP_CONF_START 4'd4
`define MEMP_CONF_SHIFT 4'd5
`define MEMP_CONF_INCR 4'd6
`define MEMP_CONF_DELAY 4'd7
`define MEMP_CONF_RVRS 4'd8
`define MEMP_CONF_EXT 4'd9
`define MEMP_CONF_MODE 4'd10
`define MEMP_CONF_OFFSET 4'd10

//MEM latency
`define MEMP_LAT 1


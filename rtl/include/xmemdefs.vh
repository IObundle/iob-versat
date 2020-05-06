// MEM port config bits
// input selection  bits = N_W
// start width = MEM_ADDR_W
// incr width = 2*MEM_ADDR_W
// iterations = 2*MEM_ADDR_W
// shift width = 2*MEM_ADDR_W
// period = 2*PERIOD_W
// duty = PERIOD_W
// delay width = PERIOD_W
// reverse = 1
// ext = 1
// in_wr = 1

// Number of config bits
`define MEMP_CONF_BITS (`N_W + 7*`MEM_ADDR_W + 4*`PERIOD_W + 1 + 1 + 1)

//MEM configuration offsets
`define MEMP_CONF_ITER 		0
`define MEMP_CONF_PER 		1
`define MEMP_CONF_DUTY 		2
`define MEMP_CONF_SEL 		3
`define MEMP_CONF_START 	4
`define MEMP_CONF_SHIFT 	5 
`define MEMP_CONF_INCR 		6
`define MEMP_CONF_DELAY 	7
`define MEMP_CONF_RVRS 		8
`define MEMP_CONF_EXT 		9
`define MEMP_CONF_IN_WR 	10
`define MEMP_CONF_ITER2 	11
`define MEMP_CONF_PER2 		12
`define MEMP_CONF_SHIFT2 	13 
`define MEMP_CONF_INCR2 	14
`define MEMP_CONF_OFFSET 	15

//MEM latency
`define MEMP_LAT 3

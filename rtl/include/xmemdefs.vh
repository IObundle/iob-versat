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
// in_wr = 1
// addr_out_en = 1

// Number of config bits
`define MEMP_CONF_BITS (`N_W + 4*`MEM_ADDR_W + 3*`PERIOD_W + 1 + 1 + 1 + 1)

//MEM configuration offsets
`define MEMP_CONF_ITER 		{`VERSAT_ADDR_W{1'd0}}
`define MEMP_CONF_PER 		(`MEMP_CONF_ITER  	+ 1'd1)
`define MEMP_CONF_DUTY 		(`MEMP_CONF_PER   	+ 1'd1)
`define MEMP_CONF_SEL 		(`MEMP_CONF_DUTY  	+ 1'd1)
`define MEMP_CONF_START 	(`MEMP_CONF_SEL   	+ 1'd1)
`define MEMP_CONF_SHIFT 	(`MEMP_CONF_START 	+ 1'd1) 
`define MEMP_CONF_INCR 		(`MEMP_CONF_SHIFT 	+ 1'd1)
`define MEMP_CONF_DELAY 	(`MEMP_CONF_INCR  	+ 1'd1)
`define MEMP_CONF_RVRS 		(`MEMP_CONF_DELAY 	+ 1'd1)
`define MEMP_CONF_EXT 		(`MEMP_CONF_RVRS        + 1'd1)
`define MEMP_CONF_IN_WR 	(`MEMP_CONF_EXT         + 1'd1)
`define MEMP_CONF_ADDR_OUT_EN 	(`MEMP_CONF_IN_WR       + 1'd1)
`define MEMP_CONF_OFFSET 	(`MEMP_CONF_ADDR_OUT_EN + 1'd1)

//MEM latency
`define MEMP_LAT 1

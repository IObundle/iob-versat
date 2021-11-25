// External memory address width 2**32 = 4GB
`define IO_ADDR_W 32

// Transfer size width 2**11 = 2048 words (same as parameter ADDR_W)
`define IO_SIZE_W 11

// Configuration

// `MEMP_CONF_BITS = `N_W + 7*`MEM_ADDR_W + 4*`PERIOD_W + 1 + 1 + 1

// Configuration versat-i
`define VI_MEMP_CONF_BITS (`MEMP_CONF_BITS - 1) // No select and write enable

`define VI_CONFIG_BITS (`IO_ADDR_W+`IO_SIZE_W+4*`MEM_ADDR_W+2*`PERIOD_W+`VI_MEMP_CONF_BITS)

// IO configuration offsets
`define VI_CONF_EXT_ADDR    0
`define VI_CONF_INT_ADDR    1
`define VI_CONF_SIZE        2
`define VI_CONF_ITER_A      3
`define VI_CONF_PER_A       4
`define VI_CONF_DUTY_A      5
`define VI_CONF_SHIFT_A     6
`define VI_CONF_INCR_A      7
`define VI_CONF_ITER_B      8
`define VI_CONF_PER_B       9
`define VI_CONF_DUTY_B      10
`define VI_CONF_START_B     11
`define VI_CONF_SHIFT_B     12
`define VI_CONF_INCR_B      13
`define VI_CONF_DELAY_B     14
`define VI_CONF_RVRS_B      15
`define VI_CONF_EXT_B       16
`define VI_CONF_ITER2_B     17
`define VI_CONF_PER2_B      18
`define VI_CONF_SHIFT2_B    19
`define VI_CONF_INCR2_B     20
`define VI_CONF_OFFSET      21

// Configuration versat-o
`define VO_MEMP_CONF_BITS (`MEMP_CONF_BITS-1) // No write enable

`define VO_CONFIG_BITS (`IO_ADDR_W+`IO_SIZE_W+4*`MEM_ADDR_W+2*`PERIOD_W+`VO_MEMP_CONF_BITS)

// IO configuration offsets
`define VO_CONF_EXT_ADDR    0
`define VO_CONF_INT_ADDR    1
`define VO_CONF_SIZE        2
`define VO_CONF_ITER_A      3
`define VO_CONF_PER_A       4
`define VO_CONF_DUTY_A      5
`define VO_CONF_SHIFT_A     6
`define VO_CONF_INCR_A      7
`define VO_CONF_ITER_B      8
`define VO_CONF_PER_B       9
`define VO_CONF_DUTY_B      10
`define VO_CONF_SEL_B       11
`define VO_CONF_START_B     12
`define VO_CONF_SHIFT_B     13
`define VO_CONF_INCR_B      14
`define VO_CONF_DELAY_B     15
`define VO_CONF_RVRS_B      16
`define VO_CONF_EXT_B       17
`define VO_CONF_ITER2_B     18
`define VO_CONF_PER2_B      19
`define VO_CONF_SHIFT2_B    20
`define VO_CONF_INCR2_B     21
`define VO_CONF_OFFSET      22

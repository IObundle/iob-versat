// ALU lite config bits:
// input selection  bits = 2 * N_W
// fns = 4
`define ALULITE_FNS_W     4
`define ALULITE_CONF_BITS (`ALULITE_FNS_W)

// ALULITE functions
// concat a 1 to the left to get the feedback versions of these functions
`define ALULITE_ADD              3'd0
`define ALULITE_SUB              3'd1
`define ALULITE_CMP_SIG          3'd2    
`define ALULITE_MUX   	         3'd3
`define ALULITE_MAX              3'd4 		
`define ALULITE_MIN              3'd5
`define ALULITE_OR               3'd6
`define ALULITE_AND              3'd7

//ALULITE internal configuration address offsets
`define ALULITE_CONF_SELA        0
`define ALULITE_CONF_SELB        1
`define ALULITE_CONF_FNS         2
`define ALULITE_CONF_OFFSET      3

//ALULITE latency
`define ALULITE_LAT            	 2

// ALU config bits:
// input selection  bits = 2 * N_W
`define ALU_FNS_W       4
`define ALU_CONF_BITS (2*`N_W + `ALU_FNS_W)

// ALU functions
`define ALU_ADD           `ALU_FNS_W'd0
`define ALU_SUB           `ALU_FNS_W'd1
`define ALU_CMP_SIG       `ALU_FNS_W'd2    
`define ALU_MUX   	  `ALU_FNS_W'd3
`define ALU_MAX       	  `ALU_FNS_W'd4		
`define ALU_MIN       	  `ALU_FNS_W'd5
`define ALU_OR            `ALU_FNS_W'd6
`define ALU_AND           `ALU_FNS_W'd7
`define ALU_CMP_UNS       `ALU_FNS_W'd8
`define ALU_XOR           `ALU_FNS_W'd9
`define ALU_SEXT8         `ALU_FNS_W'd10
`define ALU_SEXT16        `ALU_FNS_W'd11
`define ALU_SHIFTR_ARTH   `ALU_FNS_W'd12
`define ALU_SHIFTR_LOG    `ALU_FNS_W'd13
`define ALU_CLZ           `ALU_FNS_W'd14
`define ALU_ABS           `ALU_FNS_W'd15

//ALU internal configuration address offsets
`define ALU_CONF_SELA 	  0
`define ALU_CONF_SELB     1
`define ALU_CONF_FNS      2
`define ALU_CONF_OFFSET   3

//ALU latency
`define ALU_LAT 2

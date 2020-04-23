`timescale 1ns / 1ps
`include "xversat.vh"
`include "xmuldefs.vh"

module xmul # ( 
	     parameter			DATA_W = 32
	) (
             input                      clk,
             input                      rst,

             //flow interface
             input [2*`DATABUS_W-1:0]   flow_in,
             output reg [DATA_W-1:0] 	flow_out,

             //config interface
             input [`MUL_CONF_BITS-1:0] configdata
             );

   // 2*DATA_W MUL RESULT AND REGISTERED RESULT
   wire signed [2*DATA_W-1:0]                res2;
   reg signed [2*DATA_W-1:0]                 res2_reg;
   reg signed [DATA_W-1:0]                   res;
   
   //data
   wire signed [DATA_W-1:0]                  op_a;
   wire signed [DATA_W-1:0]                  op_b;
   reg signed [DATA_W-1:0]                   op_a_reg;
   reg signed [DATA_W-1:0]                   op_b_reg;
   
   //config data
   wire [`N_W-1: 0]                          sela;
   wire [`N_W-1: 0]                          selb;
   wire [`MUL_FNS_W-1: 0]                    fns;
   
   //unpack config bits 
   assign sela = configdata[`MUL_CONF_BITS-1 -: `N_W];
   assign selb = configdata[`MUL_CONF_BITS-1-`N_W -: `N_W];
   assign fns = configdata[`MUL_CONF_BITS-1-2*`N_W -: `MUL_FNS_W];
   
   //input selection 
   xinmux # ( 
	.DATA_W(DATA_W)
   ) muxa (
	.sel(sela),
	.data_in(flow_in),
	.data_out(op_a)
	);
   
   xinmux # ( 
	.DATA_W(DATA_W)
   ) muxb (
        .sel(selb),
        .data_bus(flow_in),
        .data_out(op_b)
        );

   always @ (posedge clk, posedge rst) 
     if(rst) begin 
        op_a_reg <= {DATA_W{1'd0}};
        op_b_reg <= {DATA_W{1'd0}};
     end else begin
        op_a_reg <= op_a;
        op_b_reg <= op_b;
     end
   
   assign res2 = op_a_reg * op_b_reg;

   //apply function
   always @*
     case(fns)
       `MUL_HI: res = res2_reg[2*DATA_W-2 -: DATA_W];
       `MUL_DIV2_HI: res = res2_reg[2*DATA_W-1 -: DATA_W];
       default: res = res2_reg[DATA_W-1 -: DATA_W];
     endcase 

   always @ (posedge clk, posedge rst) 
     if (rst)
       flow_out <= {DATA_W{1'b0}};
     else 
       flow_out <= res;
   
endmodule

`timescale 1ns / 1ps
`include "xdefs.vh"
`include "xmuldefs.vh"

module xmul (
      input 			     clk,
      input 			     rst,
      
      //data 
      input [`N*`DATA_W-1:0] 	        data_bus,
      input [`N*`DATA_W-1:0] 	        data_bus_prev,
      output reg signed [`DATA_W-1:0] result,
      
      //config 
      input [`MUL_CONF_BITS-1:0]      configdata
      );
   
   reg 					     rst_reg;

   // 2*`DATA_W MUL RESULT AND REGISTERED RESULT
   wire signed [2*`DATA_W-1:0] 		     res2;
   reg signed [2*`DATA_W-1:0] 		     res2_reg;
   reg signed [`DATA_W-1:0] 		     res;
   
   //data
   wire signed [`DATA_W-1:0] 		     op_a;
   wire signed [`DATA_W-1:0] 		     op_b;
   reg signed [`DATA_W-1:0] 		     op_a_reg;
   reg signed [`DATA_W-1:0] 		     op_b_reg;
   
   //config data
   wire [`N_W-1: 0] 			     sela;
   wire [`N_W-1: 0] 			     selb;
   wire [`MUL_FNS_W-1: 0] 		     fns;
   
   wire 				     enablea;
   wire 				     enableb;
   wire 				     enabled;

   
   //unpack config bits 
   assign sela = configdata[`MUL_CONF_BITS-1 -: `N_W];
   assign selb = configdata[`MUL_CONF_BITS-1-`N_W -: `N_W];
   assign fns = configdata[`MUL_CONF_BITS-1-2*`N_W -: `MUL_FNS_W];
      
     //input selection 
     xinmux muxa (
		  .sel(sela),
		  .data_bus(data_bus),
		  .data_bus_prev(data_bus_prev),
		  .data_out(op_a),
      .enabled(enablea)
		  );
   
     xinmux muxb (
      .sel(selb),
      .data_bus(data_bus),
      .data_bus_prev(data_bus_prev),
      .data_out(op_b),
      .enabled(enableb)
  	  );

   assign enabled = enablea & enableb;
   
   always @ (posedge clk)
     rst_reg <= rst;
   
   always @ (posedge rst_reg, posedge clk)
     if (rst_reg) begin
	      res2_reg <= {2*`DATA_W{1'b0}};
        op_a_reg <= `DATA_W'd0;
        op_b_reg <= `DATA_W'd0;
     end else begin
        res2_reg <= res2;
        op_a_reg <= op_a;
        op_b_reg <= op_b;
     end
   
   assign res2 = op_a_reg * op_b_reg;

   //apply function
   always @*
     case(fns)
       `MUL_HI: res = res2_reg[2*`DATA_W-2 -: `DATA_W];
       `MUL_DIV2_HI: res = res2_reg[2*`DATA_W-1 -: `DATA_W];
       default: res = res2_reg[`DATA_W-1 -: `DATA_W];
      endcase 

   always @ (posedge rst_reg, posedge clk) 
     if (rst_reg)
       result <= `DATA_W'h00000000;
     else if (enabled) begin
	result <= res;
     end
   
endmodule

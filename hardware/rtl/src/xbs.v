`timescale 1ns / 1ps
`include "xdefs.vh"
`include "xdata_engdefs.vh"
`include "xbsdefs.vh"

module xbs(
      //control 
      input 			                    clk,
      input 			                    rst,
      
      //data 
      input [`N*`DATA_W-1:0] 	        data_bus,
      input [`N*`DATA_W-1:0] 	        data_bus_prev,
      output reg signed [`DATA_W-1:0] result,
      
      //config data
      input [`BS_CONF_BITS-1:0] 	    configdata
      );
   
   reg 					              rst_reg;

   // extract the 2 inputs
   wire signed [`DATA_W-1:0] 	data_in; //data to shift 
   wire [`DATA_W-1:0] 			  shift; //shift value
   
   reg [`DATA_W-1:0] 			    res;

   //wiring config data
   wire [`N_W-1: 0] 			    seldata;
   wire [`N_W-1: 0] 			    selshift;
   wire [`BS_FNS_W-1:0] 		  fns;

   //enables
   wire 				   enablea;
   wire 				   enables;
   wire 				   enabled;

   // register input mux outputs
   reg [`DATA_W-1:0] 			    data_in_reg;
   reg [`DATA_W-1:0] 			    shift_reg;

   
   //unpack config bits
   assign seldata = configdata[`BS_CONF_BITS-1 -: `N_W];
   assign selshift = configdata[`BS_CONF_BITS-1-`N_W -: `N_W];
   assign fns = configdata[`BS_CONF_BITS-1-2*`N_W -: `BS_FNS_W];
   

   //input selection 
   xinmux muxdata (
		   .sel(seldata),
		   .data_bus(data_bus),
		   .data_bus_prev(data_bus_prev),
		   .data_out(data_in),
		   .enabled(enablea)
		   );
   
   xinmux muxshift (
		    .sel(selshift),
		    .data_bus(data_bus),
		    .data_bus_prev(data_bus_prev),
		    .data_out(shift),
		    .enabled(enables)
		    );
   
   // compute output enable
   assign enabled = enablea & enables;

   always @ (posedge clk)
     rst_reg <= rst;

   always @ (posedge clk)
     if (rst_reg) begin
	      data_in_reg <= `DATA_W'h00000000;
	      shift_reg <= `DATA_W'h00000000;
     end else begin
        data_in_reg <= data_in;
        shift_reg <= shift;
     end // else: !if(rst)

   always @* begin
      case(fns)
        `BS_SHR_A: res = data_in_reg >>> shift_reg[4:0];
        `BS_SHR_L: res = data_in_reg >> shift_reg[4:0];
        `BS_SHL: res = data_in_reg << shift_reg[4:0];
	    default: res = data_in_reg;
      endcase
   end

   always @ (posedge rst_reg, posedge clk) 
     if (rst_reg)
       result <= `DATA_W'h00000000;
     else if (enabled)
       result <= res;

endmodule

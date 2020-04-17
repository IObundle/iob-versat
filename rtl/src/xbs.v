`timescale 1ns / 1ps
`include "xversat.vh"

`include "xbsdefs.vh"

module xbs(
           //control 
           input                     clk,
           input                     rst,

           //flow interface
           input [2*`DATABUS_W-1:0]  flow_in,
           output [`DATA_W-1:0]	     flow_out,

           //config data
           input [`BS_CONF_BITS-1:0] configdata
           );
   
   // extract the 2 inputs
   wire signed [`DATA_W-1:0]         data_in; //data to shift 
   wire [`DATA_W-1:0]                shift; //shift value
   
   reg [`DATA_W-1:0]                 res;

   //wiring config data
   wire [`N_W-1: 0]                  seldata;
   wire [`N_W-1: 0]                  selshift;
   wire [`BS_FNS_W-1:0]              fns;


   // register input mux outputs
   reg [`DATA_W-1:0]                 data_in_reg;
   reg [`DATA_W-1:0]                 shift_reg;

   
   //unpack config bits
   assign seldata = configdata[`BS_CONF_BITS-1 -: `N_W];
   assign selshift = configdata[`BS_CONF_BITS-1-`N_W -: `N_W];
   assign fns = configdata[`BS_CONF_BITS-1-2*`N_W -: `BS_FNS_W];
   
   //input selection 
   xinmux muxdata (
		   .sel(seldata),
		   .data_in(flow_in),
		   .data_out(data_in)
		   );
   
   xinmux muxshift (
		    .sel(selshift),
		    .data_in(flow_in),
		    .data_out(shift)
		    );
   
   always @ (posedge clk, posedge rst)
     if (rst) begin
	data_in_reg <= `DATA_W'h0;
	shift_reg <= `DATA_W'h0;
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

   always @ (posedge clk, posedge rst) 
     if (rst)
       flow_out <= `DATA_W'h0;
     else begin
	flow_out <= res;
     end

endmodule

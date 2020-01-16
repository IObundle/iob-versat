`timescale 1ns / 1ps
`include "xdefs.vh"
`include "xmuldefs.vh"
`include "xmuladddefs.vh"
`include "xconfdefs.vh"

module xconf_mem(
		 input 			      clk,
		 input 			      rst,

		 //configuration port
		 input 			      req,
		 input 			      rnw,
		 input [`CONF_MEM_ADDR_W-1:0] addr,
		 input [`CONF_BITS-1:0]       data_in,
		 output reg [`CONF_BITS-1:0]  data_out,
		 output reg 		      conf_ld
		 );

   // Load configuration to xconf_reg after 1-cycle memory read latency
   always @ (posedge rst, posedge clk) begin
      if(rst)
	conf_ld <= 1'b0;
      else
	conf_ld <= req & rnw;
   end

   //instantiate the config cache memory
`ifdef ALTERA
   reg [`CONF_BITS-1:0] mem [(2**`CONF_MEM_ADDR_W)-1:0];
   always @ (posedge clk) begin
      if (~rnw)
        mem[addr] <= data_in;
      data_out <= mem[addr];
   end
`endif

endmodule


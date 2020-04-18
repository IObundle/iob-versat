`timescale 1ns / 1ps
`include "xversat.vh"
`include "xconfdefs.vh"

module xconf_mem(
		 input 			      clk,
		 input 			      rst,

                 //data interface
		 input [`CONF_BITS-1:0]       conf_in,
		 output reg [`CONF_BITS-1:0]  conf_out,

		 //control interface
		 input 			      ctr_valid,
		 input 			      ctr_we,
		 input [`CONF_MEM_ADDR_W-1:0] ctr_addr,
		 output reg 		      conf_ld
		 );

   // Load configuration to xconf_reg after 1-cycle memory read latency
   always @ (posedge rst, posedge clk) begin
      if(rst)
	conf_ld <= 1'b0;
      else
	conf_ld <= ctr_valid & ~ctr_we;
   end

   //instantiate the config cache memory
   iob_1p_mem #(
	.DATA_W(`CONF_BITS),
	.ADDR_W(`CONF_MEM_ADDR_W))
   mem (
   	.clk(clk),
	.en(ctr_valid),
	.we(ctr_we),
	.addr(ctr_addr),
	.data_out(conf_out),
	.data_in(conf_in)
   );

endmodule

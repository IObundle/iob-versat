`timescale 1ns / 1ps
`include "xversat.vh"
`include "xmemdefs.vh"
`include "versat-io.vh"
`include "xaludefs.vh"
`include "xalulitedefs.vh"
`include "xmuldefs.vh"
`include "xmuladddefs.vh"
`include "xbsdefs.vh"
`include "xconfdefs.vh"

module xconf # (
	      parameter			   DATA_W = 32
	) (
              input                      clk,
              input                      rst,

              // Control bus interface
              input                      ctr_valid,
              input                      ctr_we,
              input [`CONF_REG_ADDR_W:0] ctr_addr,
              input [`IO_ADDR_W-1:0]     ctr_data_in,

              // configuration output to data engine
              output [`CONF_BITS-1:0]    conf_out
              );

   reg                                   conf_reg_valid;

`ifdef CONF_MEM_USE
   reg                                   conf_mem_valid;
   wire                                  conf_ld;
   wire [`CONF_BITS-1:0]                 conf_from_mem;
`endif
   

   // address decoder
   always @ * begin
`ifdef CONF_MEM_USE
      conf_mem_valid = 1'b0;
`endif
      conf_reg_valid = 1'b0;
      if (ctr_addr <  (`CONF_BS0  + `nBS*`BS_CONF_OFFSET) || ctr_addr == `CONF_CLEAR)
        conf_reg_valid = ctr_valid;
`ifdef CONF_MEM_USE
      else if(`CONF_MEM == (ctr_addr & ({`CONF_REG_ADDR_W+1{1'b1}}<<`CONF_MEM_ADDR_W))) begin
        conf_mem_valid = ctr_valid;
      end
`endif
      else if (ctr_valid)
        $display("Warning: unmapped config address %x at time %f", ctr_addr, $time);
   end
   
   
   //instantiate configuration register 
   xconf_reg # ( 
			.DATA_W(DATA_W)
   ) xconf_reg (
			.clk(clk),
			.rst(rst),

			//data interface
`ifdef CONF_MEM_USE
			.conf_in(conf_from_mem),
			.conf_ld(conf_ld),
`endif
			.conf_out(conf_out),

			//control interface
			.ctr_valid(conf_reg_valid),
			.ctr_we(ctr_we),
			.ctr_addr(ctr_addr),
			.ctr_data_in(ctr_data_in)
			);

`ifdef CONF_MEM_USE
   //instantiate configuration memory
   xconf_mem # ( 
		       .DATA_W(DATA_W)
   ) conf_mem (
		       .clk(clk),
		       .rst(rst),	

		       // data interface 
		       .conf_in(conf_out),
		       .conf_out(conf_from_mem),

		       // control interface
		       .ctr_valid(conf_mem_valid),
		       .ctr_we(ctr_we),
		       .ctr_addr(ctr_addr[`CONF_MEM_ADDR_W-1:0]),
		       .conf_ld(conf_ld)
		       );
`endif

endmodule

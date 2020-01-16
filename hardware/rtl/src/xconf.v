`timescale 1ns / 1ps
`include "xdefs.vh"
`include "xdata_engdefs.vh"
`include "xconfdefs.vh"
`include "xmemdefs.vh"
`include "xaludefs.vh"
`include "xalulitedefs.vh"
`include "xmuldefs.vh"
`include "xmuladddefs.vh"
`include "xbsdefs.vh"

module xconf (
        input                     clk,
        input                     rst,
        
        // Control bus interface
        input                     ctr_req,
        input                     ctr_rnw,
        input [`CONF_ADDR_W-1:0]    ctr_addr,
        input [`DADDR_W-1:0]      ctr_data,
        
        // configuration output to data engine
        output [`CONF_BITS-1:0]   conf_out
        );

   reg 				       conf_reg_req;

`ifdef CONF_MEM_USE
   reg 				       conf_mem_req;
   wire 			       conf_ld;
   wire [`CONF_BITS-1:0] 	       conf_from_mem;
`endif
   

   // address decoder
   always @ * begin
`ifdef CONF_MEM_USE
     conf_mem_req = 1'b0;
`endif
     conf_reg_req = 1'b0;
     if (ctr_addr < `CONF_REG_OFFSET)  
        conf_reg_req = ctr_req;
`ifdef CONF_MEM_USE
     else if(`CONF_MEM == (ctr_addr & ({`CONF_ADDR_W{1'b1}}<<`CONF_MEM_ADDR_W)))
        conf_mem_req = ctr_req;
`endif
     else if (ctr_req)
        $display("Warning: unmapped config address %x at time %f", ctr_addr, $time);
   end
   
 
   //instantiate configuration register 
   xconf_reg xconf_reg (
			.clk(clk),
			.rst(rst),

			//data interface
`ifdef CONF_MEM_USE
			.conf_in(conf_from_mem),
			.conf_ld(conf_ld),
`endif
			.conf_out(conf_out),

			//control interface
			.req(conf_reg_req),
			.rnw(ctr_rnw),
			.addr(ctr_addr[`CONF_REG_ADDR_W:0]),
			.data_in(ctr_data)
			);

`ifdef CONF_MEM_USE
   //instantiate configuration memory
   xconf_mem conf_mem (
		       .clk(clk),
		       .rst(rst),	

		       // data interface 
		       .conf_in(conf_out),
		       .conf_out(conf_from_mem),

		       // control interface
		       .req(conf_mem_req),
		       .rnw(ctr_rnw),
		       .addr(ctr_addr[`CONF_MEM_ADDR_W:0]),
		       .conf_ld(conf_ld)
		       );
`endif

endmodule

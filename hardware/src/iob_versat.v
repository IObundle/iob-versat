`timescale 1ns/1ps
`include "system.vh"
`include "iob_lib.vh"
`include "interconnect.vh"
`include "iob_versat.vh"

module iob_versat
  # (//the below parameters are used in cpu if includes below
    	parameter ADDR_W = `VERSAT_ADDR_W, //NODOC Address width
    	parameter DATA_W = `VERSAT_RDATA_W, //NODOC CPU data width
    	parameter WDATA_W = `VERSAT_WDATA_W //NODOC CPU data width
    )
  	(
   	//CPU interface
	`ifndef USE_AXI4LITE
 		`include "cpu_nat_s_if.v"
	`else
 		`include "cpu_axi4lite_s_if.v"
	`endif

   input clk,
   input rst
	);

versat_instance #(.ADDR_W(ADDR_W),.DATA_W(DATA_W)) xversat(
      .valid(valid),
      .we(|wstrb),
      .addr(address),
      .rdata(rdata),
      .wdata(wdata),
      .ready(ready),

      .clk(clk),
      .rst(rst)
   ); 

endmodule
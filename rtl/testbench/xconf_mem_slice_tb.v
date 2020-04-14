`timescale 1ns / 1ps
`include "xdefs.vh"

module xconf_mem_slice_tb;

   parameter clk_period = 20;

   // Inputs
   reg                        clk;
   reg                        en;

   // DMA
   reg 					      dma_req;
   reg 					      dma_rnw;
   reg [`CONF_REG_ADDR_W-1:0] dma_addr;
   reg [`DATA_W-1:0]          dma_data_in;
   wire [`DATA_W-1:0]         dma_data_out;

   // Configuration register
   reg                        conf_req;
   reg                        conf_rnw;
   reg [`CONF_REG_ADDR_W-1:0] conf_addr;
   reg [`DATA_W-1:0]          conf_data_in;
   wire [`DATA_W-1:0]         conf_data_out;

   // Instantiate the Unit Under Test (UUT)
   xconf_mem_slice uut (
				 .clk(clk),
				 .en(en),
				 .dma_req(dma_req),
				 .dma_rnw(dma_rnw),
				 .dma_addr(dma_addr),
				 .dma_data_in(dma_data_in),
				 .dma_data_out(dma_data_out),
				 .conf_req(conf_req),
				 .conf_rnw(conf_rnw),
				 .conf_addr(conf_addr),
				 .conf_data_in(conf_data_in),
				 .conf_data_out(conf_data_out)
				 );

   integer 				   i;

   initial begin
      
`ifdef DEBUG
      $dumpfile("xconf_mem_slice.vcd");
      $dumpvars();
`endif
      
      // Initialize inputs
      clk = 1'b1;
      conf_req = 1'b0;

      // Testing DMA write in all slice positions
      #1
      en = 1'b1;
      dma_rnw = 1'b0;
      dma_req = 1'b1;
      dma_addr = 0;
      dma_data_in = 0;

      for (i=0; i < 2**`CONF_REG_ADDR_W-1; i=i+1) begin
	 #clk_period
	 dma_addr = dma_addr + 1'b1;
	 dma_data_in = dma_data_in + 1'b1;
      end

      #clk_period
      dma_req = 1'b0;
      dma_data_in = `DATA_W'bx;

      // Testing DMA read from all slice positions
      #clk_period
      dma_rnw = 1'b1;
      dma_req = 1'b1;
      dma_addr = 0;

      for (i=0; i < 2**`CONF_REG_ADDR_W-1; i=i+1) begin
	 #clk_period
	 dma_addr = dma_addr + 1'b1;
      end

      #clk_period
      dma_req = 1'b0;
      dma_rnw = 1'bx;
      dma_addr = `CONF_REG_ADDR_W'bx;

      // Testing configuration register write in all slice positions
      #clk_period
      conf_rnw = 1'b0;
      conf_req = 1'b1;
      conf_addr = 0;
      conf_data_in = 0;

      for (i=0; i < 2**`CONF_REG_ADDR_W-1; i=i+1) begin
	 #clk_period
	 conf_addr = conf_addr + 1'b1;
	 conf_data_in = conf_data_in + 1'b1;
      end

      #clk_period
      conf_req = 1'b0;
      conf_data_in = `DATA_W'bx;

      // Testing configuration register read from all slice positions
      #clk_period
      conf_rnw = 1'b1;
      conf_req = 1'b1;
      conf_addr = 0;

      for (i=0; i < 2**`CONF_REG_ADDR_W-1; i=i+1) begin
	 #clk_period
	 conf_addr = conf_addr + 1'b1;
      end

      #clk_period
      conf_rnw = 1'bx;
      conf_req = 1'bx;
      conf_addr = `CONF_REG_ADDR_W'bx;

      // End simulation
      #(2*clk_period)
      $finish;

   end // initial begin

   always 
     #(clk_period/2) clk = ~clk;

endmodule

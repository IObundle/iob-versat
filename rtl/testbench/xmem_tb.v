`timescale 1ns / 1ps

`include "xdefs.vh"

module xmem_tb;

   //parameters 
   parameter clk_period = 20;
   
   // Inputs
   reg clk;
   reg rst;
   //reg [`MEM_ADDR_W-1:0] addr;
   
   //control 
   reg  	        initA;
   reg 			initB;
   
   wire [1:0] 		done;

   //controller interface
   reg 			rw_addrA_req;
   reg 			rw_addrB_req;
   
   reg 			rw_mem_req;
   reg 			rw_rnw;
   reg [`MEM_ADDR_W-1:0] 	rw_addr;
   reg [`DATA_W-1:0] 	rw_data_to_wr;
   
   //dma interface
   reg 			dma_rnw;
   reg [`MEM_ADDR_W-1:0] 	dma_addr;
   reg [`DATA_W-1:0] 	dma_data_in;
   reg 			dma_mem_req;
   
					 //input / output data
   reg [`N*`DATA_W-1:0] data_bus;
   wire [`DATA_W-1:0]   outA;
   wire [`DATA_W-1:0]   outB;
   
   //configurations
   reg [2*`MEMP_CONF_BITS-1:0] config_bits;
   
   // Instantiate the Unit Under Test (UUT)
   xmem uut (
	     //control 
	     .clk(clk),
	     .rst(rst),
	     .initA(initA),
	     .initB(initB),
	     .done(done),

	     //controller interface
	     .rw_addrA_req(rw_addrA_req),
	     .rw_addrB_req(rw_addrB_req),
	     
	     .rw_mem_req(rw_mem_req),
	     .rw_rnw(rw_rnw),
	     .rw_addr(rw_addr),
	     .rw_data_to_wr(rw_data_to_wr),
	     
	     //dma interface
 	     .dma_rnw(dma_rnw),
	     .dma_addr(dma_addr),
	     .dma_data_in(dma_data_in),
	     .dma_mem_req(dma_mem_req),

	     //input / output data
	     .data_bus(data_bus),
	     .outA(outA),
	     .outB(outB),
	     
	     //configurations
	     .config_bits(config_bits)
	     
	     );
   
   initial begin
      
      #100
      
`ifdef DEBUG
      $dumpfile("xmem.vcd");
      $dumpvars();
`endif
      
      // Initialize Inputs
      clk = 0;
      rst = 0;

      initA = 0;
      initB = 0;

      rw_addrA_req = 0;
      rw_addrB_req = 0;
      rw_mem_req = 0;
      rw_rnw = 1;
      config_bits = 0;

      dma_rnw = 1;
      dma_addr = 0;
      dma_data_in = 0;
      dma_mem_req = 0;
      
      // Global reset
      #(clk_period+1) rst = 1;

      #clk_period rst = 0;
      
      #clk_period rw_mem_req = 1;
      rw_rnw = 0;
      rw_addr = 10;
      
      rw_data_to_wr = 32'hF0F0F0F0;

      #clk_period rw_mem_req = 0;
      #clk_period rw_mem_req = 1;
      rw_rnw = 1;

      #clk_period rw_mem_req = 0;

      // Simulation time
      #(10000) $finish;
   end
	
   always 
     #(clk_period/2) clk = ~clk;
      	 
endmodule

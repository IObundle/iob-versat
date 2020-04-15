`timescale 1ns / 1ps
`include "xdefs.vh"
`include "./xaludefs.vh"

module xconf_mem_tb;
   
   //parameters 
   parameter clk_period = 40;

   //sizes
   parameter MEM_SIZE = 2**`MEM_ADDR_W;
   
   reg clk;
   reg rst;
   
   //parallel interface
   reg 	[`CTRL_REGF_ADDR_W-1:0] par_addr;
   reg 				par_we;
   reg [`DATA_W-1:0] 		par_in;
   wire [`DATA_W-1:0] 		par_out;
   reg [`DATA_W-1:0] 		data_addr;
   
   //dma interface 
   reg 				dma_req;
   reg 				dma_rnw;
   reg [`ADDR_W-1:0] 	dma_addr;
   reg [`DATA_W-1:0] 		dma_data_in;
   wire [`DATA_W-1:0] 		dma_data_out;
   
   // Inputs
   reg [`CONF_BITS+`DATA_W*`N_CONF_SLICES-`CONF_BITS-1:0] 	conf_data_in;

   // Outputs
   reg [`CONF_BITS+`DATA_W*`N_CONF_SLICES-`CONF_BITS-1:0] 	conf_data_out;
   
   //configs 
   reg [`ALU_FNS_W-1:0] 	alu_fns;
   reg [`N_W-1:0] 		sela;
   reg [`N_W-1:0] 		selb;
   
   reg [`MEM_ADDR_W-1:0] 		iterations;
   reg [`PERIOD_W-1:0] 		period;
   reg [`PERIOD_W-1:0] 		duty;

   reg [`MEM_ADDR_W-1:0] 		start;
   reg signed [`MEM_ADDR_W-1:0] 	shift;
   reg signed [`MEM_ADDR_W-1:0] 	incr;
   reg [`PERIOD_W-1:0] 		delay;
   reg 				reverse;
   reg 				ext;
   
   integer 			i, k, z, fp;
   
   // Data Bank
   reg [`DATA_W-1:0] data [MEM_SIZE-1:0];

   
   // Instantiate the Unit Under Test (UUT)
   xtop uut (
	     .clk(clk), 
	     .rst(rst),
	     // parallel interface
	     .par_addr(par_addr),
	     .par_we(par_we),
	     .par_in(par_in),
	     .par_out(par_out),

	     .dma_req(dma_req),
	     .dma_rnw(dma_rnw),
	     .dma_addr(dma_addr),
	     .dma_data_in(dma_data_in),
	     .dma_data_out(dma_data_out)
	     );
   
   initial begin
      
`ifdef DEBUG
      $dumpfile("xconf_mem.vcd");
      $dumpvars();
`endif
      
      dma_req = 0;
      dma_rnw = 1;
      dma_addr = 0;
      dma_data_in = 0;
      conf_data_in = {(`CONF_BITS+`DATA_W*`N_CONF_SLICES-`CONF_BITS){1'b0}};
      
      // Global reset of FPGA
      #100
 
      // Initialize Inputs
      clk = 0;
      rst = 0;

      //parallel interface
      par_addr = 0;
      par_we = 0;
      par_in = 0;
      
      // Global reset
      #(clk_period+1)
      rst = 1;

      #clk_period
      rst = 0;
      
      #clk_period;
      
      //
      //example configuration
      //
      
      //configuration reset (set all parameters 0)
      conf_data_in = 0;
      
      //setup memory 0 for reading vector a and b
      sela = `sdisabled;
      selb = `sdisabled;
      start = 1;
      shift = 0;
      incr = 1;
      delay = 0;
      reverse = 0;
      ext = 0;
      iterations = 19;
      period = 0;
      duty = 0;
      //setup port A
      conf_data_in[`MEM0A_CONF_OFFSET+`DATA_W*`N_CONF_SLICES-`CONF_BITS-1 -: `MEMP_CONF_BITS] = 
												      {iterations, period, duty, sela, start, shift, incr, delay, reverse, ext};
      
      start = 21;
      //setup port B
      conf_data_in[`MEM0B_CONF_OFFSET+`DATA_W*`N_CONF_SLICES-`CONF_BITS-1 -: `MEMP_CONF_BITS] = 
												      {iterations, period, duty, selb, start, shift, incr, delay, reverse, ext};
      
      //setup memory 1 for storing vector a + b
      sela = `sALU1;
      start = 0;
      incr = 1;
      delay = 3;
      reverse = 0;
      ext = 0;
      iterations = 19;
      period = 0;
      duty = 0;
      //setup port A
      conf_data_in[`MEM1A_CONF_OFFSET+`DATA_W*`N_CONF_SLICES-`CONF_BITS-1 -: `MEMP_CONF_BITS] = 
												      {iterations, period, duty, sela, start, shift, incr, delay, reverse, ext};

      //setup alu3 for adding a and b 
      alu_fns = `ALU_ADD;
      sela = `sMEM0A;
      selb = `sMEM0B;
      
      conf_data_in[`ALU1_CONF_OFFSET+`DATA_W*`N_CONF_SLICES-`CONF_BITS-1 -: `ALU_CONF_BITS] = 
												    {sela, selb, alu_fns};
      
      //
      //load configuration
      //
      
      dma_req = 1;
      dma_rnw = 0;
      dma_addr = `CONF_MEM_BASE;
      
      for (i=0; i < `N_CONF_SLICES; i=i+1) begin
	 
	 dma_data_in = conf_data_in[`N_CONF_SLICES*`DATA_W-i*`DATA_W-1 -:`DATA_W];
	 #clk_period;
	 
	 dma_addr = dma_addr + `N_CONF_SLICE_W'b1;
      end
      
      dma_req = 0;
      dma_rnw = 1;
      
      #(2*clk_period);
      
      //
      //read configuration
      //
      
      dma_addr = `CONF_MEM_BASE;
      
      fp = $fopen("./xconf_mem.out","w");

      dma_req = 1;
      dma_addr = `CONF_MEM_BASE;
      
      for (i=0; i < (`N_CONF_SLICES+2); i=i+1) begin 

	 if(i > 1)
	   conf_data_out[`N_CONF_SLICES*`DATA_W-(i-2)*`DATA_W-1 -:`DATA_W] = dma_data_out;
	 
	 #clk_period;
	 
	 if (i < `N_CONF_SLICES)
	   dma_addr = dma_addr + `N_CONF_SLICE_W'b1;

	 if (i >= (`N_CONF_SLICES-1))
	   dma_req = 0;
      end
      
      #(2*clk_period);
      
      if (conf_data_out == {uut.conf.conf_mem.conf_data_out,{(`DATA_W*`N_CONF_SLICES-`CONF_BITS){1'b0}}} && conf_data_in == {uut.conf.conf_mem.conf_data_out,{(`DATA_W*`N_CONF_SLICES-`CONF_BITS){1'b0}}})
	$fwrite(fp,"passed\n");
      else
	$fwrite(fp,"falied\n");
      
      $fclose(fp);
      
      // Simulation time
      #(1000) $finish;
      
   end

   always 
     #(clk_period/2) clk = ~clk;

endmodule

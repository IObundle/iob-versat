/* ****************************************************************************

  Description: Testbench to run Versat
               Loads program and data from file
               Starts the program
               Detects program end
               Dumps data to file

               R0: Versat starts execution whenever this register is non-zero.
                   Simulation finishes when Versat resets this register.

***************************************************************************** */

`timescale 1ns / 1ps
`include "xdefs.vh"
`include "xprogdefs.vh"
`include "xregfdefs.vh"
`include "xdata_engdefs.vh"

module xtop_tb;

   //parameters
   parameter clk_period = 12;

   reg clk;
   reg rst;

   //parallel interface
   reg 	[`REGF_ADDR_W-1:0] par_addr;
   reg 			   par_we;
   reg [`DATA_W-1:0] 	   par_in;
   wire [`DATA_W-1:0] 	   par_out;
   reg [`DATA_W-1:0] 	   data_addr;

   //dma interface
   reg 			   dma_req;
   reg 			   dma_rnw;
   reg [`ADDR_W-1:0] 	   dma_addr;
   reg [`DATA_W-1:0] 	   dma_data_from;
   wire [`DATA_W-1:0] 	   dma_data_to;

   wire 		   ctr_timer_en;
   integer 		   k, start_time, ctr_timer;

   // Testbench data
   // memory
   reg [`DATA_W-1:0] 	   data [`MEM_DATA_SZ + 2**`REGF_ADDR_W-1:0];

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
	     .dma_data_from(dma_data_from),
	     .dma_data_to(dma_data_to)
	     );

/*

 BEGIN TEST PROGRAM

 */

   initial begin

`ifdef DEBUG
      $dumpfile("xtop_tb.vcd");
      $dumpvars();
`endif

      // Initialize clk and rst
      clk = 1;
      rst = 0;

      // Initialize control interface
      par_we = 0;
      par_addr = 1;

      // Initialize DMA interface
      dma_req = 0;
      dma_rnw = 1;
      dma_addr = `ENG_BASE;
      dma_data_from = 0;

      // Pulse reset
      #(clk_period*1.1) rst = 1;
      #clk_period rst = 0;

`ifdef PROG_MEM_USE
      //
      // Load RAM program
      //

      // insert code here to load RAM program via DMA
`endif


      //
      // Load data from infile
      //

      // Read data from input file
      $display("Engine memory size: ", `MEM_DATA_SZ);
      $readmemh("./data_in.hex", data, 0,  `MEM_DATA_SZ + 2**`REGF_ADDR_W - 1);


      // Load registers using parallel interface
      #(10*clk_period) par_we = 1;

      for (k = 1; k < 2**`REGF_ADDR_W; k=k+1) begin
	 par_in = data[`MEM_DATA_SZ + k];
	 #clk_period par_addr = par_addr + 1;
      end

      par_we = 0;

     // Load data memories using dma interface
      dma_req = 1;
      dma_rnw = 0;

      for (k = 0; k < `MEM_DATA_SZ; k=k+1) begin
	 dma_data_from = data[k];
	 #clk_period dma_addr = dma_addr + 1;
      end

      dma_req = 0;
      dma_rnw = 1;


      //
      // Start Versat (write non-zero value to R0)
      //

      par_addr = 0;
      par_in = 1;
      par_we = 1;
      #clk_period par_we = 0;

      start_time = $time;

      //wait for versat to reset R0
      while(par_out != 0) #clk_period;

      $display("Total engine run time: %0d",($time-start_time)/clk_period);
      $display("Controller Clocks: %0d", ctr_timer);

      //
      // Dump data to outfile
      //

      // memories

      dma_req = 1;
      dma_addr = `ENG_BASE;
      for (k = 0; k < `MEM_DATA_SZ; k=k+1) begin
	 #clk_period data[k] = dma_data_to;
	 dma_addr = dma_addr + 1;
      end
      dma_req = 0;

      // registers

      par_addr = 0;
      for (k = `MEM_DATA_SZ; k < (`MEM_DATA_SZ + 2**`REGF_ADDR_W); k=k+1) begin
	   #clk_period data[k] = par_out;
	   par_addr = par_addr + 1;
      end

      $writememh("data_out.hex", data, 0, `MEM_DATA_SZ + 2**`REGF_ADDR_W - 1);

      //
      // End simulation
      //
      $display("Total run time: %d",$time/clk_period);
      #clk_period $finish;

   end // initial begin


   //Measure unhidden controller time
   assign ctr_timer_en = (& uut.data_eng.mem_done) & ~dma_req
`ifdef DIV
			 & uut.div_wrapper.ready
`endif
			 ; //ends assign statement

   always @ (posedge clk) begin
      if(rst == 1)
	ctr_timer = 0;
      else if (ctr_timer_en)
	ctr_timer = ctr_timer+1;
   end

   // Clock generation
   always
     #(clk_period/2) clk = ~clk;

   // Create some useful signals


   wire [`DATA_W-1:0]     r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15;
   wire [`DATA_W-1:0]     regA, regB, regC;
   wire [`PROG_ADDR_W:0]  pc;
   wire [`INSTR_W-1:0]    instr;
   wire [`ADDR_W-1:0] 	  rw_addr;
   wire 		  rw_req, rw_rnw;

   wire [`DATA_W-1:0] data_mem0A;
   wire [`DATA_W-1:0] data_mem0B;
   wire [`DATA_W-1:0] data_mem1A;
   wire [`DATA_W-1:0] data_mem1B;
   wire [`DATA_W-1:0] data_mem2A;
   wire [`DATA_W-1:0] data_mem2B;
   wire [`DATA_W-1:0] data_mem3A;
   wire [`DATA_W-1:0] data_mem3B;

   assign data_mem0A =  uut.data_eng.data_bus[`DATA_MEM0A_B -: `DATA_W];
   assign data_mem0B =  uut.data_eng.data_bus[`DATA_MEM0A_B-`DATA_W -: `DATA_W];
   assign data_mem1A =  uut.data_eng.data_bus[`DATA_MEM0A_B-2*`DATA_W -: `DATA_W];
   assign data_mem1B =  uut.data_eng.data_bus[`DATA_MEM0A_B-3*`DATA_W -: `DATA_W];
   assign data_mem2A =  uut.data_eng.data_bus[`DATA_MEM0A_B-4*`DATA_W -: `DATA_W];
   assign data_mem2B =  uut.data_eng.data_bus[`DATA_MEM0A_B-5*`DATA_W -: `DATA_W];
   assign data_mem3A =  uut.data_eng.data_bus[`DATA_MEM0A_B-6*`DATA_W -: `DATA_W];
   assign data_mem3B =  uut.data_eng.data_bus[`DATA_MEM0A_B-7*`DATA_W -: `DATA_W];



   assign pc = uut.pc;
   assign instr = uut.instruction;
   assign regA = uut.controller.regA;
   assign regB = uut.controller.regB;
   assign regC = uut.controller.cs;
   assign rw_addr = uut.controller.rw_addr;
   assign rw_req = uut.controller.rw_req;
   assign rw_rnw = uut.controller.rw_rnw;

   assign r0 = uut.regf.reg_1[0];
   assign r1 = uut.regf.reg_1[1];
   assign r2 = uut.regf.reg_1[2];
   assign r3 = uut.regf.reg_1[3];
   assign r4 = uut.regf.reg_1[4];
   assign r5 = uut.regf.reg_1[5];
   assign r6 = uut.regf.reg_1[6];
   assign r7 = uut.regf.reg_1[7];
   assign r8 = uut.regf.reg_1[8];
   assign r9 = uut.regf.reg_1[9];
   assign r10 = uut.regf.reg_1[10];
   assign r11 = uut.regf.reg_1[11];
   assign r12 = uut.regf.reg_1[12];
   assign r13 = uut.regf.reg_1[13];
   assign r14 = uut.regf.reg_1[14];
   assign r15 = uut.regf.reg_1[15];

endmodule

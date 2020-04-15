/* ****************************************************************************

  Description: Testbench to run Versat
               Loads configuration and data from file
               Starts the program
               Detects program end
               Dumps data to file

***************************************************************************** */

`timescale 1ns / 1ps
`include "xversat.vh"
//total versat memory
`define MEM_DATA_SZ (nMEM * 2**`MEM_ADDR_W)

//
// MEMORY MAP
//
`define CTR_ENG_CTR_ADDR (1<<(`nMEM_W+`MEM_ADDR_W))
`define CTR_CONF_REG_ADDR (2'b10<<(`nMEM_W+`MEM_ADDR_W-1))
`define CTR_CONF_REG_CTR_ADDR (3'b101<<(`nMEM_W+`MEM_ADDR_W-2))
`define CTR_CONF_MEM_ADDR (3'b110<<(`nMEM_W+`MEM_ADDR_W-2))

module xtop_tb;

   //parameters
   parameter clk_period = 10;

   reg clk;
   reg rst;

   //ctr interface
   reg 			   ctr_valid;
   reg [`ADDR_W:0]         ctr_addr;
   reg 			   ctr_we;
   reg [`DATA_W-1:0]       ctr_data_in;
   wire [`DATA_W-1:0]      ctr_data_out;

   //data interface
   reg 			   data_valid;
   reg 			   data_we;
   reg [`ADDR_W-1:0] 	   data_addr;
   reg [`DATA_W-1:0] 	   data_data_in;
   wire [`DATA_W-1:0] 	   data_data_out;


   // Testbench data
   // memory
   reg [`DATA_W-1:0] 	   data [`MEM_DATA_SZ-1:0];

   // Instantiate the Unit Under Test (UUT)
   xversat uut (
	     .clk(clk),
	     .rst(rst),
	     // parallel interface
	     .ctr_addr(ctr_addr),
	     .ctr_we(ctr_we),
	     .ctr_data_in(ctr_data_in),
	     .ctr_data_out(ctr_data_out),

	     .data_valid(data_valid),
	     .data_we(data_we),
	     .data_addr(data_addr),
	     .data_data_in(data_data_in),
	     .data_data_out(data_data_out)
	     );

/*

 BEGIN TEST PROGRAM

 */
   wire 		   sys_timer_en;
   integer 		   k, sys_timer;
   integer                 start_time = $time;

   initial begin

`ifdef DEBUG
      $dumpfile("xtop_tb.vcd");
      $dumpvars();
`endif

      // Initialize clk and rst
      clk = 1;
      rst = 0;

      // Initialize control interface
      ctr_we = 0;
      ctr_addr = 1;

      // Initialize data interface
      data_valid = 0;
      data_we = 1;
      data_addr = 0;
      data_data_in = 0;

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
      //$readmemh("./data_in.hex", data, 0,  `MEM_DATA_SZ - 1);

      // Load data memories using data interface
      /*
       data_valid = 1;
       data_we = 0;

       for (k = 0; k < `MEM_DATA_SZ; k=k+1) begin
       data_data_in = data[k];
       #clk_period data_addr = data_addr + 1;
      end
       
       data_valid = 0;
       data_we = 1;
       */

      //
      // Start Versat (write non-zero value to R0)
      //

      ctr_control_addr = `CTR_ENG_CTR_ADDR;
      ctr_data_in = 1;
      ctr_we = 1;
      #clk_period ctr_we = 0;


      $display("Total engine run time: %0d",($time-start_time)/clk_period);
      $display("Controller Clocks: %0d", sys_timer);

      //
      // Dump data to outfile
      //

      /*
       data_valid = 1;
       data_addr = `ENG_BASE;
       for (k = 0; k < `MEM_DATA_SZ; k=k+1) begin
       #clk_period data[k] = data_data_out;
       data_addr = data_addr + 1;
      end
       data_valid = 0;

       $writememh("data_out.hex", data, 0, `MEM_DATA_SZ - 1);
       */

      //
      // End simulation
      //
      $display("Total run time: %d",$time/clk_period);
      #clk_period $finish;

   end // initial begin


   //Measure unhidden controller time
   assign sys_timer_en = (& uut.data_eng.mem_done) & ~data_valid;
   

   //system timer                 
   always @ (posedge clk) begin
      if(rst == 1)
	ctr_timer = 0;
      else if (ctr_timer_en)
	ctr_timer = ctr_timer+1;
   end

   // Clock generation
   always
     #(clk_period/2) clk = ~clk;

 
endmodule

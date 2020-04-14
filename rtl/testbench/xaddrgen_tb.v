`timescale 1ns / 1ps
`include "xdefs.vh"

module addrgen_tb;

   // Inputs
   reg clk;
   reg rst;
   reg en;
   
   reg [`DADDR_W - 1:0]  iterations;
   reg [`PERIOD_W - 1:0] period;
   reg [`PERIOD_W - 1:0] duty;
   reg [`PERIOD_W - 1:0] delay;
   reg [`DADDR_W - 1:0]  start;
   reg [`DADDR_W - 1:0]  end_loop;
   reg [`DADDR_W - 1:0]  incr;

   //Outputs 
   wire [`DADDR_W - 1:0] addr;
   wire 		 mem_en;
   wire 		 done;
   
   parameter clk_per = 20;
   
   // Instantiate the Unit Under Test (UUT)
   addrgen uut (
		.clk(clk), 
		.rst(rst), 
		.en(en),
		.iterations(iterations), 
		.period(period), 
		.duty(duty), 
		.delay(delay), 
		.start(start),
		.end_loop(end_loop),
		.incr(incr), 
		.addr(addr), 
		.mem_en(mem_en), 
		.done(done)
	);

   initial begin
      
`ifdef DEBUG
      $dumpfile("xaddrgen.vcd");
      $dumpvars();   
`endif
      
      // Initialize Inputs
      clk = 0;
      rst = 0;
      en = 0;
      iterations = 2;
      period = 5;
      duty = 2;
      delay = 4;
		
      start = 12;
      end_loop = 0;
      incr = 5;

      // Wait 100 ns for global reset to finish
      #21 rst = 1;
      #20 rst = 0;
      
      // Add stimulus here
      
      //Simulation time 550 ns
      #(550-41) $finish;
   end
	
   always 
     #(clk_per/2) clk = ~clk;
   
   always 
     #clk_per en = ~en;
      	 
endmodule


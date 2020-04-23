`timescale 1ns / 1ps
`include "xversat.vh"

module addrgen_tb;

   // Inputs
   reg clk;
   reg rst;
   reg init;
   reg run;
   
   //configurations
   reg [`MEM_ADDR_W - 1:0]  	iterations;
   reg [`PERIOD_W - 1:0] 	period;
   reg [`PERIOD_W - 1:0] 	duty;
   reg [`PERIOD_W - 1:0] 	delay;
   reg [`MEM_ADDR_W - 1:0]  	start;
   reg [`MEM_ADDR_W - 1:0]  	shift;
   reg [`MEM_ADDR_W - 1:0]  	incr;

   //Outputs 
   wire [`MEM_ADDR_W - 1:0] 	addr;
   wire 		 	mem_en;
   wire 		 	done;
   
   parameter clk_per = 20;
   integer i, j, aux_addr;
  
   // Instantiate the Unit Under Test (UUT)
   xaddrgen uut (
		.clk(clk), 
		.rst(rst), 
		.init(init),
		.run(run),
		.iterations(iterations), 
		.period(period), 
		.duty(duty), 
		.delay(delay), 
		.start(start),
		.shift(shift),
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
      rst = 1;
      init = 0;
      run = 0;

      //configurations
      //simulate reading first 3x3 block of 5x5 feature map
      iterations = 3;
      period = 3;
      duty = 3;
      delay = 0;
      start = 0;      
      shift = 5-3;
      incr = 1;

      //display addr values that should be generated
      $display("\nExpected addr values");
      aux_addr = start;
      for(i = 0; i < iterations; i++) begin
        for(j = 0; j < period; j++) begin
          $display("Address %d", aux_addr);
          aux_addr += incr;
        end
        aux_addr += shift;
      end
      $display("\nActual addr values");
      
      // Wait 100 ns for global reset to finish
      #(clk_per*5) rst = 0;
      
      //run
      init = 1;
      run = 1;
      #(clk_per);
      init = 0;
      run = 0;

      //wait until done
      do begin
        if(mem_en) $display("Address %d", addr);
        @(posedge clk) #1;
      end while(done == 0);
      $finish;      

   end
	
   always 
     #(clk_per/2) clk = ~clk;
      	 
endmodule


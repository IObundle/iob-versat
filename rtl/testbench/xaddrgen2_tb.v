`timescale 1ns / 1ps
`include "xversat.vh"

module xaddrgen2_tb;

   // Inputs
   reg clk;
   reg rst;
   reg run;

   //configurations
   reg [`MEM_ADDR_W - 1:0]  	iterA, iterB;
   reg [`PERIOD_W - 1:0] 	perA, perB;
   reg [`PERIOD_W - 1:0] 	duty;
   reg [`PERIOD_W - 1:0] 	delay;
   reg [`MEM_ADDR_W - 1:0]  	start;
   reg [`MEM_ADDR_W - 1:0]  	shiftA, shiftB;
   reg [`MEM_ADDR_W - 1:0]  	incrA, incrB;

   //Outputs
   wire [`MEM_ADDR_W - 1:0] 	addr;
   wire 		 	mem_en;
   wire 		 	done;

   //simulation variables
   parameter clk_per = 20;
   integer i, j, k, l, aux_addr, start_aux, cnt_err = 0;

   // Instantiate the Unit Under Test (UUT)
   xaddrgen2 addrgen2 (
	.clk(clk),
	.rst(rst),
	.run(run),
	.iterations(iterA),
	.period(perA),
	.duty(duty),
	.delay(delay),
	.start(start),
	.shift(shiftA),
	.incr(incrA),
	.iterations2(iterB),
	.period2(perB),
	.shift2(shiftB),
	.incr2(incrB),
	.addr(addr),
	.mem_en(mem_en),
	.done(done)
	);

   initial begin

`ifdef DEBUG
      $dumpfile("xaddrgen_4loop.vcd");
      $dumpvars();
`endif

      // Initialize Inputs
      clk = 0;
      rst = 1;
      run = 0;

      //configurations
      //simulate reading 3x3 blocks from 5x5 feature map
      iterA = 3;
      perA = 3;
      duty = 3;
      delay = 0;
      start = 0;
      shiftA = 5-3;
      incrA = 1;
      iterB = 3;
      perB = 3;
      shiftB = 5-3;
      incrB = 1;
      start_aux = start;

      // Wait 100 ns for global reset to finish
      #(clk_per*5) rst = 0;

      //run
      run = 1;
      #(clk_per);
      run = 0;

      //wait until done
      do begin

        //compare expected and actual addr
        for(i = 0; i < iterB; i++) begin
          for(j = 0; j < perB; j++) begin
            aux_addr = start_aux;
            for(k = 0; k < iterA; k++) begin
              for(l = 0; l < perA; l++) begin
                if(aux_addr != addr) cnt_err++;
                aux_addr += incrA;
                #clk_per;
              end
              aux_addr += shiftA;
            end
            start_aux += incrB;
          end
          start_aux += shiftB;
        end
      end while(done == 0);

      //end simulation
      if(cnt_err == 0) $display("\n4-loop addrgen ran successfully\n");
      else $display("\n4-loop addrgen fail with %d errors\n", cnt_err);
      $finish;

   end

   always
     #(clk_per/2) clk = ~clk;

endmodule

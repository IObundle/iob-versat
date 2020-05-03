/* ****************************************************************************

  Description: Testbench to run the multiplier
               Generates test data
               Applies test data
               Verifies results

***************************************************************************** */

`timescale 1ns / 1ps
`define NR_RND_VECS 200
`define NR_SPECIAL_VECS 8
`define NR_VECS (`NR_RND_VECS + `NR_SPECIAL_VECS)

module xmul_pipe_tb;

   //parameters
   parameter clk_period = 10;
   parameter DATA_W = 16;
   
   //loop iterator
   integer i, j, ok_flag = 1;

   // product debug register
   reg [2*DATA_W-1:0] 	      p_reg;


   // signed results and expected results
   reg signed [2*DATA_W-1:0] sres [`NR_VECS-1:0];
   reg signed [2*DATA_W-1:0] sexp [`NR_VECS-1:0];

   reg clk;
   reg rst;

   //interface signals
   reg [DATA_W-1:0] op_a;
   reg [DATA_W-1:0] op_b;
   wire [2*DATA_W-1:0] product;
   
   //signed test vectors
   reg signed [DATA_W-1:0] a [`NR_VECS-1:0];
   reg signed [DATA_W-1:0] b [`NR_VECS-1:0];

   // Instantiate the Unit Under Test (UUT)
   xmul_pipe # ( 
		  .DATA_W(DATA_W)
   ) uut (
                  .rst                  (rst),
                  .clk                  (clk),

                  .op_a                 (op_a[DATA_W-1:0]),
                  .op_b                 (op_b[DATA_W-1:0]),
		  .product              (product[2*DATA_W-1:0])
		  );
   
/*

 BEGIN TEST PROGRAM

 */

   initial begin

`ifdef DEBUG
      $dumpfile("xmul_pipe.vcd");
      $dumpvars();
`endif

      // Initialize clk and rst
      clk = 1;
      rst = 0;

      //
      // Generate test data
      //

      //generate vector of random numbers of worst case width (64-bit)
      for (j = 0; j < `NR_RND_VECS; j=j+1) begin 
	 a[j] = $random;
	 b[j] = $random;
      end

      //generate special cases
      
      //multiply by 0
      a[j] = 0;     
      b[j] = 1; j=j+1;            

      a[j] = 1;     
      b[j] = 0; j=j+1;
      
      //mutiply by 1
      a[j] = 10;     
      b[j] = 1; j=j+1;            
 
      a[j] = 1;    
      b[j] = 10; j=j+1;            


      // expected results
      for (j = 0; j < `NR_VECS; j=j+1) begin
	 sexp[j] = a[j]*b[j];
      end


      // Pulse reset 
      @(posedge clk) #1 rst = 1;
      @(posedge clk) #1 rst = 0;
      

      //
      // Apply tests and check results
      //


      for (j = 0; j < `NR_VECS; j=j+1) begin
	 op_a = a[j];
	 op_b = b[j];
         @(posedge clk) #1;
      end

   end // initial begin

   //
   // Capture results
   //


   initial begin
      @(posedge rst);
      @(negedge clk);
      @(posedge clk) #1;
      @(posedge clk) #1;
      @(posedge clk) #1;
      @(posedge clk) #1;
      
      for (i = 0; i < `NR_VECS; i=i+1) begin
	 sres[i] = product;
         @(posedge clk) #1;
      end

      //check results
      for (i = 0; i < `NR_VECS; i=i+1) begin
	 if(sres[i] != sexp[i]) begin
	   $display("#%d: failed signed: %d * %d = %d != %d", i, a[i], b[i], sexp[i], sres[i]);
	   ok_flag = 0;
         end 
      end
      if(ok_flag) $display("\nmul_pipe test passed");
      else $display("\nmul_pipe test failed");

      //
      // End simulation
      //
      $display("Simulation finished\n");
      #clk_period $finish;

   end
   
   // Clock generation
   always
     #(clk_period/2) clk = ~clk;
   
endmodule

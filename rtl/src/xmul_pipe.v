`timescale 1ns / 1ps

/* 
 Module: 2-stage pipeline integer multiplier
 */

module xmul_pipe # ( 
	       parameter		  DATA_W = 32
	) (
	       input                      rst,
	       input                      clk,

	       //data 
	       input [DATA_W-1:0]         op_a,
	       input [DATA_W-1:0]         op_b,

               output reg [2*DATA_W-1:0]  product
	       );

   //input registers
   reg [DATA_W-1:0]                       op_a_reg;
   reg [DATA_W-1:0]                       op_b_reg;

   // half words
   wire signed [DATA_W/2:0]               a_l;
   wire signed [DATA_W/2:0]               a_h;
   wire signed [DATA_W/2:0]               b_l;
   wire signed [DATA_W/2:0]               b_h;

   //stage 1 registers
   reg signed [DATA_W+1:0]                blXal;
   reg signed [DATA_W+1:0]                blXah;
   reg signed [DATA_W+1:0]                bhXal;
   reg signed [DATA_W+1:0]                bhXah;
   

   // compute partial operands
   assign a_l = {1'b0, op_a_reg[DATA_W/2-1:0]};
   assign a_h = {op_a_reg[DATA_W-1], op_a_reg[DATA_W-1:DATA_W/2]};
   assign b_l = {1'b0, op_b_reg[DATA_W/2-1:0]};
   assign b_h = {op_b_reg[DATA_W-1], op_b_reg[DATA_W-1:DATA_W/2]};
   
   
   //update registers 
   always @ (posedge clk) 
     if (rst) begin
        // stage 0
        op_a_reg <= {DATA_W{1'b0}};
        op_b_reg <= {DATA_W{1'b0}};
	//stage 1
	blXal <= {DATA_W+2{1'b0}};
	blXah <= {DATA_W+2{1'b0}};
	bhXal <= {DATA_W+2{1'b0}}; 
	bhXah <= {DATA_W+2{1'b0}};
        //stage 2
        product <= {2*DATA_W{1'b0}};
     end else begin
        // stage 0
        op_a_reg <= op_a;   
        op_b_reg <= op_b;

	//stage 1
	blXal <= a_l*b_l; //partial products
	blXah <= a_h*b_l;
	bhXal <= a_l*b_h;
	bhXah <= a_h*b_h;	

	//stage 2
	product <= {{DATA_W-2{1'd0}}, blXal} + {{DATA_W/2-2{blXah[DATA_W+1]}}, blXah, {DATA_W/2{1'd0}}} + {{DATA_W/2-1{bhXal[DATA_W+1]}}, bhXal, {DATA_W/2{1'd0}}} + {bhXah[DATA_W-1:0], {DATA_W{1'd0}}};
     end

endmodule // xpmult

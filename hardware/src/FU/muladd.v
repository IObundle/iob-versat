`timescale 1ns / 1ps

`define DATA_W 16

module muladd(
	input	clk,
	input	rst, 

	input [31:0] in1,
	input [31:0] in2,
	output [31:0] out
	);

   reg signed [DATA_W-1:0] op_a_reg, op_b_reg;
   reg signed [2*DATA_W-1:0] acc, dsp_out;
   wire signed [2*DATA_W-1:0] acc_w;
   
   //DSP48E template
   always @ (posedge clk,posedge rst) begin
		if(rst) begin
			acc <= 0;
		end else begin
			op_a_reg <= in1[15:0];
	   	op_b_reg <= in2[15:0];
	     	result_mult_reg <= op_a_reg * op_b_reg;
	     	acc <= acc + result_mult_reg;
	     	dsp_out <= acc;
     	end
   end
   
   assign out = dsp_out;

end
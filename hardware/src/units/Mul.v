`timescale 1ns / 1ps

/*

 Description: multiply functions

 */

module Mul #(
   parameter DATA_W = 32
) (
   input rst,
   input clk,
   input run,
   input running,

   input [DATA_W-1:0] in0,
   input [DATA_W-1:0] in1,

   (* versat_latency = 4 *) output [DATA_W-1:0] out0,

   // config interface
   input [31:0] delay0
);

   // select multiplier statically
   reg signed [DATA_W-1:0] in0_reg, in1_reg;
   reg signed [2*DATA_W-1:0] acc, dsp_out0, result_mult_reg;
   //DSP48E template
   always @(posedge clk, posedge rst) begin
      if (rst) begin
         in0_reg  <= 0;
         in1_reg  <= 0;
         acc      <= 0;
         dsp_out0 <= 0;
      end else begin
         in0_reg         <= in0;
         in1_reg         <= in1;
         result_mult_reg <= in0_reg * in1_reg;
         acc             <= result_mult_reg;
         dsp_out0        <= acc;
      end
   end

   assign out0 = dsp_out0[31:0];

endmodule

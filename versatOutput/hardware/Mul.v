`timescale 1ns / 1ps

/*

 Description: multiply functions

 */

module Mul #(
   parameter DATA_W = 32
) (
   input rst,
   input clk,
   input running,

   input [DATA_W-1:0] in0,
   input [DATA_W-1:0] in1,

   (* versat_latency = 4 *) output [DATA_W-1:0] out0

);

   // select multiplier statically
   reg signed [DATA_W-1:0] in0_reg, in1_reg, dsp_out0, acc, result_mult_reg;
   //DSP48E template
   always @(posedge clk, posedge rst) begin
      if (rst) begin
         in0_reg         <= {DATA_W{1'b0}};
         in1_reg         <= {DATA_W{1'b0}};
         dsp_out0        <= {DATA_W{1'b0}};
         result_mult_reg <= {DATA_W{1'b0}};
         acc             <= {DATA_W{1'b0}};
      end else if (running) begin
         in0_reg         <= in0;
         in1_reg         <= in1;
         result_mult_reg <= in0_reg * in1_reg;
         acc             <= result_mult_reg;
         dsp_out0        <= acc;
      end
   end

   assign out0 = dsp_out0;

endmodule

`timescale 1ns / 1ps

module Conditional #(
   parameter DELAY_W = 32
) (
   input clk,
   input rst,

   input running,
   input run,

   input [31:0] in0,  // Selector
   input [31:0] in1,
   input [31:0] in2,

   (* versat_latency = 1 *) output reg [31:0] out0
);

   always @(posedge clk, posedge rst) begin
      if (in0) begin
         out0 <= in1;
      end else begin
         out0 <= in2;
      end
   end

endmodule

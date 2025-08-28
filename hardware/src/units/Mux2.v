`timescale 1ns / 1ps

module Mux2 #(
   parameter DATA_W = 32
) (
   //control
   input clk,
   input rst,

   input running,

   //input / output data
   input [DATA_W-1:0] in0,
   input [DATA_W-1:0] in1,

   (* versat_latency = 1 *) output reg [DATA_W-1:0] out0,

   input sel
);

   always @(posedge clk, posedge rst) begin
      if (rst) begin
         out0 <= {DATA_W{1'b0}};
      end else if (running) begin
        if (sel) begin
           out0 <= in1;
        end else begin
           out0 <= in0;
        end
      end
   end

endmodule

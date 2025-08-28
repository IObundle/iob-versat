`timescale 1ns / 1ps

module PipelineRegister #(
   parameter DATA_W = 32
) (
   input running,

   input [DATA_W-1:0] in0,

   (* versat_latency = 1 *) output reg [DATA_W-1:0] out0,

   input clk,
   input rst
);

   always @(posedge clk, posedge rst) begin
      if (rst) begin
         out0 <= {DATA_W{1'b0}};
      end else if (running) begin
         out0 <= in0;
      end else begin
         out0 <= {DATA_W{1'b0}};
      end
   end

endmodule

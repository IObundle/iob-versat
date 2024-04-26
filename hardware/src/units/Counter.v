`timescale 1ns / 1ps

module Counter #(
   parameter PERIOD_W = 16
) (
   input clk,
   input rst,

   input running,
   input run,

   //configurations 
   input [          31:0] start,

   input [31:0] delay0,

   //outputs 
   (* versat_latency = 0 *) output reg [31:0] out0
);

   always @(posedge clk,posedge rst) begin
      if(rst) begin
         out0 <= 0;
      end else if(run) begin
         out0 <= start;
      end else if(running) begin
         out0 <= out0 + 1;
      end
   end

endmodule

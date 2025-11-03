`timescale 1ns / 1ps

module TimedMux #(
   parameter DATA_W = 32,
   parameter DELAY_W = 32
) (
   //control
   input clk,
   input rst,

   input running,
   input run,

   //input / output data
   input [DATA_W-1:0] in0,
   input [DATA_W-1:0] in1,

   (* versat_latency = 1 *) output reg [DATA_W-1:0] out0,

   input [DELAY_W-1:0] delay0
);

   reg [DELAY_W-1:0] delay;

   always @(posedge clk, posedge rst) begin
      if (rst) begin
         delay <= 0;
      end else if (run) begin
         delay <= delay0 + 1;
      end else if (running && |delay) begin
         delay <= delay - 1;
         out0  <= in0;
      end else if (running) begin
         out0 <= in1;
      end
   end

endmodule

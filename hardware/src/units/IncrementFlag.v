`timescale 1ns / 1ps

module IncrementFlag #(
   parameter DATA_W = 32
) (
   //control
   input clk,
   input rst,

   input running,
   input run,

   //input / output data
   input [DATA_W-1:0] in0,

   (* versat_latency = 1 *) output reg [DATA_W-1:0] out0,

   input [31:0] delay0
);

   reg [      31:0] delay;
   reg [DATA_W-1:0] stored;
   reg              storedFirst;

   always @(posedge clk, posedge rst) begin
      if (rst) begin
         delay       <= 0;
         storedFirst <= 1'b0;
      end else if (run) begin
         delay       <= delay0;
         storedFirst <= 1'b0;
      end else if (|delay) begin
         delay <= delay - 1;
         out0  <= 0;
      end else if (running) begin
         out0 <= 0;
         if (!storedFirst) begin
            storedFirst <= 1'b1;
            stored      <= in0;
            out0        <= ~0;
         end else if (in0 > stored) begin
            stored <= in0;
            out0   <= ~0;
         end
      end
   end

endmodule

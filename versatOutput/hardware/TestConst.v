`timescale 1ns / 1ps

module TestConst #(
   parameter DATA_W = 32,
   parameter DELAY_W = 7
) (
   //control
   input clk,
   input rst,

   input running,
   input run,

   //input / output data
   output reg [DATA_W-1:0] out0,

   input [DELAY_W-1:0] delay0,

   input [DATA_W-1:0] constant
);

   reg [DELAY_W-1:0] delay;

   always @(posedge clk, posedge rst) begin
      if (rst) begin
         out0 <= {DATA_W{1'b0}};
      end else if (run) begin
         if (delay0 == 0) begin
            out0 <= constant;
         end
         delay <= delay0;
      end else if (running & (|delay)) begin
         delay <= delay - 1;
         if (delay == 1) begin
             out0 <= constant;
         end
      end else begin
         out0 <= {DATA_W{1'b0}};
      end
   end

endmodule

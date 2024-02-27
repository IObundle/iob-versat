`timescale 1ns / 1ps

module Store #(
   parameter DELAY_W = 32,
   parameter ADDR_W  = 1,
   parameter DATA_W  = 32
) (
   //control
   input clk,
   input rst,

   input      running,
   input      run,
   output reg done,

   //input / output data
   input [DATA_W-1:0] in0,

   input [DELAY_W-1:0] delay0,  // delay

   output reg [DATA_W-1:0] currentValue  // state
);

   reg [DELAY_W-1:0] delay;

   always @(posedge clk, posedge rst) begin
      if (rst) begin
         currentValue <= 0;
         delay        <= 0;
         done         <= 1;
      end else if (run) begin
         done  <= 0;
         delay <= delay0;
      end else begin
         if (|delay) delay <= delay - 1;

         if (delay == 0) begin
            currentValue <= in0;
            done         <= 1;
         end
      end
   end

endmodule






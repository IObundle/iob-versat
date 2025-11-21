`timescale 1ns / 1ps

module DelayCalc #(
   parameter DELAY_W = 7
) (
   //control
   input clk,
   input rst,

   input      running,
   input      run,
   output reg done,

   output     lastCycle,

   input [DELAY_W-1:0] delay0
);
   reg [DELAY_W-1:0] delay;

   assign lastCycle = (delay == 0) && !done;

   always @(posedge clk, posedge rst) begin
      if (rst) begin
         delay  <= 0;
         done <= 1'b1;
      end else if(run) begin
         delay <= delay0;
         done <= 1'b0;
      end else if(running) begin
         if (delay == 0) begin
            done <= 1'b1;
         end else begin
            delay <= delay - 1;
         end
      end
   end

endmodule

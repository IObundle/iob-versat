`timescale 1ns / 1ps

module DoneCycle (
   //control
   input clk,
   input rst,

   input  running,
   input  run,
   output done,

   input [31:0] amount
);

   reg [31:0] counter;

   assign done = (counter == 0);

   always @(posedge clk, posedge rst) begin
      if (rst) begin
         counter <= 0;
      end else if (run) begin
         counter <= amount;
      end else if (running & (|counter)) begin
         counter <= counter - 1;
      end else begin
         counter <= counter;
      end
   end

endmodule

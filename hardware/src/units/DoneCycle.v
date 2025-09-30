`timescale 1ns / 1ps

module DoneCycle #(
    parameter DATA_W = 32

) (
   //control
   input clk,
   input rst,

   input  running,
   input  run,
   output done,

   input [DATA_W-1:0] amount
);

   reg [DATA_W-1:0] counter;

   assign done = (counter == {DATA_W{1'b0}});

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

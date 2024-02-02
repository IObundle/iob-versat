`timescale 1ns / 1ps

module FixedBuffer #(
   parameter ADDR_W = 6,
   parameter DATA_W = 32,
   parameter AMOUNT = 0
) (
   //control
   input clk,
   input rst,

   input running,
   input run,

   //input / output data
   input [DATA_W-1:0] in0,

   (* versat_latency = 1 *) output reg [DATA_W-1:0] out0
);

   generate
      if (AMOUNT == 0) begin
         always @(posedge clk, posedge rst) begin
            if (rst) out0 <= 0;
            else out0 <= in0;
         end
      end else begin
         integer        i;
         reg     [31:0] bufferData[AMOUNT-1:0];
         always @(posedge clk, posedge rst) begin
            if (rst) begin
               out0 <= 0;
               for (i = 0; i < AMOUNT; i = i + 1) begin
                  bufferData[i] <= 0;
               end
            end else begin
               out0 <= bufferData[0];
               for (i = 0; i < AMOUNT - 1; i = i + 1) begin
                  bufferData[i] <= bufferData[i+1];
               end
               bufferData[AMOUNT-1] <= in0;
            end
         end
      end
   endgenerate

endmodule

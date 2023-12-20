`timescale 1ns / 1ps

module MemoryScanner #(
   parameter DATA_W = 32,
   parameter ADDR_W = 10
) (
   output reg [ADDR_W-1:0] addr_o,
   input      [DATA_W-1:0] dataIn_i,
   output                  enable_o,

   input               nextValue_i,
   output [DATA_W-1:0] currentValue_o,

   input reset_i,

   input clk_i,
   input rst_i
);

   localparam INCREMENT = DATA_W / 8;

   reg hasValueStored;

   assign currentValue_o = dataIn_i;

   assign enable_o       = nextValue_i || !hasValueStored;

   always @(posedge clk_i, posedge rst_i) begin
      if (rst_i) begin
         addr_o         <= 0;
         hasValueStored <= 1'b0;
      end else if (reset_i) begin
         addr_o         <= 0;
         hasValueStored <= 1'b0;
      end else begin
         if (enable_o) begin
            hasValueStored <= 1'b1;
         end
         if (nextValue_i || enable_o) begin
            addr_o <= addr_o + INCREMENT;
         end
      end
   end

endmodule  // MemoryScanner

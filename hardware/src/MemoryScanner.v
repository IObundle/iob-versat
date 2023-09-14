`timescale 1ns / 1ps

module MemoryScanner #(
      parameter DATA_W = 32,
      parameter ADDR_W = 10
   )(
   output reg [ADDR_W-1:0] addr,
   input  [DATA_W-1:0]     dataIn,
   output                  enable,
   
   input               nextValue,
   output [DATA_W-1:0] currentValue,

   input reset,

   input clk,
   input rst
);

localparam INCREMENT = DATA_W/8;

reg hasValueStored;

assign currentValue = dataIn;

assign enable = nextValue || !hasValueStored;

always @(posedge clk,posedge rst) begin
   if(rst) begin
      addr <= 0;
      hasValueStored <= 1'b0;
   end else if(reset) begin
      addr <= 0;
      hasValueStored <= 1'b0;
   end else begin
      if(enable) begin
         hasValueStored <= 1'b1;
      end
      if(nextValue || enable) begin
         addr <= addr + INCREMENT;
      end
   end
end

endmodule

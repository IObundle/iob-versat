`timescale 1ns / 1ps
`include "xversat.vh"

module Delay #(
         parameter MAX_DELAY = 128,
         parameter DATA_W = 32
      )
    (
    //control
    input                         clk,
    input                         rst,
    
    input                         run,

    //input / output data
    input [DATA_W-1:0]            in0,

    (* latency=1 *) output reg [DATA_W-1:0]       out0,
    
    input [6:0]                  amount
    );

reg [DATA_W-1:0] mem[MAX_DELAY - 1:0];
reg [6:0] index;

always @(posedge clk,posedge rst)
begin
   if(rst) begin
      out0 <= 0;
      index <= 0;
   end else begin
      if(amount == 0) begin
         out0 <= in0;
      end else begin
         out0 <= mem[index];
         mem[index] <= in0;

         index <= index + 7'h1;
         if(index + 7'h1 >= amount)
            index <= 0;
      end
   end
end

endmodule
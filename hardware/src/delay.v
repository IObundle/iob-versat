`timescale 1ns / 1ps
`include "xversat.vh"

module delay #(
         parameter MAX_DELAY = 32,
         parameter DATA_W = 32
      )
    (
    //control
    input                         clk,
    input                         rst,
    
    input                         run,
    output                        done,

    //input / output data
    input [DATA_W-1:0]            in0,

    output reg [DATA_W-1:0]       out0,
    
    input [31:0]                  amount
    );

assign done = 1'b1;

reg [DATA_W-1:0] mem[MAX_DELAY - 1:0];
reg [$clog2(MAX_DELAY)-1:0] index;

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

         index <= index + 1;
         if(index + 1 >= amount)
            index <= 0;
      end
   end
end

endmodule
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
integer i;

always @(posedge clk,posedge rst)
begin
   if(rst)
      out0 <= 0;
   else begin
      out0 <= mem[0];

      for(i = 0; i < (MAX_DELAY - 1); i = i + 1) begin
         mem[i] <= mem[i + 1];
      end

      if(amount == 0)
         out0 <= in0;
      else
         mem[amount - 1] <= in0;
   end
end

endmodule
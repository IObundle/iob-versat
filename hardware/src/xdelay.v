`timescale 1ns / 1ps
`include "xversat.vh"

module xdelay #(
         parameter MAX_DELAY = 4,
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
    
    input [$clog2(MAX_DELAY)-1:0]   extra_delay
    );

assign done = 1'b1;

reg [DATA_W-1:0] mem[MAX_DELAY - 1:0];
integer i;

always @(posedge clk,posedge rst)
begin
   if(rst)
      out0 <= 0;
   else begin
      out0 <= mem[0]

      for(i = 0; i < (MAX_DELAY - 1); i = i + 1)
         mem[i] <= mem[i + 1];

      if(extra_delay == 0)
         out0 <= in0;
      else
         mem[extra_delay - 1] <= in0;
   end
end

endmodule
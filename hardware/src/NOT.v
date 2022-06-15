`timescale 1ns / 1ps
`include "xversat.vh"

module NOT #(
         parameter DATA_W = 32
      )
      (
         input run,
         output done,

         input [DATA_W-1:0] in0,

         output [DATA_W-1:0] out0,
      
         input clk,
         input rst
      ); 

assign out0 = !in0;
assign done = 1'b1;

endmodule

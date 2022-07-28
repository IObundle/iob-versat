`timescale 1ns / 1ps
`include "xversat.vh"

module CircuitInput #(
         parameter DATA_W = 32
      )
      (
         input run,
         output done,

         input [DATA_W-1:0] in0,

         output [DATA_W-1:0] out0,
         output [DATA_W-1:0] out1,
         output [DATA_W-1:0] out2,
         output [DATA_W-1:0] out3,
         output [DATA_W-1:0] out4,
         output [DATA_W-1:0] out5,
         output [DATA_W-1:0] out6,
         output [DATA_W-1:0] out7,
         output [DATA_W-1:0] out8,
         output [DATA_W-1:0] out9,
         output [DATA_W-1:0] out10,
         output [DATA_W-1:0] out11,
         output [DATA_W-1:0] out12,
         output [DATA_W-1:0] out13,
         output [DATA_W-1:0] out14,
         output [DATA_W-1:0] out15,
      
         input clk,
         input rst
      ); 

assign out0 = in0;
assign out1 = in0;
assign out2 = in0;
assign out3 = in0;
assign out4 = in0;
assign out5 = in0;
assign out6 = in0;
assign out7 = in0;
assign out8 = in0;
assign out9 = in0;
assign out10 = in0;
assign out11 = in0;
assign out12 = in0;
assign out13 = in0;
assign out14 = in0;
assign out15 = in0;
assign done = 1'b1;

endmodule

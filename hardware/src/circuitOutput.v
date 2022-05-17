`timescale 1ns / 1ps
`include "xversat.vh"

module circuitOutput #(
         parameter DATA_W = 32
      )
      (
         input run,
         output done,

         input [DATA_W-1:0] in0,
         input [DATA_W-1:0] in1,
         input [DATA_W-1:0] in2,
         input [DATA_W-1:0] in3,
         input [DATA_W-1:0] in4,
         input [DATA_W-1:0] in5,
         input [DATA_W-1:0] in6,
         input [DATA_W-1:0] in7,
         input [DATA_W-1:0] in8,
         input [DATA_W-1:0] in9,
         input [DATA_W-1:0] in10,
         input [DATA_W-1:0] in11,
         input [DATA_W-1:0] in12,
         input [DATA_W-1:0] in13,
         input [DATA_W-1:0] in14,
         input [DATA_W-1:0] in15,

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
assign out1 = in1;
assign out2 = in2;
assign out3 = in3;
assign out4 = in4;
assign out5 = in5;
assign out6 = in6;
assign out7 = in7;
assign out8 = in8;
assign out9 = in9;
assign out10 = in10;
assign out11 = in11;
assign out12 = in12;
assign out13 = in13;
assign out14 = in14;
assign out15 = in15;
assign done = 1'b1;

endmodule

`timescale 1ns / 1ps

module Const #(
   parameter DATA_W = 32
) (

   output [DATA_W-1:0] out0,

   input [DATA_W-1:0] constant  // config
);

   assign out0 = constant;

endmodule

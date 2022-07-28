`timescale 1ns / 1ps
`include "xversat.vh"

module Const #(
         parameter DATA_W = 32
              )
    (
    //control
    input               clk,
    input               rst,
    
    input               run,
    output              done,

    //input / output data
    output [DATA_W-1:0] out0,

    input [DATA_W-1:0]  constant
    );

assign out0 = constant;
assign done = 1'b1;

endmodule
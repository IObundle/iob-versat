`timescale 1ns / 1ps
`include "xversat.vh"

module Const #(
         parameter DATA_W = 32
              )
    (
    //control
    input               clk,
    input               rst,
    
    input               running,
    input               run,

    output [DATA_W-1:0] out0,

    input [DATA_W-1:0]  constant // config
    );

assign out0 = constant;

endmodule
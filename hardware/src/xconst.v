`timescale 1ns / 1ps
`include "xversat.vh"

module xconst #(
         parameter DATA_W = 32
              )
    (
    //control
    input                         clk,
    input                         rst,
    
    input                         run,
    output                        done,

    //input / output data
    output     [DATA_W-1:0]       out0,

    input [DATA_W-1:0]            constant_0
    );

assign out0 = constant_0;
assign done = 1'b1;

endmodule
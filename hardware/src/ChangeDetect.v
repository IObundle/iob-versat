`timescale 1ns / 1ps
`include "xversat.vh"

module ChangeDetect #(
         parameter DATA_W = 32
              )
    (
    //control
    input               clk,
    input               rst,
    
    input               running,
    input               run,

    //input / output data
    input [DATA_W-1:0] in0,
    (* versat_latency = 1 *) input [DATA_W-1:0] in1,

    output [DATA_W-1:0] out0
    );

assign out0 = ((in0 != in1) ? ~0 : 0);

endmodule
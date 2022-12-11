`timescale 1ns / 1ps
`include "xversat.vh"

module Int2Float #(
         parameter DATA_W = 32
              )
    (
    //control
    input                         clk,
    input                         rst,
    
    input                         run,
    
    //input / output data
    input [DATA_W-1:0]            in0,
    
    (* versat_latency = 5 *) output [DATA_W-1:0]       out0
    );

fp_int2float conv(
   .clk(clk),
   .rst(rst),

   .start(1'bx),
   .done(),

   .op(in0),
   .res(out0)
    );

endmodule 

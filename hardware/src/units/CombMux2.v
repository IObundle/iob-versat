`timescale 1ns / 1ps

module CombMux2 #(
   parameter DATA_W = 32
) (
   //control
   input clk,
   input rst,

   input running,
   input run,

   //input / output data
   input [DATA_W-1:0] in0,
   input [DATA_W-1:0] in1,

   (* versat_latency = 0 *) output [DATA_W-1:0] out0,

   input sel
);

   assign out0 = (sel ? in1 : in0);

endmodule

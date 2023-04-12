`timescale 1ns / 1ps
`include "xversat.vh"

module FloatMin #(
         parameter DATA_W = 32
              )
    (
    //control
    input                         clk,
    input                         rst,
    
    input                         run,
    
    //input / output data
    input [DATA_W-1:0]            in0,
    input [DATA_W-1:0]            in1,
    
    (* versat_latency = 1 *) output [DATA_W-1:0]       out0
    );

fp_minmax min(
     .start(1'bx),
     .done(),

     .op_a(in0),
     .op_b(in1),

     .res(out0),

     .max_n_min(1'b0),

     .clk(clk),
     .rst(rst)
     );

endmodule

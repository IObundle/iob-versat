`timescale 1ns / 1ps

module FloatMul #(
         parameter DELAY_W = 32,
         parameter DATA_W = 32
              )
    (
    //control
    input                         clk,
    input                         rst,
    
    input                         running,
    input                         run,

    //input / output data
    input [DATA_W-1:0]            in0,
    input [DATA_W-1:0]            in1,

    (* versat_latency = 4 *) output [DATA_W-1:0]       out0
    );

fp_mul Mul(
    .clk(clk),
    .rst(rst),

    .start(1'b1),
    .done(),

    .op_a(in0),
    .op_b(in1),

    .overflow(),
    .underflow(),
    .exception(),

    .res(out0)
     );

endmodule

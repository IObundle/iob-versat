`timescale 1ns / 1ps

module Float2UInt #(
   parameter DATA_W = 32
) (
   //control
   input clk,
   input rst,

   input running,
   input run,

   //input / output data
   input [DATA_W-1:0] in0,

   (* versat_latency = 1 *) output [DATA_W-1:0] out0
);

   iob_fp_float2uint conv (
      .clk_i(clk),
      .rst_i(rst),

      .start_i(1'bx),
      .done_o (),

      .op_i (in0),
      .res_o(out0)
   );

endmodule

`timescale 1ns / 1ps

module ParamConst #(
   parameter /* versat:unique */ DATA_W = 32,
   parameter [DATA_W-1:0] VALUE = 0
) (
   output [DATA_W-1:0] out0
);

   assign out0 = VALUE;

endmodule

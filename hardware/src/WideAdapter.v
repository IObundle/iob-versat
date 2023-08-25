`timescale 1ns / 1ps

// Ex: 64 bits input and 32 bits output, sel == 0 selects lower 32 bits. sel == 1 selects upper 32 bits

module WideAdapter #(
   parameter INPUT_W = 64,
   parameter SIZE_W = 32, // Size of data (data can be smaller than output, in which case the output is zero extended)
   parameter OUTPUT_W = 32 // Size of output
   )
   (
      input [DIFF_W-1:0] sel,

      input [INPUT_W-1:0] in,
      output [OUTPUT_W-1:0] out
   );

localparam DIFF_W = $clog2(INPUT_W / SIZE_W);

ZeroExtend #(.INPUT_W(SIZE_W),.OUTPUT_W(OUTPUT_W)) extend
   (
      .in(in[sel * SIZE_W +: SIZE_W]),
      .out(out)
   );

endmodule
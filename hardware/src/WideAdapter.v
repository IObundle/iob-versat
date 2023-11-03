`timescale 1ns / 1ps
// Comment so that verible-format will not put timescale and defaultt_nettype into same line
`default_nettype none

// Ex: 64 bits input and 32 bits output, sel == 0 selects lower 32 bits. sel == 1 selects upper 32 bits

module WideAdapter 
  #(
    parameter INPUT_W = 64,
    parameter SIZE_W = 32, // Size of data (data can be smaller than output, in which case the output is zero extended)
    parameter OUTPUT_W = 32 // Size of output
    )
   (
    input [DIFF_W-1:0]    sel_i,

    input [INPUT_W-1:0]   in_i,
    output [OUTPUT_W-1:0] out_o
    );

localparam DIFF_W = $clog2(IN_IPUT_W / SIZE_W);

ZeroExtend #(.IN_IPUT_W(SIZE_W),.OUT_OPUT_W(OUT_OPUT_W)) extend
   (
      .in_i(in_i[sel_i * SIZE_W +: SIZE_W]),
      .out_o(out_o)
   );

endmodule // WideAdapter

`default_nettype wire

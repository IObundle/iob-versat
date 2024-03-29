`timescale 1ns / 1ps

// Assumes input is smaller than output, otherwise will produce an error
module ZeroExtend #(
   parameter OUTPUT_W = 0,
   parameter INPUT_W  = 0
) (
   input      [ INPUT_W-1:0] in_i,
   output reg [OUTPUT_W-1:0] out_o
);

   always @* begin
      out_o              = 0;
      out_o[INPUT_W-1:0] = in_i;
   end

endmodule  // ZeroExtend

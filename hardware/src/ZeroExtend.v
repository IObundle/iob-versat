`timescale 1ns / 1ps

// Assumes input is smaller than output, otherwise will produce an error

module ZeroExtend #(
   parameter OUTPUT_W = 0,
   parameter INPUT_W = 0
   )
   (
      input [INPUT_W-1:0] in,
      output reg [OUTPUT_W-1:0] out
   );

always @*
begin
   out = 0;
   out[INPUT_W-1:0] = in;
end

endmodule
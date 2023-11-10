`timescale 1ns / 1ps
// Comment so that verible-format will not put timescale and defaultt_nettype into same line
`default_nettype none

module FloatToLargeInteger
    (
    //input / output data
    input [31:0]             in_i,

    output reg [278:0]       out_o // Much higher order
    );

   localparam EXP_W = 8;
   localparam BIAS = 2**(EXP_W-1)-1;

   wire [7:0]  exponent = in_i[30:23];
   
   wire [23:0] mantissa = {(exponent == 8'h00 ? 1'b0 : 1'b1),in_i[22:0]};
   wire        sign     = in_i[31];

   reg [278:0] temp;

   always @* begin
     temp = 0;
     temp[23:0] = mantissa;
     out_o = sign ? -(temp << exponent) : (temp << exponent);
   end 

endmodule // FloatToLargeInteger

`default_nettype wire

`timescale 1ns / 1ps

module FloatToLargeInteger
    (
    //input / output data
    input [31:0]             in,

    output reg [278:0]       out // Much higher order
    );

   localparam EXP_W = 8;
   localparam BIAS = 2**(EXP_W-1)-1;

   wire [7:0]  exponent = in[30:23];
   
   wire [23:0] mantissa = {(exponent == 8'h00 ? 1'b0 : 1'b1),in[22:0]};
   wire        sign     = in[31];

   reg [278:0] temp;

   always @* begin
     temp = 0;
     temp[23:0] = mantissa;
     out = sign ? -(temp << exponent) : (temp << exponent);
   end 

endmodule

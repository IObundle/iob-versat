`timescale 1ns / 1ps

module CombMux8 #(
   parameter DATA_W = 32
) (
   //input / output data
   input [DATA_W-1:0] in0,
   input [DATA_W-1:0] in1,
   input [DATA_W-1:0] in2,
   input [DATA_W-1:0] in3,
   input [DATA_W-1:0] in4,
   input [DATA_W-1:0] in5,
   input [DATA_W-1:0] in6,
   input [DATA_W-1:0] in7,

   (* versat_latency = 0 *) output reg [DATA_W-1:0] out0,

   input [2:0] sel
);

   always @* begin
      case (sel)
         3'b000: out0 = in0;
         3'b001: out0 = in1;
         3'b010: out0 = in2;
         3'b011: out0 = in3;
         3'b100: out0 = in4;
         3'b101: out0 = in5;
         3'b110: out0 = in6;
         3'b111: out0 = in7;
      endcase
   end

endmodule

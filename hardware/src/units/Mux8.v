`timescale 1ns / 1ps

module Mux8 #(
   parameter DATA_W = 32
) (
   //control
   input clk,
   input rst,

   input running,
   input run,

   //input / output data
   input [DATA_W-1:0] in0,
   input [DATA_W-1:0] in1,
   input [DATA_W-1:0] in2,
   input [DATA_W-1:0] in3,
   input [DATA_W-1:0] in4,
   input [DATA_W-1:0] in5,
   input [DATA_W-1:0] in6,
   input [DATA_W-1:0] in7,

   (* versat_latency = 1 *) output reg [DATA_W-1:0] out0,

   input [2:0] sel
);

   reg [DATA_W-1:0] out;
   always @* begin
      case (sel)
         3'b000: out = in0;
         3'b001: out = in1;
         3'b010: out = in2;
         3'b011: out = in3;
         3'b100: out = in4;
         3'b101: out = in5;
         3'b110: out = in6;
         3'b111: out = in7;
      endcase
   end

   always @(posedge clk,posedge rst) begin
      if(rst) begin
         out0 <= 0;
      end else begin
         out0 <= out;         
      end

   end

endmodule

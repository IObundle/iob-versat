`timescale 1ns / 1ps

module MyFloatAccum #(
         parameter DATA_W = 32
              )
    (
    //control
    input                         clk,
    input                         rst,
    
    input run,
    input running,

    //input / output data
    input [DATA_W-1:0]            in0,
    input [DATA_W-1:0]            in1, // Signal

    (* versat_latency = 1 *) output reg [31:0] out0 // Much higher order
    );

   localparam MAN_W = 23;
   localparam EXP_W = 8;
   localparam BIAS = 2**(EXP_W-1)-1;

   reg [278:0] accum;
   wire [278:0] in0_decoded;

   FloatToLargeInteger conv(.in(in0),.out(in0_decoded));

   always @(posedge clk,posedge rst) begin
     if(rst) begin
          accum <= 0;
     end else begin
          if(|in1) begin
               accum <= in0_decoded;
          end else begin
               accum <= in0_decoded + accum;
          end
     end
   end
     
   // Repack
     
   wire [8:0] lzc;
   clz #(.DATA_W(279)) count(.data_in(accum),.data_out(lzc));

   reg [8:0] augExponent;   
   reg [7:0] exponent;

   always @* begin
     augExponent = 0;
     augExponent = lzc - 8'h23;
     exponent = augExponent[7:0];
   end

   reg [22:0] mantissa;
   always @* begin
     mantissa = accum[augExponent +: 23];
   end

   reg sign;
   always @* begin
     sign = accum[278];
   end

   assign out0 = {sign,exponent,mantissa};

endmodule

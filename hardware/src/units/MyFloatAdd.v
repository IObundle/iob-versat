`timescale 1ns / 1ps

module MyFloatAdd #(
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
    input [DATA_W-1:0]            in1,

    (* versat_latency = 1 *) output reg [31:0] out0 // Much higher order
    );

   localparam MAN_W = 23;
   localparam EXP_W = 8;
   localparam BIAS = 2**(EXP_W-1)-1;

   wire [278:0] in0_decoded;
   wire [278:0] in1_decoded;

   reg [278:0] res;

   FloatToLargeInteger conv1(.in(in0),.out(in0_decoded));
   FloatToLargeInteger conv2(.in(in1),.out(in1_decoded));

   always @(posedge clk,posedge rst) begin
     if(rst) begin
          res <= 0;
     end else begin
          res <= in0_decoded + in1_decoded;
     end
   end
     
   // Repack
   wire [8:0] lzc;
   clz #(.DATA_W(279)) count(.data_in(res),.data_out(lzc));

   wire [278:0] negatedRes = -res;

   wire [8:0] nlzc; // Negative
   clz #(.DATA_W(279)) countNeg(.data_in(negatedRes),.data_out(nlzc));

   wire sign = res[278];

   reg [7:0] exponent;

   always @* begin
     if(lzc == 9'd279) begin
          exponent = 0;
     end else begin
          if(sign) begin
               exponent = 8'hff - nlzc[7:0];
          end else begin
               exponent = 8'hff - lzc[7:0];
          end
     end
   end

   reg [22:0] mantissa;
   always @* begin
     if(sign) begin
          mantissa = negatedRes[{1'b0,exponent} +: 23];
     end else begin
          mantissa = res[{1'b0,exponent} +: 23];
     end
   end

   assign out0 = {sign,exponent,mantissa};

endmodule

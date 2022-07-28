`timescale 1ns / 1ps
`include "xversat.vh"

module PipelineRegister #(
         parameter DATA_W = 32
      )
      (
         input run,
         output done,

         input [DATA_W-1:0] in0,

         (* latency = 1 *) output reg [DATA_W-1:0] out0,
      
         input clk,
         input rst
      ); 
      
assign done = 1'b1;

always @(posedge clk,posedge rst)
begin
   if(rst) begin
      out0 <= 0;
   end else begin
      out0 <= in0;
   end
end

endmodule

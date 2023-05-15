`timescale 1ns / 1ps
`include "xversat.vh"

module PipelineRegister #(
         parameter DATA_W = 32
      )
      (
         input run,
         input running,
                
         input [DATA_W-1:0] in0,

         (* versat_latency = 1 *) output reg [DATA_W-1:0] out0,
      
         input clk,
         input rst
      ); 

always @(posedge clk)
begin
   out0 <= in0;
end

endmodule

`timescale 1ns / 1ps

module Mux2 #(
         parameter DATA_W = 32
      )
    (
    //control
    input                         clk,
    input                         rst,
    
    input                         running,
    input                         run,

    //input / output data
    input [DATA_W-1:0]            in0,
    input [DATA_W-1:0]            in1,

    (* versat_latency = 1 *) output reg [DATA_W-1:0]       out0,
    
    input                         sel
    );

always @(posedge clk,posedge rst)
begin
   if(rst)
      out0 <= 0;
   else
      if(sel)
         out0 <= in1;
      else
         out0 <= in0;
end

endmodule
`timescale 1ns / 1ps
`include "xversat.vh"

module TestConst #(
         parameter DATA_W = 32
     )
    (
    //control
    input                     clk,
    input                     rst,
    
    input                     run,

    //input / output data
    output reg [DATA_W-1:0]   out0,

    input [DATA_W-1:0]        constant
    );

always @(posedge clk,posedge rst)
begin
     if(rst) begin
          out0 <= 0;
     end else if(run) begin
          out0 <= constant;
     end else begin
          out0 <= 0;
     end
end

endmodule
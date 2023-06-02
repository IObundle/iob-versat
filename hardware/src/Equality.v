`timescale 1ns / 1ps

module Equality #(
         parameter DATA_W = 32
              )
    (
    //control
    input               clk,
    input               rst,
    
    input               running,
    input               run,

    //input / output data
    input [DATA_W-1:0] in0,
    input [DATA_W-1:0] in1,

    (* versat_latency = 1 *) output reg [DATA_W-1:0] out0
    );

always @(posedge clk,posedge rst)
begin
     if(rst) begin
          out0 <= 0;
     end else begin
          out0 <= (in0 == in1) ? ~0 : 0; 
     end
end

endmodule
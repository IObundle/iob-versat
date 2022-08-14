`timescale 1ns / 1ps
`include "xversat.vh"

module SwapEndian #(
         parameter DATA_W = 32
              )
    (
    //control
    input               clk,
    input               rst,
    
    input               run,

    (* latency = 0 *) input [DATA_W-1:0]  in0,

    //input / output data
    (* latency = 0 *) output reg [DATA_W-1:0] out0,

    input               enabled
    );

always @*
begin
     if(enabled) begin
          out0[0+:8] = in0[24+:8];
          out0[8+:8] = in0[16+:8];
          out0[16+:8] = in0[8+:8];
          out0[24+:8] = in0[0+:8];
     end else begin
          out0 = in0;
     end
end

endmodule
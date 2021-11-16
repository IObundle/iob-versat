`timescale 1ns / 1ps
`include "xversat.vh"

module xreg #(
         parameter DELAY_W = 10,
         parameter DATA_W = 32
              )
    (
    //control
    input                         clk,
    input                         rst,
    
    input                         run,
    output                        done,

    //input / output data
    input [DATA_W-1:0]            in0,
    output reg [DATA_W-1:0]       out0,

    input [DATA_W-1:0]            initialValue,
    input [DELAY_W-1:0]           writeDelay,

    output [DATA_W-1:0]           currentValue
    );

reg [DELAY_W-1:0] delay;
reg alreadyInit;

assign currentValue = out0;
assign done = (delay == 0);

always @(posedge clk,posedge rst)
begin
   if(rst) begin
      out0 <= 0;
      delay <= 0;
      alreadyInit <= 0;
   end else if(run) begin
      if(!alreadyInit) begin
         out0 <= initialValue;
         alreadyInit <= 1;
      end
      delay <= writeDelay;
   end else begin
      if(|delay) begin
         delay <= delay - 1;
      end

      if(delay == 1) begin
         out0 <= in0;
      end
   end
end

endmodule
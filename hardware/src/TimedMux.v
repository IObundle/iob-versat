`timescale 1ns / 1ps
`include "xversat.vh"

module TimedMux #(
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

    (* versat_latency = 0 *) output reg [DATA_W-1:0]       out0,
    
    input [31:0]                  delay0
    );

reg [31:0] delay;
reg done;

always @*
begin
   out0 = 0;

   if(running) begin
      out0 = (done ? in1 : in0);
   end
end

always @(posedge clk,posedge rst)
begin
   if(rst) begin
      delay <= 0;
      done <= 0;
   end else if(run) begin
      delay <= delay0;
      done <= 0;
   end else if(|delay) begin
      delay <= delay - 1;
      done <= 0;
   end else begin
      done <= 1;
   end
end

endmodule
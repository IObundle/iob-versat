`timescale 1ns / 1ps
`include "xversat.vh"

(* source *) module Reg #(
         parameter DELAY_W = 32,
         parameter ADDR_W = 1,
         parameter DATA_W = 32
              )
    (
    //control
    input                         clk,
    input                         rst,
    
    input                         run,
    output reg                    done,

    // native interface 
    input [DATA_W/8-1:0]          wstrb,
    input                         addr, // shouldn't even have it at all, but needed so that it follows the interface expected by verilator
    input [DATA_W-1:0]            wdata,
    input                         valid,
    output reg                    ready,
    output [DATA_W-1:0]           rdata,

    //input / output data
    input [DATA_W-1:0]            in0,
    output reg [DATA_W-1:0]       out0,

    input [DELAY_W-1:0]           delay0,

    output [DATA_W-1:0]           currentValue
    );

reg [DELAY_W-1:0] delay;

assign rdata = (ready ? out0 : 0);
assign currentValue = out0;

always @(posedge clk,posedge rst)
begin
   if(rst) begin
      out0 <= 0;
      delay <= 0;
      ready <= 0;
      done <= 1;
   end else begin
      // Native interface
      ready <= valid;

      if(valid & |wstrb) begin
         out0 <= wdata;
      end

      if(run) begin
         done <= 0;
         delay <= delay0;
      end 

      if(!done) begin
         if(delay == 0) begin
            out0 <= in0;
            done <= 1;
         end else begin
            delay <= delay - 1;
         end
      end
   end
end

endmodule
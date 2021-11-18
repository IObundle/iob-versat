`timescale 1ns / 1ps
`include "xversat.vh"

module xreg #(
         parameter DELAY_W = 10,
         parameter ADDR_W = 1,
         parameter DATA_W = 32
              )
    (
    //control
    input                         clk,
    input                         rst,
    
    input                         run,
    output                        done,

    // native interface 
    input [DATA_W/8-1:0]          wstrb,
    input [ADDR_W-1:0]            addr,
    input [DATA_W-1:0]            wdata,
    input                         valid,
    output reg                    ready,
    output [DATA_W-1:0]           rdata,

    //input / output data
    input [DATA_W-1:0]            in0,
    output reg [DATA_W-1:0]       out0,

    input [DELAY_W-1:0]           writeDelay,

    output [DATA_W-1:0]           currentValue
    );

reg [DELAY_W-1:0] delay;

assign rdata = (ready ? out0 : 0);
assign currentValue = out0;
assign done = (delay == 0);

always @(posedge clk,posedge rst)
begin
   if(rst) begin
      out0 <= 0;
      delay <= 0;
      ready <= 0;
   end else if(run) begin
      ready <= 0;
      delay <= writeDelay;
   end else if(valid & |wstrb) begin
      out0 <= wdata;
      ready <= 1'b1;
   end else begin
      ready <= 0;

      if(|delay) begin
         delay <= delay - 1;
      end

      if(delay == 1) begin
         out0 <= in0;
      end
   end
end

endmodule
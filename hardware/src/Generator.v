`timescale 1ns / 1ps
`include "xversat.vh"

module Generator #(
   parameter           PERIOD_W = `PERIOD_W
   ) (
   input                   clk,
   input                   rst,

   input                   running,
   input                   run,

   //configurations 
   input [31:0]            iterations,
   input [PERIOD_W - 1:0]  period,
   input [PERIOD_W - 1:0]  duty,
   input [31:0]            start,
   input [31:0]            shift,
   input [31:0]            incr,

   input [31:0]            delay0,

   //outputs 
   (* versat_latency = 0 *) output [31:0] out0 // Latency zero because Address Gen is one cycle ahead in providing the address
   );

MyAddressGen #(.ADDR_W(32)) addrGen(
   .clk(clk),
   .rst(rst),

   .run(run),

   //configurations 
   .iterations(iterations),
   .period(period),
   .duty(duty),
   .delay(delay0),
   .start(start),
   .shift(shift),
   .incr(incr),

   //outputs 
   .valid(),
   .ready(1'b1),
   .addr(out0),

   .done()
   );

endmodule
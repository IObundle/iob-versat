`timescale 1ns / 1ps

(* source *) module Reg #(
   parameter DELAY_W = 7,
   parameter DATA_W  = 32
) (
   //control
   input clk,
   input rst,

   input  running,
   input  run,
   output done,

   // native interface 
   input      [         1:0] addr,
   input      [DATA_W/8-1:0] wstrb,
   input      [  DATA_W-1:0] wdata,
   input                     valid,
   output reg                rvalid,
   output     [  DATA_W-1:0] rdata,

   //input / output data
   input [DATA_W-1:0] in0,
   output reg [DATA_W-1:0]       out0, /* technically should have versat_latency 1 but since it's not a computing unit, output is only used to start a datapath, we save 1 cycle */

   input [DELAY_W-1:0] delay0,
   input disabled,

   output [DATA_W-1:0] currentValue
);

   wire lastCycle;

   DelayCalc #(.DELAY_W(DELAY_W)) delay_inst (
   .clk(clk),
   .rst(rst),

   .running(running),
   .run(run),
   .done(done),

   .lastCycle(lastCycle),

   .delay0(delay0)
   );

   assign rdata        = (rvalid ? out0 : 0);
   assign currentValue = out0;

   always @(posedge clk, posedge rst) begin
      if (rst) begin
         out0   <= {DATA_W{1'b0}};
         rvalid <= 1'b0;
      end else begin
         // Native interface
         rvalid <= 1'b0;

         if (valid & (|wstrb)) begin
            out0 <= wdata;
         end
         if (lastCycle && !disabled) begin
            out0 <= in0;
         end

         if (valid & (wstrb == 0)) begin
            rvalid <= 1'b1;
         end

      end
   end

endmodule

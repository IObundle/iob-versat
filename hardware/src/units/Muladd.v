`timescale 1ns / 1ps

`define MULADD_MACC 1'b0

/*

 Description: multiply (accumulate) functions

 */

module Muladd #(
   parameter DELAY_W = 7,
   parameter DATA_W = 32,
   parameter LOOP_W = 10
) (
   input      rst,
   input      clk,
   input      run,
   input      running,
   output reg done,

                            input  [DATA_W-1:0] in0,
                            input  [DATA_W-1:0] in1,
   (* versat_latency = 3 *) output [DATA_W-1:0] out0,

   // config interface
   input       opcode,
   input [LOOP_W-1:0] iter,
   input [LOOP_W-1:0] period,

   input [DELAY_W-1:0] delay0
);

   reg [DELAY_W-1:0] delay;
   reg [LOOP_W-1:0] currentIteration;
   reg [LOOP_W-1:0] currentPeriod;

   always @(posedge clk, posedge rst) begin
      if (rst) begin
         done             <= 1'b0;
         delay            <= {DELAY_W{1'b0}};
         currentIteration <= {LOOP_W{1'b0}};
         currentPeriod    <= {LOOP_W{1'b0}};
      end else if (run) begin
         delay <= delay0 + 2; // Add 2 because it takes 2 cycles for the accumulator to start working and this whole logic is just the control for the accumulator
         done <= 1'b0;
      end else if (running) begin
         if (|delay) begin
            delay <= delay - 1;
            if (iter == 0) begin
               done <= 1'b1;
            end
            currentIteration <= 0;
            currentPeriod    <= 0;
         end else if (!done) begin
            currentPeriod <= currentPeriod + 1;

            if ((currentPeriod + 1) >= period) begin
               currentPeriod    <= 0;

               currentIteration <= currentIteration + 1;
               if ((currentIteration + 1) >= iter) begin
                  done <= 1'b1;
               end
            end
         end
      end
   end

   // select multiplier statically
   reg signed [DATA_W-1:0] in0_reg, in1_reg, dsp_out0;
   reg signed [2*DATA_W-1:0] acc, result_mult_reg;
   //DSP48E template
   always @(posedge clk, posedge rst) begin
      if (rst) begin
         in0_reg  <= {DATA_W{1'b0}};
         in1_reg  <= {DATA_W{1'b0}};
         dsp_out0 <= {DATA_W{1'b0}};
         acc      <= {2*DATA_W{1'b0}};
      end else if (running) begin
         in0_reg         <= in0;
         in1_reg         <= in1;
         result_mult_reg <= in0_reg * in1_reg;

         if (currentPeriod == 0) begin
            acc <= result_mult_reg;
         end else begin
            if (opcode == `MULADD_MACC) acc <= acc + result_mult_reg;
            else acc <= acc - result_mult_reg;
         end

         dsp_out0 <= acc[DATA_W-1:0];
      end
   end

   assign out0 = dsp_out0;

endmodule

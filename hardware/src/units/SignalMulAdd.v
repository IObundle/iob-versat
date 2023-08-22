`timescale 1ns / 1ps

`define MULADD_MACC 1'b0

/*

 Description: multiply (accumulate) functions

 */

module SignalMulAdd #( 
		parameter		      DATA_W = 32
	) (
                input              rst,
                input              clk,
                input              run,
                input              running,
                output reg         done,

                input [DATA_W-1:0] in0,
                input [DATA_W-1:0] in1,
                (* versat_latency = 3 *) output [DATA_W-1:0] out0,

                // config interface
                input opcode,
                input [31:0] amount,

                input signal_loop,

                input [31:0]                 delay0
                );

reg [31:0] delay;
reg [31:0] toProcess;

always @(posedge clk,posedge rst)
begin
   if(rst) begin
      done <= 1'b1;
      delay <= 0;
      toProcess <= 0;
   end else if(run) begin
      delay <= delay0 + 1; // Add 2 because it takes 2 cycles for the accumulator to start working and this whole logic is just the control for the accumulator
      done <= 1'b0;
   end else if(|delay) begin
      delay <= delay - 1;
      if(toProcess == amount) begin
         done <= 1;
      end
      toProcess <= amount;
   end else if(delay == 0 && !done) begin
      if(toProcess != 0) begin
         toProcess <= toProcess - 1;
      end
      if(toProcess - 1 == 0) begin
         done <= 1;
      end
   end
end

// select multiplier statically
reg signed [DATA_W-1:0] in0_reg, in1_reg;
reg signed [2*DATA_W-1:0] acc, dsp_out0, result_mult_reg;
//DSP48E template
always @ (posedge clk,posedge rst) begin
   if(rst) begin
      in0_reg <= 0;
      in1_reg <= 0;
      acc <= 0;
      dsp_out0 <= 0;
   end else begin
      in0_reg <= in0;
      in1_reg <= in1;
      result_mult_reg <= in0_reg * in1_reg;

      if(signal_loop) begin
         acc <= 0;
      end else if(toProcess != 0) begin
         if(opcode == `MULADD_MACC)
            acc <= acc + result_mult_reg;
         else
            acc <= acc - result_mult_reg;
      end
     
      dsp_out0 <= acc;
   end
end

assign out0 = dsp_out0[31:0];

endmodule

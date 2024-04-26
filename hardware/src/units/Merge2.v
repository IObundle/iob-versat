`timescale 1ns / 1ps

module Merge2 #(
   parameter DELAY_W = 32,
   parameter DATA_W  = 32
) (
   //control
   input clk,
   input rst,

   input running,
   input run,

   //input / output data
                             input [DATA_W-1:0] in0,
   (* versat_latency = 1 *)  input [DATA_W-1:0] in1,
   (* versat_latency = 2 *)  input [DATA_W-1:0] in2,
   (* versat_latency = 3 *)  input [DATA_W-1:0] in3,
   (* versat_latency = 4 *)  input [DATA_W-1:0] in4,
   (* versat_latency = 5 *)  input [DATA_W-1:0] in5,
   (* versat_latency = 6 *)  input [DATA_W-1:0] in6,
   (* versat_latency = 7 *)  input [DATA_W-1:0] in7,
   (* versat_latency = 8 *)  input [DATA_W-1:0] in8,
   (* versat_latency = 9 *)  input [DATA_W-1:0] in9,
   (* versat_latency = 10 *) input [DATA_W-1:0] in10,
   (* versat_latency = 11 *) input [DATA_W-1:0] in11,
   (* versat_latency = 12 *) input [DATA_W-1:0] in12,
   (* versat_latency = 13 *) input [DATA_W-1:0] in13,
   (* versat_latency = 14 *) input [DATA_W-1:0] in14,
   (* versat_latency = 15 *) input [DATA_W-1:0] in15,
   (* versat_latency = 16 *) input [DATA_W-1:0] in16,
   (* versat_latency = 17 *) input [DATA_W-1:0] in17,
   (* versat_latency = 18 *) input [DATA_W-1:0] in18,
   (* versat_latency = 19 *) input [DATA_W-1:0] in19,
   (* versat_latency = 20 *) input [DATA_W-1:0] in20,
   (* versat_latency = 21 *) input [DATA_W-1:0] in21,
   (* versat_latency = 22 *) input [DATA_W-1:0] in22,
   (* versat_latency = 23 *) input [DATA_W-1:0] in23,
   (* versat_latency = 24 *) input [DATA_W-1:0] in24,
   (* versat_latency = 25 *) input [DATA_W-1:0] in25,
   (* versat_latency = 26 *) input [DATA_W-1:0] in26,
   (* versat_latency = 27 *) input [DATA_W-1:0] in27,
   (* versat_latency = 28 *) input [DATA_W-1:0] in28,
   (* versat_latency = 29 *) input [DATA_W-1:0] in29,
   (* versat_latency = 30 *) input [DATA_W-1:0] in30,
   (* versat_latency = 31 *) input [DATA_W-1:0] in31,

   (* versat_latency = 1 *) output reg [DATA_W-1:0] out0,

   input [DELAY_W-1:0] delay0
);

   reg  [DELAY_W-1:0] delay;

   wire [ DATA_W-1:0] select[31:0];

   assign select[0]  = in0;
   assign select[1]  = in1;
   assign select[2]  = in2;
   assign select[3]  = in3;
   assign select[4]  = in4;
   assign select[5]  = in5;
   assign select[6]  = in6;
   assign select[7]  = in7;
   assign select[8]  = in8;
   assign select[9]  = in9;
   assign select[10] = in10;
   assign select[11] = in11;
   assign select[12] = in12;
   assign select[13] = in13;
   assign select[14] = in14;
   assign select[15] = in15;
   assign select[16] = in16;
   assign select[17] = in17;
   assign select[18] = in18;
   assign select[19] = in19;
   assign select[20] = in20;
   assign select[21] = in21;
   assign select[22] = in22;
   assign select[23] = in23;
   assign select[24] = in24;
   assign select[25] = in25;
   assign select[26] = in26;
   assign select[27] = in27;
   assign select[28] = in28;
   assign select[29] = in29;
   assign select[30] = in30;
   assign select[31] = in31;

   reg [4:0] counter;

   always @(posedge clk, posedge rst) begin
      if (rst) begin
         delay   <= 0;
         counter <= 0;
      end else if (run) begin
         delay   <= delay0;
         counter <= 0;
      end else begin
         if (|delay) begin
            delay <= delay - 1;
         end

         if (delay == 0) begin
            counter <= counter + 1;
         end

         out0 <= select[counter];
      end
   end

endmodule

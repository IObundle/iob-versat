`timescale 1ns / 1ps
`include "xversat.vh"

module StridedMerge #(
         parameter DELAY_W = 32,
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
    (* versat_latency = 1 *) input [DATA_W-1:0]            in1,
    (* versat_latency = 2 *) input [DATA_W-1:0]            in2,
    (* versat_latency = 3 *) input [DATA_W-1:0]            in3,
    (* versat_latency = 4 *) input [DATA_W-1:0]            in4,
    (* versat_latency = 5 *) input [DATA_W-1:0]            in5,
    (* versat_latency = 6 *) input [DATA_W-1:0]            in6,
    (* versat_latency = 7 *) input [DATA_W-1:0]            in7,
    (* versat_latency = 8 *) input [DATA_W-1:0]            in8,
    (* versat_latency = 9 *) input [DATA_W-1:0]            in9,
    (* versat_latency = 10 *) input [DATA_W-1:0]            in10,
    (* versat_latency = 11 *) input [DATA_W-1:0]            in11,
    (* versat_latency = 12 *) input [DATA_W-1:0]            in12,
    (* versat_latency = 13 *) input [DATA_W-1:0]            in13,
    (* versat_latency = 14 *) input [DATA_W-1:0]            in14,
    (* versat_latency = 15 *) input [DATA_W-1:0]            in15,

    (* versat_latency = 0 *) output [DATA_W-1:0]       out0,

    input [DELAY_W-1:0]                      delay0,
    input [DELAY_W-1:0]                      stride
    );

reg [DELAY_W-1:0] delay;
reg [DELAY_W-1:0] delay2;

wire [DATA_W-1:0] select[15:0];

assign select[0] = in0;
assign select[1] = in1;
assign select[2] = in2;
assign select[3] = in3;
assign select[4] = in4;
assign select[5] = in5;
assign select[6] = in6;
assign select[7] = in7;
assign select[8] = in8;
assign select[9] = in9;
assign select[10] = in10;
assign select[11] = in11;
assign select[12] = in12;
assign select[13] = in13;
assign select[14] = in14;
assign select[15] = in15;

reg [3:0] counter;

assign out0 = running ? select[counter] : 0;

always @(posedge clk,posedge rst)
begin
     if(rst) begin
          delay <= 0;
          counter <= 0;
     end else if(run) begin
          delay <= delay0;
          counter <= 0;
          delay2 <= stride;
     end else if(running) begin
          if(|delay) begin
               delay <= delay - 1;
               if(delay == 1) begin
                    delay2 <= stride;
               end
          end else if(delay == 0) begin
               if(|delay2) begin
                    delay2 <= delay2 - 1;
               end

               if(delay2 == 0) begin
                    counter <= counter + 1;
                    delay2 <= stride;
               end
          end


          //out0 <= select[counter];
     end
end

endmodule

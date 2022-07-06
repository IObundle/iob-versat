`timescale 1ns / 1ps
`include "xversat.vh"

module Merge #(
         parameter DELAY_W = 32,
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
    input [DATA_W-1:0]            in1,
    input [DATA_W-1:0]            in2,
    input [DATA_W-1:0]            in3,
    input [DATA_W-1:0]            in4,
    input [DATA_W-1:0]            in5,
    input [DATA_W-1:0]            in6,
    input [DATA_W-1:0]            in7,
    input [DATA_W-1:0]            in8,
    input [DATA_W-1:0]            in9,
    input [DATA_W-1:0]            in10,
    input [DATA_W-1:0]            in11,
    input [DATA_W-1:0]            in12,
    input [DATA_W-1:0]            in13,
    input [DATA_W-1:0]            in14,
    input [DATA_W-1:0]            in15,

    output reg [DATA_W-1:0]       out0,

    input [DELAY_W-1:0]           delay0
    );

reg [DELAY_W-1:0] delay;

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

assign done = 1'b1;

always @(posedge clk,posedge rst)
begin
     if(rst) begin
          delay <= 0;
          counter <= 0;
     end else if(run) begin
          delay <= delay0;
          counter <= 0;
     end else begin
          if(|delay) begin
               delay <= delay - 1;
          end

          if(delay == 0) begin
               counter <= counter + 1;
          end

          out0 <= select[counter];
     end
end

endmodule

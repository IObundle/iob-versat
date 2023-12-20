`timescale 1ns / 1ps
// Comment so that verible-format will not put timescale and defaultt_nettype into same line
`default_nettype none

module xmux4 #(
   parameter DATA_W = 32
) (
   //control
   input clk_i,
   input rst_i,
   input running_i,

   input  run_i,
   output done_o,

   //input / output data
   input [DATA_W-1:0] in0_i,
   input [DATA_W-1:0] in1_i,
   input [DATA_W-1:0] in2_i,
   input [DATA_W-1:0] in3_i,

   output reg [DATA_W-1:0] out0_o,

   input [1:0] sel_i
);

   assign done_o = 1'b1;

   always @(posedge clk_i, posedge rst_i) begin
      if (rst_i) out0_o <= 0;
      else
         case (sel_i)
            2'b00: out0_o <= in0_i;
            2'b01: out0_o <= in1_i;
            2'b10: out0_o <= in2_i;
            2'b11: out0_o <= in3_i;
         endcase
   end

endmodule  // xmux4

`default_nettype wire

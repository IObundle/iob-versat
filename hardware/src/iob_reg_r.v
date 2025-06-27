`timescale 1ns / 1ps

module iob_reg_r #(
   parameter DATA_W  = 21,
   parameter RST_VAL = {DATA_W{1'b0}}
) (
   input [1-1:0] clk_i,
   input [1-1:0] cke_i,
   input [1-1:0] arst_i,

   input rst_i,

   input  [DATA_W-1:0] data_i,
   output [DATA_W-1:0] data_o
);

   wire [DATA_W-1:0] data_nxt = rst_i ? RST_VAL : data_i;

   iob_reg #(
      .DATA_W (DATA_W),
      .RST_VAL(RST_VAL)
   ) iob_reg_inst (
      .clk_i(clk_i), //clock signal
      .cke_i(cke_i), //clock enable
      .arst_i(arst_i), //asynchronous reset
      .data_i(data_nxt),
      .data_o(data_o)
   );

endmodule

`timescale 1ns / 1ps
`include "iob_reg_conf.vh"

module iob_reg #(
  parameter DATA_W = `IOB_REG_DATA_W,
  parameter RST_VAL = `IOB_REG_RST_VAL
) (
   input [1-1:0] clk_i,
   input [1-1:0] cke_i,
   input [1-1:0] arst_i,

   input      [DATA_W-1:0] data_i,
   output reg [DATA_W-1:0] data_o
);

   localparam RST_POL = `IOB_REG_RST_POL;

   generate
      if (RST_POL == 1) begin: g_rst_pol_1
         always @(posedge clk_i, posedge arst_i) begin
            if (arst_i) begin
               data_o <= RST_VAL;
            end else if (cke_i) begin
               data_o <= data_i;
            end
         end
      end else begin: g_rst_pol_0
         always @(posedge clk_i, negedge arst_i) begin
            if (~arst_i) begin
               data_o <= RST_VAL;
            end else if (cke_i) begin
               data_o <= data_i;
            end
         end
      end
   endgenerate

endmodule

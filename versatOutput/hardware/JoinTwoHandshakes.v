`timescale 1ns / 1ps

// Modules that forces the fastest side to slow down.
// Basically converts two simple handshakes into one handshake
module JoinTwoHandshakes #(
   parameter FIRST_DATA_W = 0,
   parameter SECOND_DATA_W = 0
)(
   input  first_valid_i,
   output first_ready_o,
   input  [FIRST_DATA_W-1:0]  first_data_i,

   input  second_valid_i,
   output second_ready_o,
   input  [SECOND_DATA_W-1:0] second_data_i,

   output result_valid_o,
   input  result_ready_i,
   output [FIRST_DATA_W-1:0]  result_first_data_o,
   output [SECOND_DATA_W-1:0] result_second_data_o,

   input forceReset,

   input clk_i,
   input rst_i
);

wire buffered_first_valid,buffered_second_valid;

SkidBuffer #(.DATA_W(FIRST_DATA_W)) firstBuffer(
   .in_valid_i(first_valid_i),
   .in_ready_o(first_ready_o),
   .in_data_i(first_data_i),

   .out_valid_o(buffered_first_valid),
   .out_ready_i(result_ready_i && buffered_second_valid),
   .out_data_o(result_first_data_o),

   .forceReset(forceReset),

   .clk_i(clk_i),
   .rst_i(rst_i)
);

SkidBuffer #(.DATA_W(SECOND_DATA_W)) secondBuffer(
   .in_valid_i(second_valid_i),
   .in_ready_o(second_ready_o),
   .in_data_i(second_data_i),

   .out_valid_o(buffered_second_valid),
   .out_ready_i(result_ready_i && buffered_first_valid),
   .out_data_o(result_second_data_o),

   .forceReset(forceReset),

   .clk_i(clk_i),
   .rst_i(rst_i)
);

assign result_valid_o = buffered_first_valid & buffered_second_valid;

endmodule


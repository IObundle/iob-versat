`timescale 1ns / 1ps

module Buffer #(
   parameter DELAY_W = 7,
   parameter DATA_W  = 32
) (
   //control
   input clk,
   input rst,

   input running,

   //input / output data
   input [DATA_W-1:0] in0,

   (* versat_latency = 1 *) output reg [DATA_W-1:0] out0,

   input [DELAY_W-1:0] amount
);

   wire [  DELAY_W:0] occupancy;

   wire               aboveOrEqual = (occupancy >= {1'b0, amount});
   wire               belowOrEqual = (occupancy <= {1'b0, amount});

   wire               read_en = (running && aboveOrEqual);
   wire               write_en = (running && belowOrEqual);

   wire               ext_2p_write;
   wire [DELAY_W-1:0] ext_2p_addr_out;
   wire [ DATA_W-1:0] ext_2p_data_out;

   wire               ext_2p_read;
   wire [DELAY_W-1:0] ext_2p_addr_in;
   wire [ DATA_W-1:0] ext_2p_data_in;

   my_2p_asym_ram #(
      .W_DATA_W(DATA_W),
      .R_DATA_W(DATA_W),
      .ADDR_W  (DELAY_W)
   ) ext_2p (
      // Writting port
      .w_en_i  (ext_2p_write),
      .w_addr_i(ext_2p_addr_out),
      .w_data_i(ext_2p_data_out),

      // Reading port
      .r_en_i  (ext_2p_read),
      .r_addr_i(ext_2p_addr_in),
      .r_data_o(ext_2p_data_in),

      .clk_i(clk)
   );

   reg  [DATA_W-1:0] inData;
   wire [DATA_W-1:0] fifo_data;
   iob_fifo_sync #(
      .W_DATA_W(DATA_W),
      .R_DATA_W(DATA_W),
      .ADDR_W  (DELAY_W)
   ) fifo (
      .rst_i (rst || !running), // Always clear fifo in between runs
      .clk_i (clk),
      .cke_i (1'b1),
      .arst_i(1'b0),

      .level_o(occupancy),

      //read port
      .r_data_o (fifo_data),
      .r_empty_o(),
      .r_en_i   (read_en),

      //write port
      .w_data_i(in0),
      .w_full_o(),
      .w_en_i  (write_en),

      //write port
      .ext_mem_clk_o   (),
      .ext_mem_w_en_o  (ext_2p_write),
      .ext_mem_w_addr_o(ext_2p_addr_out),
      .ext_mem_w_data_o(ext_2p_data_out),
      //read port
      .ext_mem_r_en_o  (ext_2p_read),
      .ext_mem_r_addr_o(ext_2p_addr_in),
      .ext_mem_r_data_i(ext_2p_data_in)
   );

   always @(posedge clk) begin
      if (rst) inData <= 0;
      else inData <= in0;
   end

   always @* begin
      out0 = 0;

      if (amount == 0) out0 = inData;
      else out0 = fifo_data;
   end

endmodule

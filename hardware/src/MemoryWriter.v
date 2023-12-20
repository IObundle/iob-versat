`timescale 1ns / 1ps
// Comment so that verible-format will not put timescale and defaultt_nettype into same line
`default_nettype none

module MemoryWriter #(
   parameter ADDR_W = 32,
   parameter DATA_W = 32
) (
   // Slave connected to address gen
   input                   gen_valid_i,
   output reg              gen_ready_o,
   input      [ADDR_W-1:0] gen_addr_i,

   // Slave connected to data source
   input                   data_valid_i,
   output reg              data_ready_o,
   input      [DATA_W-1:0] data_in_i,

   // Connect to memory
   output reg              mem_enable_o,
   output reg [ADDR_W-1:0] mem_addr_o,
   output reg [DATA_W-1:0] mem_data_o,

   input clk_i,
   input rst_i
);

   always @(posedge clk_i, posedge rst_i) begin
      if (rst_i) begin
         mem_enable_o <= 0;
         mem_addr_o   <= 0;
         mem_data_o   <= 0;
      end else begin
         mem_enable_o <= 1'b0;

         if (gen_valid_i && data_valid_i) begin
            mem_addr_o   <= gen_addr_i;
            mem_data_o   <= data_in_i;
            mem_enable_o <= 1'b1;
         end
      end
   end

   always @* begin
      gen_ready_o  = 1'b0;
      data_ready_o = 1'b0;

      data_ready_o = gen_valid_i;

      if (data_valid_i) begin
         gen_ready_o = 1'b1;
      end
   end

endmodule  // MemoryWriter

`default_nettype wire

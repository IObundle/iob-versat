`timescale 1ns / 1ps

/*

Receives addresses in the slave interface
Reads memory content (memory connected to the mem_ wire)
Writes data through the master interface

This module is intended to function as a simple adapter.
Connect a address generator on one side and connect this to the other and it should work fine.
TODO: To make this easier to use, the m_last_i signals should get through. The Slave interface is really poor compared to what is actually expected.
      A proper slave probable uses more logic but it simplifies the code a lot more.

*/

module MemoryReader #(
   parameter ADDR_W = 32,
   parameter DATA_W = 32
) (
   // Slave
   input               s_valid_i,
   output              s_ready_o,
   input  [ADDR_W-1:0] s_addr_i,

   // Master
   output reg              m_valid_o,
   input                   m_ready_i,
   output     [ADDR_W-1:0] m_addr_o,
   output     [DATA_W-1:0] m_data_o,
   input                   m_last_i,

   // This assumes that the memory takes "zero" cycles to return data, I think.
   // 

   // Connect to memory
   output              mem_enable_o,
   output [ADDR_W-1:0] mem_addr_o,
   input  [DATA_W-1:0] mem_data_i,

   input               force_reset_i,

   input clk_i,
   input rst_i
);

/*
   wire s_transfer = (s_valid_i && s_ready_o);
   wire m_transfer = (m_valid_o && m_ready_i);

   reg [ADDR_W-1:0] last_addr;

   always @(posedge clk_i, posedge rst_i) begin
      if (rst_i) begin
         m_valid_o <= 1'b0;
         last_addr <= 0;
      end else begin
         if (m_transfer) begin
            m_valid_o <= 1'b0;
         end

         if (s_transfer && !m_last_i) begin
            m_valid_o <= 1'b1;
            last_addr <= s_addr_i;
         end

         if(force_reset_i) begin
            m_valid_o <= 1'b0;
         end
      end
   end

   assign s_ready_o    = m_valid_o ? m_ready_i : 1'b1;

   assign mem_enable_o = s_transfer;
   assign mem_addr_o   = s_addr_i;

   assign m_data_o     = mem_data_i;
   assign m_addr_o     = last_addr;
*/

assign mem_enable_o = s_valid_i;
assign mem_addr_o = s_addr_i;

reg fifoFull;
reg [2:0] fifoLevel;

wire fifoCanReceive = fifoLevel < 4;

wire loadMem = s_valid_i && s_ready_o;
assign s_ready_o = fifoCanReceive;

reg [ADDR_W-1:0] delayed_s_addr_i,delayed_s_addr_i_2;
reg mem_en_delayed,mem_en_delayed_2;

always @(posedge clk_i,posedge rst_i) begin
   if(rst_i) begin
      mem_en_delayed <= 0;
      mem_en_delayed_2 <= 0;
      delayed_s_addr_i <= 0;
      delayed_s_addr_i_2 <= 0;
   end else begin
      mem_en_delayed <= loadMem;
      mem_en_delayed_2 <= mem_en_delayed;

      delayed_s_addr_i <= s_addr_i;
      delayed_s_addr_i_2 <= delayed_s_addr_i;

      if(force_reset_i) begin
         mem_en_delayed <= 0;
         mem_en_delayed_2 <= 0;
         delayed_s_addr_i <= 0;
         delayed_s_addr_i_2 <= 0;
      end
   end
end

assign m_valid_o = (fifoLevel > 0);

SimpleFifo #(.DATA_W(DATA_W + ADDR_W),.COUNT(5)) fifo (
   .clk_i(clk_i),
   .rst_i(rst_i || force_reset_i),

   //write port
   .w_en_i(mem_en_delayed_2),
   .w_data_i({delayed_s_addr_i_2,mem_data_i}),
   .w_full_o(fifoFull),

   //read port
   .r_en_i(m_valid_o && m_ready_i),
   .r_data_o({m_addr_o,m_data_o}),
   .r_empty_o(),

   //FIFO level
   .level_o(fifoLevel)
);

endmodule  // MemoryReader

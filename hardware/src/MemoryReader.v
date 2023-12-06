`timescale 1ns / 1ps
// Comment so that verible-format will not put timescale and defaultt_nettype into same line
`default_nettype none

// Axi like interface 
module MemoryReader 
  #(
    parameter ADDR_W = 32,
    parameter DATA_W = 32
    )(
      // Slave
      input               s_valid_i,
      output              s_ready_o,
      input [ADDR_W-1:0]  s_addr_i,

      // Master
      output reg          m_valid_o,
      input               m_ready_i,
      output [ADDR_W-1:0] m_addr_o,
      output [DATA_W-1:0] m_data_o, 
      input               m_last_i,

      // Connect to memory
      output              mem_enable_o,
      output [ADDR_W-1:0] mem_addr_o,
      input [DATA_W-1:0]  mem_data_i,

      input               clk_i,
      input               rst_i
      );

wire s_transfer = (s_valid_i && s_ready_o);
wire m_transfer = (m_valid_o && m_ready_i);

reg [ADDR_W-1:0] last_addr;

always @(posedge clk_i,posedge rst_i)
begin
   if(rst_i) begin
      m_valid_o <= 1'b0;
      last_addr <= 0;
   end else begin
      if(m_transfer) begin
         m_valid_o <= 1'b0;
      end

      if(s_transfer && !m_last_i) begin
         m_valid_o <= 1'b1;
         last_addr <= s_addr_i;
      end
   end
end

assign s_ready_o = m_valid_o ? m_ready_i : 1'b1;

assign mem_enable_o = s_transfer;
assign mem_addr_o = s_addr_i;

assign m_data_o = mem_data_i;
assign m_addr_o = last_addr;

endmodule // MemoryReader

`default_nettype wire

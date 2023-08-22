`timescale 1ns / 1ps

// Axi like interface 
module MemoryReader #(
      parameter ADDR_W = 32,
      parameter DATA_W = 32
   )(
      // Slave
      input s_valid,
      output s_ready,
      input [ADDR_W-1:0] s_addr,

      // Master
      output reg m_valid,
      input m_ready,
      output [ADDR_W-1:0] m_addr,
      output [DATA_W-1:0] m_data,      

      // Connect to memory
      output              mem_enable,
      output [ADDR_W-1:0] mem_addr,
      input  [DATA_W-1:0] mem_data,

      input clk,
      input rst
   );

wire s_transfer = (s_valid && s_ready);
wire m_transfer = (m_valid && m_ready);

reg [ADDR_W-1:0] last_addr;

always @(posedge clk,posedge rst)
begin
   if(rst) begin
      m_valid <= 1'b0;
      last_addr <= 0;
   end else begin
      if(m_transfer) begin
         m_valid <= 1'b0;
      end

      if(s_transfer) begin
         m_valid <= 1'b1;
         last_addr <= s_addr;
      end
   end
end

assign s_ready = m_valid ? m_ready : 1'b1;

assign mem_enable = s_transfer;
assign mem_addr = s_addr;

assign m_data = mem_data;
assign m_addr = last_addr;

endmodule 

`timescale 1ns / 1ps

// A simple DMA mostly used to load memory mapped units and in some cases configuration data.

module SimpleDMA #(
      parameter ADDR_W = 32,
      parameter DATA_W = 32,
      parameter AXI_ADDR_W = 32,
      parameter LEN_W = 20
   )(
      // Master interface used to fetch data from memory
      input                   m_databus_ready,
      output                  m_databus_valid,
      output [ADDR_W-1:0]     m_databus_addr,
      input  [DATA_W-1:0]     m_databus_rdata,
      output [DATA_W-1:0]     m_databus_wdata,
      output [(DATA_W/8)-1:0] m_databus_wstrb,
      output [LEN_W-1:0]      m_databus_len,
      input                   m_databus_last,      

      // Configuration (set before asserting run)
      input                   addr_internal,
      input [AXI_ADDR_W-1:0]  addr_read,
      input [LEN_W-1:0]       length,

      // Run and running status
      input run,
      output reg running,

      output                   valid,
      output reg [ADDR_W-1:0]  address,
      output [DATA_W-1:0]      data,

      input clk,
      input rst
   );

assign m_databus_addr = addr_read;
assign m_databus_len = length;
assign m_databus_wdata = 0;
assign m_databus_wstrb = 0;

assign m_databus_valid = running;
assign valid = m_databus_ready;
assign data  = m_databus_rdata;

always @(posedge clk,posedge rst) begin
   if(rst) begin
      address <= 0;
      running <= 0;
   end else if(running) begin
      if(m_databus_ready) begin
         address <= address + 4;
            
         if(m_databus_last) begin
            running <= 1'b0;
         end
      end
   end else if(run) begin
      address <= addr_internal;
      running <= 1'b1;
   end
end

endmodule

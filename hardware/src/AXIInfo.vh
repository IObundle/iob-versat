`ifndef INCLUDED_AXI_INFO
`define INCLUDED_AXI_INFO

function integer calculate_AXI_OFFSET_W(input integer AXI_DATA_W);
   begin
      calculate_AXI_OFFSET_W = $clog2((AXI_DATA_W/8));
   end
endfunction

function [2:0] calculate_AXI_AXSIZE(input integer AXI_DATA_W);
   begin
      calculate_AXI_AXSIZE = 0;
      if(AXI_DATA_W >= 8)
         calculate_AXI_AXSIZE = 3'b000;
      if(AXI_DATA_W >= 16)
         calculate_AXI_AXSIZE = 3'b001;
      if(AXI_DATA_W >= 32)
         calculate_AXI_AXSIZE = 3'b010;
      if(AXI_DATA_W >= 64)
         calculate_AXI_AXSIZE = 3'b011;
      if(AXI_DATA_W >= 128)
         calculate_AXI_AXSIZE = 3'b100;
      if(AXI_DATA_W >= 256)
         calculate_AXI_AXSIZE = 3'b101;
      if(AXI_DATA_W >= 512)
         calculate_AXI_AXSIZE = 3'b110;
      if(AXI_DATA_W >= 1024)
         calculate_AXI_AXSIZE = 3'b111;
   end
endfunction

`endif

// Assumes maximum throughput.
// Computes AXI values for a given transfer (given actual address and actual transfer length in bytes)

/*
module AXIInfo #(
   parameter AXI_ADDR_W = 32,
   parameter AXI_DATA_W = 32,
   parameter TRANSFER_LEN_W = 8
   )
   (
      input [AXI_ADDR_W-1:0]     start_address,
      input [TRANSFER_LEN_W-1:0] transfer_length, // Actual size of transfer, in bytes

      output                     boundary_4k_condition,
      output                     unaligned,

      input [AXI_ADDR_W-1:0]     addr,
                                 addr_offset_bit, // Bits that define address alignment for the given AXI_DATA_W selected

      output [7:0]               m_axi_awlen,
      output reg [2:0]           m_axi_axsize
   );

localparam OFFSET_W = calculate_AXI_OFFSET_W(AXI_DATA_W/8);

output reg [OFFSET_W-1:0] addr_offset_bit = addr[OFFSET_W-1:0];

// TODO: Probably should check if not true power of 2
always @*
begin
   m_axi_axsize = 0;
   if(AXI_DATA_W >= 8)
      m_axi_axsize = 3'b000;
   if(AXI_DATA_W >= 16)
      m_axi_axsize = 3'b001;
   if(AXI_DATA_W >= 32)
      m_axi_axsize = 3'b010;
   if(AXI_DATA_W >= 64)
      m_axi_axsize = 3'b011;
   if(AXI_DATA_W >= 128)
      m_axi_axsize = 3'b100;
   if(AXI_DATA_W >= 256)
      m_axi_axsize = 3'b101;
   if(AXI_DATA_W >= 512)
      m_axi_axsize = 3'b110;
   if(AXI_DATA_W >= 1024)
      m_axi_axsize = 3'b111;
end

wire [LEN_W-1:0] m_axi_awlen = (32'd1020 + (32'd4 - address[1:0]));

endmodule

*/

`default_nettype wire

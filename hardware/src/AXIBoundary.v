`timescale 1ns / 1ps

module AXIBoundaryWrite #(
   parameter ADDR_W = 0,
   parameter DATA_W = 32
) (
   input                     s_valid_i,
   output                    s_ready_o,
   input  [      ADDR_W-1:0] s_addr_i,
   input  [      DATA_W-1:0] s_wdata_i,
   input  [(DATA_W / 8)-1:0] s_wstrb_i,
   input  [             7:0] s_len_i,

   output                    m_valid_o,
   input                     m_ready_i,
   output [      ADDR_W-1:0] m_addr_o,
   output [      DATA_W-1:0] m_wdata_o,
   output [(DATA_W / 8)-1:0] m_wstrb_o,
   output [             7:0] m_len_o,

   input clk_i,
   input rst_i
);

   assign m_valid_o = s_valid_i;
   assign s_ready_o = m_ready_i;
   assign m_addr_o  = s_addr_i;
   assign m_wdata_o = s_wdata_i;
   assign m_wstrb_o = s_wstrb_i;
   assign m_len_o   = s_len_i;

   /*
wire [ADDR_W-1:0] addr_boundary = {s_addr_i[ADDR_W-1:12],12'h0};
wire [ADDR_W-1:0] start_addr = {s_addr_i[ADDR_W-1:2],2'b00};
wire [ADDR_W-1:0] end_addr = (start_addr + (s_len_i + 1) * 4);

wire need_to_split = (end_addr > addr_boundary);

wire [7:0] before_split_len = ((~start_addr[9:0]) >> 2) - 1;
wire [7:0] after_split_len = (len - before_split_len + 1);

reg do_split;
reg did_split;
always @(posedge clk,posedge rst)
begin
  if(rst) begin
    do_split <= 1'b0;
    did_split <= 1'b0;
  end else begin

  end
end

always @*
begin
    m_addr_o = 0;
    m_len_o = 0;
    valid = 1'b0;
    enable_transfer = 1'b0;

    if(need_to_split) begin
      

    end else begin
      m_valid_o = s_valid_i;
      s_ready_o = m_ready_i;
      m_addr_o = s_addr_i;
      m_wdata_o = s_wdata_i;
      m_wstrb_o = s_wstrb_i;
      m_len_o = s_len_i;
    end
end
*/

endmodule  // AXIBoundaryWrite

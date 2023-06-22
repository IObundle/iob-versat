`timescale 1ns/1ps

module AXIBoundaryWrite #(
    parameter ADDR_W = 0,
    parameter DATA_W = 32
  )
  (
    input s_valid,
    output s_ready,
    input [ADDR_W-1:0] s_addr,
    input [DATA_W-1:0] s_wdata,
    input [(DATA_W / 8)-1:0] s_wstrb,
    input [7:0] s_len,

    output m_valid,
    input m_ready,
    output [ADDR_W-1:0] m_addr,
    output [DATA_W-1:0] m_wdata,
    output [(DATA_W / 8)-1:0] m_wstrb,
    output [7:0] m_len,

    input clk,
    input rst
  );

assign m_valid = s_valid;
assign s_ready = m_ready;
assign m_addr = s_addr;
assign m_wdata = s_wdata;
assign m_wstrb = s_wstrb;
assign m_len = s_len;

/*
wire [ADDR_W-1:0] addr_boundary = {s_addr[ADDR_W-1:12],12'h0};
wire [ADDR_W-1:0] start_addr = {s_addr[ADDR_W-1:2],2'b00};
wire [ADDR_W-1:0] end_addr = (start_addr + (s_len + 1) * 4);

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
    m_addr = 0;
    m_len = 0;
    valid = 1'b0;
    enable_transfer = 1'b0;

    if(need_to_split) begin
      

    end else begin
      m_valid = s_valid;
      s_ready = m_ready;
      m_addr = s_addr;
      m_wdata = s_wdata;
      m_wstrb = s_wstrb;
      m_len = s_len;
    end
end
*/

endmodule
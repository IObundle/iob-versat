`timescale 1ns / 1ps
// Comment so that verible-format will not put timescale and defaultt_nettype into same line
`default_nettype none

module SimpleAXItoAXI 
  #(
    parameter AXI_ADDR_W = 32,
    parameter AXI_DATA_W = 32,
    parameter AXI_LEN_W = 8,
    parameter AXI_ID_W = 4,
    parameter LEN_W = 8
    )
   (
    input                        m_wvalid_i,
    output reg                   m_wready_o,
    input [AXI_ADDR_W-1:0]       m_waddr_i,
    input [AXI_DATA_W-1:0]       m_wdata_i,
    input [(AXI_DATA_W / 8)-1:0] m_wstrb_i,
    input [LEN_W-1:0]            m_wlen_i,
    output reg                   m_wlast_o,
   
    input                        m_rvalid_i,
    output reg                   m_rready_o,
    input [AXI_ADDR_W-1:0]       m_raddr_i,
    output [AXI_DATA_W-1:0]      m_rdata_o,
    input [LEN_W-1:0]            m_rlen_i,
    output                       m_rlast_o,

`include "axi_m_port.vs"

    input                        clk_i,
    input                        rst_i
    );

SimpleAXItoAXIWrite #(.AXI_ADDR_W(AXI_ADDR_W),.AXI_DATA_W(AXI_DATA_W),.AXI_ID_W(AXI_ID_W),.AXI_LEN_W(AXI_LEN_W),.LEN_W(LEN_W)) simpleWrite
  (
    .m_wvalid_i(m_wvalid_i),
    .m_wready_o(m_wready_o),
    .m_waddr_i(m_waddr_i),
    .m_wdata_i(m_wdata_i),
    .m_wstrb_i(m_wstrb_i),
    .m_wlen_i(m_wlen_i),
    .m_wlast_o(m_wlast_o),

    `include "axi_m_m_write_portmap.vs"

    .clk_i(clk_i),
    .rst_i(rst_i)
  );

SimpleAXItoAXIRead #(.AXI_ADDR_W(AXI_ADDR_W),.AXI_DATA_W(AXI_DATA_W),.AXI_ID_W(AXI_ID_W),.AXI_LEN_W(AXI_LEN_W),.LEN_W(LEN_W)) simpleRead
  (
    .m_rvalid_i(m_rvalid_i),
    .m_rready_o(m_rready_o),
    .m_raddr_i(m_raddr_i),
    .m_rdata_o(m_rdata_o),
    .m_rlen_i(m_rlen_i),
    .m_rlast_o(m_rlast_o),

    `include "axi_m_m_read_portmap.vs"

    .clk_i(clk_i),
    .rst_i(rst_i)
  );

endmodule // SimpleAXItoAXI

`default_nettype wire

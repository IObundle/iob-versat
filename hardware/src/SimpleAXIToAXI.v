`timescale 1ns/1ps
`include "axi.vh"

module SimpleAXItoAXI #(
    parameter AXI_ADDR_W = 32,
    parameter AXI_DATA_W = 32,
    parameter AXI_LEN_W = 8,
    parameter AXI_ID_W = 4,
    parameter LEN_W = 8
  )
  (
    input  m_wvalid,
    output reg m_wready,
    input  [AXI_ADDR_W-1:0] m_waddr,
    input  [AXI_DATA_W-1:0] m_wdata,
    input  [(AXI_DATA_W / 8)-1:0] m_wstrb,
    input  [LEN_W-1:0] m_wlen,
    output reg m_wlast,
    
    input  m_rvalid,
    output reg m_rready,
    input  [AXI_ADDR_W-1:0] m_raddr,
    output [AXI_DATA_W-1:0] m_rdata,
    input  [LEN_W-1:0] m_rlen,
    output m_rlast,

    `include "m_versat_axi_m_port.vh"

    input clk,
    input rst
  );

SimpleAXItoAXIWrite #(.AXI_ADDR_W(AXI_ADDR_W),.AXI_DATA_W(AXI_DATA_W),.AXI_ID_W(AXI_ID_W),.AXI_LEN_W(AXI_LEN_W),.LEN_W(LEN_W)) simpleWrite
  (
    .m_wvalid(m_wvalid),
    .m_wready(m_wready),
    .m_waddr(m_waddr),
    .m_wdata(m_wdata),
    .m_wstrb(m_wstrb),
    .m_wlen(m_wlen),
    .m_wlast(m_wlast),

    `include "m_versat_axi_write_portmap.vh"

    .clk(clk),
    .rst(rst)
  );

SimpleAXItoAXIRead #(.AXI_ADDR_W(AXI_ADDR_W),.AXI_DATA_W(AXI_DATA_W),.AXI_ID_W(AXI_ID_W),.AXI_LEN_W(AXI_LEN_W),.LEN_W(LEN_W)) simpleRead
  (
    .m_rvalid(m_rvalid),
    .m_rready(m_rready),
    .m_raddr(m_raddr),
    .m_rdata(m_rdata),
    .m_rlen(m_rlen),
    .m_rlast(m_rlast),

    `include "m_versat_axi_read_portmap.vh"

    .clk(clk),
    .rst(rst)
  );

endmodule


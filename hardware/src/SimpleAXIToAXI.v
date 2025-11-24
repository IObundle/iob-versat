`timescale 1ns / 1ps

module SimpleAXIToAXI #(
   parameter AXI_ADDR_W = 32,
   parameter AXI_DATA_W = 32,
   parameter AXI_LEN_W  = 8,
   parameter AXI_ID_W   = 4,
   parameter LEN_W      = 8
) (
   input                             m_wvalid_i,
   output reg                        m_wready_o,
   input      [      AXI_ADDR_W-1:0] m_waddr_i,
   input      [      AXI_DATA_W-1:0] m_wdata_i,
   input      [(AXI_DATA_W / 8)-1:0] m_wstrb_i,
   input      [           LEN_W-1:0] m_wlen_i,
   output reg                        m_wlast_o,

   input                       m_rvalid_i,
   output reg                  m_rready_o,
   input      [AXI_ADDR_W-1:0] m_raddr_i,
   output     [AXI_DATA_W-1:0] m_rdata_o,
   input      [     LEN_W-1:0] m_rlen_i,
   output                      m_rlast_o,

   output [AXI_ID_W-1:0] axi_awid_o,
   output [AXI_ADDR_W-1:0] axi_awaddr_o,
   output [AXI_LEN_W-1:0] axi_awlen_o,
   output [3-1:0] axi_awsize_o,
   output [2-1:0] axi_awburst_o,
   output [2-1:0] axi_awlock_o,
   output [4-1:0] axi_awcache_o,
   output [3-1:0] axi_awprot_o,
   output [4-1:0] axi_awqos_o,
   output [1-1:0] axi_awvalid_o,
   input [1-1:0] axi_awready_i,
   output [AXI_DATA_W-1:0] axi_wdata_o,
   output [(AXI_DATA_W/8)-1:0] axi_wstrb_o,
   output [1-1:0] axi_wlast_o,
   output [1-1:0] axi_wvalid_o,
   input [1-1:0] axi_wready_i,
   input [AXI_ID_W-1:0] axi_bid_i,
   input [2-1:0] axi_bresp_i,
   input [1-1:0] axi_bvalid_i,
   output [1-1:0] axi_bready_o,
   output [AXI_ID_W-1:0] axi_arid_o,
   output [AXI_ADDR_W-1:0] axi_araddr_o,
   output [AXI_LEN_W-1:0] axi_arlen_o,
   output [3-1:0] axi_arsize_o,
   output [2-1:0] axi_arburst_o,
   output [2-1:0] axi_arlock_o,
   output [4-1:0] axi_arcache_o,
   output [3-1:0] axi_arprot_o,
   output [4-1:0] axi_arqos_o,
   output [1-1:0] axi_arvalid_o,
   input [1-1:0] axi_arready_i,
   input [AXI_ID_W-1:0] axi_rid_i,
   input [AXI_DATA_W-1:0] axi_rdata_i,
   input [2-1:0] axi_rresp_i,
   input [1-1:0] axi_rlast_i,
   input [1-1:0] axi_rvalid_i,
   output [1-1:0] axi_rready_o,

   input clk_i,
   input rst_i
);

   wire temp_axi_awvalid;
   wire temp_axi_awready;
   wire temp_axi_wvalid;
   wire temp_axi_wready;
   wire temp_axi_bvalid;
   wire temp_axi_bready;

   VersatAxiDelay delayAW(
      .s_valid_i(temp_axi_awvalid),
      .s_ready_o(temp_axi_awready),
      .m_valid_o(axi_awvalid_o),
      .m_ready_i(axi_awready_i),
      .clk_i(clk_i),
      .rst_i(rst_i)      
   );

   VersatAxiDelay delayW(
      .s_valid_i(temp_axi_wvalid),
      .s_ready_o(temp_axi_wready),
      .m_valid_o(axi_wvalid_o),
      .m_ready_i(axi_wready_i),
      .clk_i(clk_i),
      .rst_i(rst_i)      
   );

   VersatAxiDelay delayB(
      .s_valid_i(axi_bvalid_i),
      .s_ready_o(axi_bready_o),
      .m_valid_o(temp_axi_bvalid),
      .m_ready_i(temp_axi_bready),
      .clk_i(clk_i),
      .rst_i(rst_i)      
   );

   SimpleAXIToAXIWrite #(
      .AXI_ADDR_W(AXI_ADDR_W),
      .AXI_DATA_W(AXI_DATA_W),
      .AXI_ID_W  (AXI_ID_W),
      .AXI_LEN_W (AXI_LEN_W),
      .LEN_W     (LEN_W)
   ) simpleWrite (
      .m_wvalid_i(m_wvalid_i),
      .m_wready_o(m_wready_o),
      .m_waddr_i (m_waddr_i),
      .m_wdata_i (m_wdata_i),
      .m_wstrb_i (m_wstrb_i),
      .m_wlen_i  (m_wlen_i),
      .m_wlast_o (m_wlast_o),

      .axi_awid_o(axi_awid_o), //Address write channel ID.
      .axi_awaddr_o(axi_awaddr_o), //Address write channel address.
      .axi_awlen_o(axi_awlen_o), //Address write channel burst length.
      .axi_awsize_o(axi_awsize_o), //Address write channel burst size. This signal indicates the size of each transfer in the burst.
      .axi_awburst_o(axi_awburst_o), //Address write channel burst type.
      .axi_awlock_o(axi_awlock_o), //Address write channel lock type.
      .axi_awcache_o(axi_awcache_o), //Address write channel memory type. Set to 0000 if master output; ignored if slave input.
      .axi_awprot_o(axi_awprot_o), //Address write channel protection type. Set to 000 if master output; ignored if slave input.
      .axi_awqos_o(axi_awqos_o), //Address write channel quality of service.
      .axi_awvalid_o(temp_axi_awvalid), //Address write channel valid.
      .axi_awready_i(temp_axi_awready), //Address write channel ready.
      .axi_wdata_o(axi_wdata_o), //Write channel data.
      .axi_wstrb_o(axi_wstrb_o), //Write channel write strobe.
      .axi_wlast_o(axi_wlast_o), //Write channel last word flag.
      .axi_wvalid_o(temp_axi_wvalid), //Write channel valid.
      .axi_wready_i(temp_axi_wready), //Write channel ready.
      .axi_bid_i(axi_bid_i), //Write response channel ID.
      .axi_bresp_i(axi_bresp_i), //Write response channel response.
      .axi_bvalid_i(temp_axi_bvalid), //Write response channel valid.
      .axi_bready_o(temp_axi_bready), //Write response channel ready.

      .clk_i(clk_i),
      .rst_i(rst_i)
   );

   SimpleAXIToAXIRead #(
      .AXI_ADDR_W(AXI_ADDR_W),
      .AXI_DATA_W(AXI_DATA_W),
      .AXI_ID_W  (AXI_ID_W),
      .AXI_LEN_W (AXI_LEN_W),
      .LEN_W     (LEN_W)
   ) simpleRead (
      .m_rvalid_i(m_rvalid_i),
      .m_rready_o(m_rready_o),
      .m_raddr_i (m_raddr_i),
      .m_rdata_o (m_rdata_o),
      .m_rlen_i  (m_rlen_i),
      .m_rlast_o (m_rlast_o),

      .axi_arid_o(axi_arid_o), //Address read channel ID.
      .axi_araddr_o(axi_araddr_o), //Address read channel address.
      .axi_arlen_o(axi_arlen_o), //Address read channel burst length.
      .axi_arsize_o(axi_arsize_o), //Address read channel burst size. This signal indicates the size of each transfer in the burst.
      .axi_arburst_o(axi_arburst_o), //Address read channel burst type.
      .axi_arlock_o(axi_arlock_o), //Address read channel lock type.
      .axi_arcache_o(axi_arcache_o), //Address read channel memory type. Set to 0000 if master output; ignored if slave input.
      .axi_arprot_o(axi_arprot_o), //Address read channel protection type. Set to 000 if master output; ignored if slave input.
      .axi_arqos_o(axi_arqos_o), //Address read channel quality of service.
      .axi_arvalid_o(axi_arvalid_o), //Address read channel valid.
      .axi_arready_i(axi_arready_i), //Address read channel ready.
      .axi_rid_i(axi_rid_i), //Read channel ID.
      .axi_rdata_i(axi_rdata_i), //Read channel data.
      .axi_rresp_i(axi_rresp_i), //Read channel response.
      .axi_rlast_i(axi_rlast_i), //Read channel last word.
      .axi_rvalid_i(axi_rvalid_i), //Read channel valid.
      .axi_rready_o(axi_rready_o), //Read channel ready.

      .clk_i(clk_i),
      .rst_i(rst_i)
   );

endmodule  // SimpleAXItoAXI

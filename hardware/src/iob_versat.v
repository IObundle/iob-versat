`timescale 1ns / 1ps

//`include "axi.vh"
//`include "system.vh"
//`include "iob_lib.vh"
//`include "iob_intercon.vh"
//`include "iob_versat.vh"

`include "versat_defs.vh"

module iob_versat #(  //the below parameters are used in cpu if includes below
   parameter AXI_ADDR_W = 32,
   parameter AXI_DATA_W = 32,
   parameter AXI_LEN_W  = 8,
   parameter AXI_ID_W   = 1,
   parameter ADDR_W     = 26,
   parameter DATA_W     = 32,
   parameter WDATA_W    = 32
) (
   input                   iob_avalid_i,
   input  [    ADDR_W-1:0] iob_addr_i,
   input  [    DATA_W-1:0] iob_wdata_i,
   input  [(DATA_W/8)-1:0] iob_wstrb_i,
   output                  iob_rvalid_o,
   output [    DATA_W-1:0] iob_rdata_o,
   output                  iob_ready_o,

`ifdef VERSAT_EXTERNAL_MEMORY
   `include "versat_external_memory_port.vh"
`endif

`ifdef VERSAT_IO
   output [      AXI_ID_W-1:0] axi_awid_o,
   output [    AXI_ADDR_W-1:0] axi_awaddr_o,
   output [     AXI_LEN_W-1:0] axi_awlen_o,
   output [             3-1:0] axi_awsize_o,
   output [             2-1:0] axi_awburst_o,
   output [             2-1:0] axi_awlock_o,
   output [             4-1:0] axi_awcache_o,
   output [             3-1:0] axi_awprot_o,
   output [             4-1:0] axi_awqos_o,
   output [             1-1:0] axi_awvalid_o,
   input  [             1-1:0] axi_awready_i,
   output [    AXI_DATA_W-1:0] axi_wdata_o,
   output [(AXI_DATA_W/8)-1:0] axi_wstrb_o,
   output [             1-1:0] axi_wlast_o,
   output [             1-1:0] axi_wvalid_o,
   input  [             1-1:0] axi_wready_i,
   input  [      AXI_ID_W-1:0] axi_bid_i,
   input  [             2-1:0] axi_bresp_i,
   input  [             1-1:0] axi_bvalid_i,
   output [             1-1:0] axi_bready_o,
   output [      AXI_ID_W-1:0] axi_arid_o,
   output [    AXI_ADDR_W-1:0] axi_araddr_o,
   output [     AXI_LEN_W-1:0] axi_arlen_o,
   output [             3-1:0] axi_arsize_o,
   output [             2-1:0] axi_arburst_o,
   output [             2-1:0] axi_arlock_o,
   output [             4-1:0] axi_arcache_o,
   output [             3-1:0] axi_arprot_o,
   output [             4-1:0] axi_arqos_o,
   output [             1-1:0] axi_arvalid_o,
   input  [             1-1:0] axi_arready_i,
   input  [      AXI_ID_W-1:0] axi_rid_i,
   input  [    AXI_DATA_W-1:0] axi_rdata_i,
   input  [             2-1:0] axi_rresp_i,
   input  [             1-1:0] axi_rlast_i,
   input  [             1-1:0] axi_rvalid_i,
   output [             1-1:0] axi_rready_o,
`endif

`ifdef EXTERNAL_PORTS
   input  [31:0] in0_i,
   input  [31:0] in1_i,
   output [31:0] out0_o,
`endif

   input cke_i,
   input clk_i,
   input arst_i
   //input                   rst_i
);

   assign iob_ready_o = 1'b1;

   localparam LEN_W = `LEN_W;
   localparam IO = `nIO;

`ifdef VERSAT_IO
   wire [               IO-1:0] m_databus_ready;
   wire [               IO-1:0] m_databus_valid;
   wire [    IO*AXI_ADDR_W-1:0] m_databus_addr;
   wire [       AXI_DATA_W-1:0] m_databus_rdata;
   wire [    IO*AXI_DATA_W-1:0] m_databus_wdata;
   wire [IO*(AXI_DATA_W/8)-1:0] m_databus_wstrb;
   wire [         IO*LEN_W-1:0] m_databus_len;
   wire [               IO-1:0] m_databus_last;

   wire w_ready, w_valid;
   wire [  AXI_ADDR_W-1:0] w_addr;
   wire [  AXI_DATA_W-1:0] w_data;
   wire [AXI_DATA_W/8-1:0] w_strb;
   wire [       LEN_W-1:0] w_len;

   wire r_ready, r_valid;
   wire [AXI_ADDR_W-1:0] r_addr;
   wire [AXI_DATA_W-1:0] r_data;
   wire [     LEN_W-1:0] r_len;

   wire w_last, r_last;

   // TODO: To improve performance in later stages, it would be helpful to further separate into current master being served and next master to be served.
   //       That way, the SimpleAXItoAXI module could be changed to pipeline the transfer calculations for the next master while servicing the current master.
   xmerge #(
      .N_SLAVES(IO),
      .ADDR_W  (AXI_ADDR_W),
      .DATA_W  (AXI_DATA_W),
      .LEN_W   (LEN_W)
   ) merge (
      .s_valid_i(m_databus_valid),
      .s_ready_o(m_databus_ready),
      .s_last_o (m_databus_last),
      .s_addr_i (m_databus_addr),
      .s_wdata_i(m_databus_wdata),
      .s_wstrb_i(m_databus_wstrb),
      .s_rdata_o(m_databus_rdata),
      .s_len_i  (m_databus_len),

      .m_wvalid_o(w_valid),
      .m_wready_i(w_ready),
      .m_waddr_o (w_addr),
      .m_wdata_o (w_data),
      .m_wstrb_o (w_strb),
      .m_wlen_o  (w_len),
      .m_wlast_i (w_last),

      .m_rvalid_o(r_valid),
      .m_rready_i(r_ready),
      .m_raddr_o (r_addr),
      .m_rdata_i (r_data),
      .m_rlen_o  (r_len),
      .m_rlast_i (r_last),

      .clk_i(clk_i),
      .rst_i(arst_i)
   );

   SimpleAXItoAXI #(
      .AXI_ADDR_W(AXI_ADDR_W),
      .AXI_DATA_W(AXI_DATA_W),
      .AXI_ID_W  (AXI_ID_W),
      .AXI_LEN_W (AXI_LEN_W),
      .LEN_W     (LEN_W)
   ) simpleToAxi (
      .m_wvalid_i(w_valid),
      .m_wready_o(w_ready),
      .m_waddr_i (w_addr),
      .m_wdata_i (w_data),
      .m_wstrb_i (w_strb),
      .m_wlen_i  (w_len),
      .m_wlast_o (w_last),

      .m_rvalid_i(r_valid),
      .m_rready_o(r_ready),
      .m_raddr_i (r_addr),
      .m_rdata_o (r_data),
      .m_rlen_i  (r_len),
      .m_rlast_o (r_last),

      .axi_awid_o(axi_awid_o),  //Address write channel ID.
      .axi_awaddr_o(axi_awaddr_o),  //Address write channel address.
      .axi_awlen_o(axi_awlen_o),  //Address write channel burst length.
      .axi_awsize_o(axi_awsize_o), //Address write channel burst size. This signal indicates the size of each transfer in the burst.
      .axi_awburst_o(axi_awburst_o),  //Address write channel burst type.
      .axi_awlock_o(axi_awlock_o),  //Address write channel lock type.
      .axi_awcache_o(axi_awcache_o), //Address write channel memory type. Set to 0000 if master output; ignored if slave input.
      .axi_awprot_o(axi_awprot_o), //Address write channel protection type. Set to 000 if master output; ignored if slave input.
      .axi_awqos_o(axi_awqos_o),  //Address write channel quality of service.
      .axi_awvalid_o(axi_awvalid_o),  //Address write channel valid.
      .axi_awready_i(axi_awready_i),  //Address write channel ready.
      .axi_wdata_o(axi_wdata_o),  //Write channel data.
      .axi_wstrb_o(axi_wstrb_o),  //Write channel write strobe.
      .axi_wlast_o(axi_wlast_o),  //Write channel last word flag.
      .axi_wvalid_o(axi_wvalid_o),  //Write channel valid.
      .axi_wready_i(axi_wready_i),  //Write channel ready.
      .axi_bid_i(axi_bid_i),  //Write response channel ID.
      .axi_bresp_i(axi_bresp_i),  //Write response channel response.
      .axi_bvalid_i(axi_bvalid_i),  //Write response channel valid.
      .axi_bready_o(axi_bready_o),  //Write response channel ready.
      .axi_arid_o(axi_arid_o),  //Address read channel ID.
      .axi_araddr_o(axi_araddr_o),  //Address read channel address.
      .axi_arlen_o(axi_arlen_o),  //Address read channel burst length.
      .axi_arsize_o(axi_arsize_o), //Address read channel burst size. This signal indicates the size of each transfer in the burst.
      .axi_arburst_o(axi_arburst_o),  //Address read channel burst type.
      .axi_arlock_o(axi_arlock_o),  //Address read channel lock type.
      .axi_arcache_o(axi_arcache_o), //Address read channel memory type. Set to 0000 if master output; ignored if slave input.
      .axi_arprot_o(axi_arprot_o), //Address read channel protection type. Set to 000 if master output; ignored if slave input.
      .axi_arqos_o(axi_arqos_o),  //Address read channel quality of service.
      .axi_arvalid_o(axi_arvalid_o),  //Address read channel valid.
      .axi_arready_i(axi_arready_i),  //Address read channel ready.
      .axi_rid_i(axi_rid_i),  //Read channel ID.
      .axi_rdata_i(axi_rdata_i),  //Read channel data.
      .axi_rresp_i(axi_rresp_i),  //Read channel response.
      .axi_rlast_i(axi_rlast_i),  //Read channel last word.
      .axi_rvalid_i(axi_rvalid_i),  //Read channel valid.
      .axi_rready_o(axi_rready_o),  //Read channel ready.

      .clk_i(clk_i),
      .rst_i(arst_i)
   );

`endif

   // For now keep the versat_instance using old names (no _i or _o)
   versat_instance #(
      .ADDR_W    (ADDR_W),
      .DATA_W    (DATA_W),
      .AXI_DATA_W(AXI_DATA_W),
      .LEN_W     (LEN_W)
   ) xversat (
      .valid (iob_avalid_i),
      .wstrb (iob_wstrb_i),
      .addr  (iob_addr_i),
      .wdata (iob_wdata_i),
      .rdata (iob_rdata_o),
      .rvalid(iob_rvalid_o),
      //.ready(iob_ready_o),

`ifdef VERSAT_EXTERNAL_MEMORY
      `include "versat_external_memory_internal_portmap.vh"
`endif

`ifdef VERSAT_IO
      .m_databus_ready(m_databus_ready),
      .m_databus_valid(m_databus_valid),
      .m_databus_addr (m_databus_addr),
      .m_databus_rdata(m_databus_rdata),
      .m_databus_wdata(m_databus_wdata),
      .m_databus_wstrb(m_databus_wstrb),
      .m_databus_len  (m_databus_len),
      .m_databus_last (m_databus_last),
`endif

`ifdef EXTERNAL_PORTS
      .in0 (in0_i),
      .in1 (in1_i),
      .out0(out0_o),
`endif

      .clk(clk_i),
      .rst(arst_i)
   );

endmodule  // iob_versat

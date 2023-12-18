`timescale 1ns / 1ps
// Comment so that verible-format will not put timescale and defaultt_nettype into same line
`default_nettype none

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
   `include "axi_m_port.vs"
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

      `include "axi_m_m_write_portmap.vs"
      `include "axi_m_m_read_portmap.vs"

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
      .valid(iob_avalid_i),
      .wstrb(iob_wstrb_i),
      .addr(iob_addr_i >> 2), // Address is now byte space so for now simple hack it into the dword space previously used
      .wdata(iob_wdata_i),
      .rdata(iob_rdata_o),
      .ready(iob_rvalid_o),

`ifdef VERSAT_EXTERNAL_MEMORY
      `include "versat_external_memory_portmap.vh"
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

`default_nettype wire

`timescale 1ns/1ps
`include "axi.vh"
`include "system.vh"
`include "iob_lib.vh"
`include "iob_intercon.vh"
`include "iob_versat.vh"

`include "versat_defs.vh"

`default_nettype none
module iob_versat
  # (//the below parameters are used in cpu if includes below
    	parameter AXI_ADDR_W = 32,
      parameter AXI_DATA_W = 32,
      parameter AXI_ID_W = 1,
      parameter ADDR_W = `VERSAT_ADDR_W, //NODOC Address width
    	parameter DATA_W = `VERSAT_RDATA_W, //NODOC CPU data width
    	parameter WDATA_W = `VERSAT_WDATA_W //NODOC CPU data width
    )
  	(
 		`include "iob_s_if.vh"

    `ifdef VERSAT_EXTERNAL_MEMORY
    `include "versat_external_memory_port.vh"
    `endif

   `ifdef VERSAT_IO
      `include "m_versat_axi_m_port.vh"
   `endif

   `ifdef EXTERNAL_PORTS
   input [31:0]              in0,
   input [31:0]              in1,
   output [31:0]             out0,
   `endif

   input clk,
   input rst
	);

localparam LEN_W = `LEN_W;
localparam IO = `nIO;

`ifdef VERSAT_IO
wire [IO-1:0]                m_databus_ready;
wire [IO-1:0]                m_databus_valid;
wire [IO*AXI_ADDR_W-1:0]     m_databus_addr;
wire [AXI_DATA_W-1:0]        m_databus_rdata;
wire [IO*AXI_DATA_W-1:0]     m_databus_wdata;
wire [IO*(AXI_DATA_W/8)-1:0] m_databus_wstrb;
wire [IO*LEN_W-1:0]          m_databus_len;
wire [IO-1:0]                m_databus_last;

wire w_ready,w_valid;
wire [AXI_ADDR_W-1:0]   w_addr;
wire [AXI_DATA_W-1:0]   w_data;
wire [AXI_DATA_W/8-1:0] w_strb;
wire [LEN_W-1:0]        w_len;

wire r_ready,r_valid;
wire [AXI_ADDR_W-1:0]  r_addr;
wire [AXI_DATA_W-1:0]  r_data;
wire [LEN_W-1:0]       r_len;

wire w_last,r_last;

// TODO: To improve performance in later stages, it would be helpful to further separate into current master being served and next master to be served.
//       That way, the SimpleAXItoAXI module could be changed to pipeline the transfer calculations for the next master while servicing the current master.
xmerge #(.N_SLAVES(IO),.ADDR_W(AXI_ADDR_W),.DATA_W(AXI_DATA_W),.LEN_W(LEN_W)) merge(
  .s_valid(m_databus_valid),
  .s_ready(m_databus_ready),
  .s_addr(m_databus_addr),
  .s_wdata(m_databus_wdata),
  .s_wstrb(m_databus_wstrb),
  .s_rdata(m_databus_rdata),
  .s_len(m_databus_len),
  .s_last(m_databus_last),
  
  .m_wvalid(w_valid),
  .m_wready(w_ready),
  .m_waddr(w_addr),
  .m_wdata(w_data),
  .m_wstrb(w_strb),
  .m_wlen(w_len),
  .m_wlast(w_last),

  .m_rvalid(r_valid),
  .m_rready(r_ready),
  .m_raddr(r_addr),
  .m_rdata(r_data),
  .m_rlen(r_len),
  .m_rlast(r_last),

  .clk(clk),
  .rst(rst)
);

SimpleAXItoAXI #(
    .AXI_ADDR_W(AXI_ADDR_W),
    .AXI_DATA_W(AXI_DATA_W),
    .AXI_ID_W(AXI_ID_W),
    .LEN_W(LEN_W)
  ) simpleToAxi(
  .m_wvalid(w_valid),
  .m_wready(w_ready),
  .m_waddr(w_addr),
  .m_wdata(w_data),
  .m_wstrb(w_strb),
  .m_wlen(w_len),
  .m_wlast(w_last),
  
  .m_rvalid(r_valid),
  .m_rready(r_ready),
  .m_raddr(r_addr),
  .m_rdata(r_data),
  .m_rlen(r_len),
  .m_rlast(r_last),

  `include "m_versat_axi_read_portmap.vh"
  `include "m_versat_axi_write_portmap.vh"

  .clk(clk),
  .rst(rst)
);

`endif

versat_instance #(.ADDR_W(ADDR_W),.DATA_W(DATA_W),.AXI_DATA_W(AXI_DATA_W),.LEN_W(LEN_W)) xversat(
      .valid(valid),
      .wstrb(wstrb),
      .addr(address),
      .rdata(rdata),
      .wdata(wdata),
      .ready(ready),

`ifdef VERSAT_EXTERNAL_MEMORY
      `include "versat_external_memory_portmap.vh"
`endif

`ifdef VERSAT_IO
      .m_databus_ready(m_databus_ready),
      .m_databus_valid(m_databus_valid),
      .m_databus_addr(m_databus_addr),
      .m_databus_rdata(m_databus_rdata),
      .m_databus_wdata(m_databus_wdata),
      .m_databus_wstrb(m_databus_wstrb),
      .m_databus_len(m_databus_len),
      .m_databus_last(m_databus_last),
`endif

`ifdef EXTERNAL_PORTS
      .in0(in0),
      .in1(in1),
      .out0(out0),
`endif

      .clk(clk),
      .rst(rst)
); 

endmodule

`ifdef VERSAT_IO // Easier to just remove everything from consideration

`endif // ifdef VERSAT_IO

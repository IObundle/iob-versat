`timescale 1ns / 1ps
`include "xversat.vh"
`include "xdefs.vh"

`include "versat_defs.vh"

module versat_instance(
   // Databus master interface
`ifdef IO
   input [`nIO-1:0]                m_databus_ready,
   output [`nIO-1:0]               m_databus_valid,
   output [`nIO*`IO_ADDR_W-1:0]    m_databus_addr,
   input [`nIO*`DATAPATH_W-1:0]    m_databus_rdata,
   output [`nIO*`DATAPATH_W-1:0]   m_databus_wdata,
   output [`nIO*`DATAPATH_W/8-1:0] m_databus_wstrb,
`endif
   // data/control interface
   input                           valid,
   input [`ADDR_W-1:0]             addr,
   input                           we,
   input [`DATA_W-1:0]             rdata,
   output reg                      ready,
   output reg [`DATA_W-1:0]        wdata,

   input [`CONFIG_W-1:0]           configdata,

   input                           clk,
   input                           rst
   );



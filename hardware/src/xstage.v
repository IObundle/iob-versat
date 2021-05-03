`timescale 1ns / 1ps

// top defines
`include "xversat.vh"
`include "xconfdefs.vh"

// FU defines
`include "xmemdefs.vh"
`include "versat-io.vh"
`include "xaludefs.vh"
`include "xalulitedefs.vh"
`include "xmuldefs.vh"
`include "xmuladddefs.vh"
`include "xbsdefs.vh"

module xstage # (
                 parameter			DATA_W = 32
                 )
   (
    input                           clk,
    input                           rst,

    // data/control interface
    input                           valid,
    input                           we,
    input [`BASE_ADDR_W+1:0]        addr,
    input [DATA_W-1:0]              rdata,
    output [DATA_W-1:0]             wdata,
`ifdef IO
    // Databus master interface
    input [`nIO-1:0]                m_databus_ready,
    output [`nIO-1:0]               m_databus_valid,
    output [`nIO*`IO_ADDR_W-1:0]    m_databus_addr,
    input [`nIO*`DATAPATH_W-1:0]    m_databus_rdata,
    output [`nIO*`DATAPATH_W-1:0]   m_databus_wdata,
    output [`nIO*`DATAPATH_W/8-1:0] m_databus_wstrb,
`endif
    // versat flow interface
    input [`DATABUS_W-1:0]          flow_in,
    output [`DATABUS_W-1:0]         flow_out
    );

   // simple address decode
   wire                       		conf_valid = addr[`BASE_ADDR_W+1] & valid;
   wire                             eng_valid = ~addr[`BASE_ADDR_W+1] & valid;

   // configuration bus from conf module to versat
   wire [`CONF_BITS-1:0]            config_bus;
   
   // configuration module
   xconf # (
	        .DATA_W(DATA_W)
            )
   conf (
	     .clk(clk),
	     .rst(rst),

	     // Control bus interface
	     .ctr_valid(conf_valid),
	     .ctr_we(we),
	     .ctr_addr(addr[`CONF_REG_ADDR_W:0]),
	     .ctr_data_in(rdata[`IO_ADDR_W-1:0]),

	     // configuration for engine
    	 .conf_out(config_bus)
	     );

   // data engine
   xdata_eng # (
	            .DATA_W(DATA_W)
                )
   data_eng (
	         .clk(clk),
	         .rst(rst),

	         // data/control interface
	         .valid(eng_valid),
             .we(we),
	         .addr(addr[`nMEM_W+`MEM_ADDR_W:0]),
	         .rdata(rdata),
	         .wdata(wdata),
`ifdef IO
		     // Databus master interface
 		     .m_databus_ready(m_databus_ready),
 		     .m_databus_valid(m_databus_valid),
 		     .m_databus_addr(m_databus_addr),
		     .m_databus_rdata(m_databus_rdata),
	 	     .m_databus_wdata(m_databus_wdata),
             .m_databus_wstrb(m_databus_wstrb),
`endif
             // flow interface
             .flow_in(flow_in),
             .flow_out(flow_out),

             // configuration bus
	         .config_bus(config_bus)
	         );

endmodule

`timescale 1ns / 1ps

// Top defines
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

module xversat # (
		          parameter  ADDR_W = 32,
		          parameter  DATA_W = 16
	 	          )
   (
    input                                clk,
    input                                rst,
`ifdef IO
    // Databus master interface
    input [`nSTAGE*`nIO-1:0]             m_databus_ready,
    output [`nSTAGE*`nIO-1:0]            m_databus_valid,
    output [`nSTAGE*`nIO*`IO_ADDR_W-1:0] m_databus_addr,
    input [`nSTAGE*`nIO*DATA_W-1:0]      m_databus_rdata,
    output [`nSTAGE*`nIO*DATA_W-1:0]     m_databus_wdata,
    output [`nSTAGE*`nIO*DATA_W/8-1:0]   m_databus_wstrb,
`endif
    // data/control interface
    input                                valid,
    input [ADDR_W-1:0]                   addr,
    input                                we,
    input [2*DATA_W-1:0]                 rdata,
	output reg                           ready,
    output reg [DATA_W-1:0]              wdata
    );

   // interface ready signal
   reg ready_r0, ready_r1;
   always @(posedge clk, posedge rst) begin
      if(rst) begin
         ready_r0 <= 1'b0;
 	     ready_r1 <= 1'b0;
         ready <= 1'b0;
      end else begin 
         ready_r0 <= valid;
         ready_r1 <= ready_r0;
         if ((addr[1+`nMEM_W+`MEM_ADDR_W -: 2] == 2'b0) && !we)
           ready <= ready_r1; //read mem
         else
           ready <= valid;
      end
   end

   // data buses for each versat
   wire [`DATABUS_W-1:0] stage_databus [`nSTAGE:0];

   // global operations addr
   wire run_done = ~addr[1+`nMEM_W+`MEM_ADDR_W] & addr[`nMEM_W+`MEM_ADDR_W];
   wire global_conf_clear = addr[1+`nMEM_W+`MEM_ADDR_W] & (addr[`CONF_REG_ADDR_W:0] == `GLOBAL_CONF_CLEAR);

   // select stage address
   wire [`CTR_ADDR_W-`nSTAGE_W-1:0] stage_addr = {addr[`CTR_ADDR_W-`nSTAGE_W-1:1], addr[0] & ~global_conf_clear};

   //
   // ADDRESS DECODER
   //

   // select stage(s)
   reg [`nSTAGE-1:0] stage_valid;
   always @ * begin
     integer j;
     stage_valid = {`nSTAGE{1'b0}};
       for (j=0; j<`nSTAGE; j=j+1)
          if (addr[`CTR_ADDR_W-1 -: `nSTAGE_W] == j[`nSTAGE_W-1:0] || run_done || global_conf_clear)
            stage_valid[j] = valid;
   end 

   // check done in all stages
   reg [`nSTAGE-1:0] done;
   reg [DATA_W-1:0] stage_wdata [`nSTAGE-1:0];
   always @ * begin
     integer j;
     for (j = 0; j < `nSTAGE; j++)
       done[j] = stage_wdata[j][0];
   end

   // select stage data
   always @ * begin
      integer j;
      wdata = {DATA_W{1'b0}};
      for (j=0; j<`nSTAGE; j=j+1) begin
         if (run_done)
           wdata = {{DATA_W-1{1'b0}}, &done};       
         else if (stage_valid[j])
           wdata = stage_wdata[j];
      end
   end

   //
   // INSTANTIATE THE VERSAT STAGES
   //
   
   genvar i;
   generate
      for (i=0; i < `nSTAGE; i=i+1) begin : stage_array
         xstage # (
		           .DATA_W(DATA_W)
	               )
         stage (
                .clk(clk),
                .rst(rst),

                // data/control interface
                .valid(stage_valid[i]),
                .we(we),
                .addr(stage_addr),
                .rdata(rdata),
                .wdata(stage_wdata[i]),
`ifdef IO
                // Databus master interface
                .m_databus_ready(m_databus_ready[`nSTAGE*`nIO-1 -i*`nIO -: `nIO]),
                .m_databus_valid(m_databus_valid[`nSTAGE*`nIO-1 -i*`nIO -: `nIO]),
                .m_databus_addr(m_databus_addr[`nSTAGE*`nIO*`IO_ADDR_W-1 -i*`nIO*`IO_ADDR_W -: `nIO*`IO_ADDR_W]),
                .m_databus_rdata(m_databus_rdata[`nSTAGE*`nIO*DATA_W-1 -i*`nIO*DATA_W -: `nIO*DATA_W]),
                .m_databus_wdata(m_databus_wdata[`nSTAGE*`nIO*DATA_W-1 -i*`nIO*DATA_W -: `nIO*DATA_W]),
                .m_databus_wstrb (m_databus_wstrb[`nSTAGE*`nIO*DATA_W/8-1 -i*`nIO*DATA_W/8 -: `nIO*DATA_W/8]),
`endif
                // flow interface
                .flow_in(stage_databus[i]),
                .flow_out(stage_databus[i+1])
                );
      end
    endgenerate

   // connect last stage back to first stage: ring topology
   assign stage_databus[0] = stage_databus [`nSTAGE];     

endmodule

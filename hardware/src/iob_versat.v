`timescale 1ns/1ps
`include "axi.vh"
`include "system.vh"
`include "iob_lib.vh"
`include "iob_intercon.vh"
`include "iob_versat.vh"

`include "versat_defs.vh"

module iob_versat
  # (//the below parameters are used in cpu if includes below
    	parameter AXI_ADDR_W = 32,
      parameter AXI_DATA_W = 32,
      parameter ADDR_W = `VERSAT_ADDR_W, //NODOC Address width
    	parameter DATA_W = `VERSAT_RDATA_W, //NODOC CPU data width
    	parameter WDATA_W = `VERSAT_WDATA_W //NODOC CPU data width
    )
  	(
 		`include "iob_s_if.vh"

   `ifdef VERSAT_IO
      `include "cpu_axi_m_port.vh"
   `endif

   input clk,
   input rst
	);

`ifdef VERSAT_IO
   wire [`nIO-1:0]               m_databus_ready;
   wire [`nIO-1:0]               m_databus_valid;
   wire [`nIO*`IO_ADDR_W-1:0]    m_databus_addr;
   wire [`nIO*`DATAPATH_W-1:0]   m_databus_rdata;
   wire [`nIO*`DATAPATH_W-1:0]   m_databus_wdata;
   wire [`nIO*`DATAPATH_W/8-1:0] m_databus_wstrb;

   wire dma_ready,dma_error;

   wire [`nIO*`REQ_W-1:0] m_req;
   wire [`nIO*`RESP_W-1:0] m_resp;

   wire [`REQ_W-1:0] s_req;
   wire [`RESP_W-1:0] s_resp;

   wire s_valid = s_req[`valid(0)];
   wire [ADDR_W-1:0] s_addr = s_req[`address(0,ADDR_W)];
   wire [DATA_W-1:0] s_wdata = s_req[`wdata(0)];
   wire [DATA_W/8-1:0] s_wstrb = s_req[`wstrb(0)];
   
   wire [DATA_W-1:0] s_rdata;
   wire s_ready;

   assign s_resp[`rdata(0)] = s_rdata;
   assign s_resp[`ready(0)] = s_ready;

   generate
   genvar i;
      for(i = 0; i < `nIO; i = i + 1)
      begin
         assign m_req[`valid(i)] = m_databus_valid[i];
         assign m_req[`address(i, 26)] = m_databus_addr[i*32 +: 32];
         assign m_req[`wstrb(i)] = m_databus_wstrb[i*`WSTRB_W_(DATA_W) +: `WSTRB_W_(DATA_W)];
         assign m_req[`wdata(i)] = m_databus_wdata[i*DATA_W +: DATA_W];

         assign m_databus_rdata[i*DATA_W +: DATA_W] = m_resp[`rdata_(i,DATA_W)];
         assign m_databus_ready[i] = m_resp[`ready_(i,DATA_W)];
      end
   endgenerate

   xmerge #(.N_MASTERS(`nIO),.DATA_W(DATA_W),.ADDR_W(ADDR_W)) io_merge
   (
    .clk(clk),
    .rst(rst),

    //masters interface
    .m_req(m_req),
    .m_resp(m_resp),

    //slave interface
    .s_req(s_req),
    .s_resp(s_resp)
    );

   SimpleIOBToAxi iobToAxi(
    //
    // Native Slave I/F
    //
    .s_valid(s_valid),
    .s_addr(s_addr),
    .s_wdata(s_wdata),
    .s_wstrb(s_wstrb),
    .s_rdata(s_rdata),
    .s_ready(s_ready),
  
    // Address write
    .m_axi_awid(m_axi_awid), 
    .m_axi_awaddr(m_axi_awaddr), 
    .m_axi_awlen(m_axi_awlen), 
    .m_axi_awsize(m_axi_awsize), 
    .m_axi_awburst(m_axi_awburst), 
    .m_axi_awlock(m_axi_awlock), 
    .m_axi_awcache(m_axi_awcache), 
    .m_axi_awprot(m_axi_awprot),
    .m_axi_awqos(m_axi_awqos), 
    .m_axi_awvalid(m_axi_awvalid), 
    .m_axi_awready(m_axi_awready),
    //write
    .m_axi_wdata(m_axi_wdata), 
    .m_axi_wstrb(m_axi_wstrb), 
    .m_axi_wlast(m_axi_wlast), 
    .m_axi_wvalid(m_axi_wvalid), 
    .m_axi_wready(m_axi_wready), 
    //write response
    .m_axi_bid(m_axi_bid), 
    .m_axi_bresp(m_axi_bresp), 
    .m_axi_bvalid(m_axi_bvalid), 
    .m_axi_bready(m_axi_bready),
    //address read
    .m_axi_arid(m_axi_arid), 
    .m_axi_araddr(m_axi_araddr), 
    .m_axi_arlen(m_axi_arlen), 
    .m_axi_arsize(m_axi_arsize), 
    .m_axi_arburst(m_axi_arburst), 
    .m_axi_arlock(m_axi_arlock), 
    .m_axi_arcache(m_axi_arcache), 
    .m_axi_arprot(m_axi_arprot), 
    .m_axi_arqos(m_axi_arqos), 
    .m_axi_arvalid(m_axi_arvalid), 
    .m_axi_arready(m_axi_arready), 
    //read 
    .m_axi_rid(m_axi_rid), 
    .m_axi_rdata(m_axi_rdata), 
    .m_axi_rresp(m_axi_rresp), 
    .m_axi_rlast(m_axi_rlast), 
    .m_axi_rvalid(m_axi_rvalid),  
    .m_axi_rready(m_axi_rready),

    .clk(clk),
    .rst(rst)
  );

/*
   iob2axi #(.ADDR_W(ADDR_W),.DATA_W(DATA_W)) iob_to_axi(
    .run(1'b0),
    .direction(|s_wstrb), // 0 for reading, 1 for writing
    .addr(0),
    .ready(),
    .error(),

    //
    // Native Slave I/F
    //
    .s_valid(s_valid),
    .s_addr(s_addr),
    .s_wdata(s_wdata),
    .s_wstrb(s_wstrb),
    .s_rdata(s_rdata),
    .s_ready(s_ready),
  
    // Address write
    .m_axi_awid(m_axi_awid), 
    .m_axi_awaddr(m_axi_awaddr), 
    .m_axi_awlen(m_axi_awlen), 
    .m_axi_awsize(m_axi_awsize), 
    .m_axi_awburst(m_axi_awburst), 
    .m_axi_awlock(m_axi_awlock), 
    .m_axi_awcache(m_axi_awcache), 
    .m_axi_awprot(m_axi_awprot),
    .m_axi_awqos(m_axi_awqos), 
    .m_axi_awvalid(m_axi_awvalid), 
    .m_axi_awready(m_axi_awready),
    //write
    .m_axi_wdata(m_axi_wdata), 
    .m_axi_wstrb(m_axi_wstrb), 
    .m_axi_wlast(m_axi_wlast), 
    .m_axi_wvalid(m_axi_wvalid), 
    .m_axi_wready(m_axi_wready), 
    //write response
    .m_axi_bid(m_axi_bid), 
    .m_axi_bresp(m_axi_bresp), 
    .m_axi_bvalid(m_axi_bvalid), 
    .m_axi_bready(m_axi_bready),
    //address read
    .m_axi_arid(m_axi_arid), 
    .m_axi_araddr(m_axi_araddr), 
    .m_axi_arlen(m_axi_arlen), 
    .m_axi_arsize(m_axi_arsize), 
    .m_axi_arburst(m_axi_arburst), 
    .m_axi_arlock(m_axi_arlock), 
    .m_axi_arcache(m_axi_arcache), 
    .m_axi_arprot(m_axi_arprot), 
    .m_axi_arqos(m_axi_arqos), 
    .m_axi_arvalid(m_axi_arvalid), 
    .m_axi_arready(m_axi_arready), 
    //read 
    .m_axi_rid(m_axi_rid), 
    .m_axi_rdata(m_axi_rdata), 
    .m_axi_rresp(m_axi_rresp), 
    .m_axi_rlast(m_axi_rlast), 
    .m_axi_rvalid(m_axi_rvalid),  
    .m_axi_rready(m_axi_rready),

    .clk(clk),
    .rst(rst)
  );
*/

/*
   dma_axi #(.DMA_DATA_W(DATA_W),.AXI_DATA_W(AXI_DATA_W),.AXI_ADDR_W(AXI_ADDR_W)) dma(
      .clk(clk),
      .rst(rst),

      // Native i/f - can't include from INTERCON because address_w = 6
      .valid(s_valid),
      .address(s_addr),
      .wdata(s_wdata),
      .wstrb(s_wstrb),
      .rdata(s_rdata),
      .ready(s_ready),

      // DMA signals
      .dma_len(8'h10),
      .dma_ready(dma_ready),
      .error(dma_error),
      
      //AXI Interface
      // Address write
      .m_axi_awid(m_axi_awid), 
      .m_axi_awaddr(m_axi_awaddr), 
      .m_axi_awlen(m_axi_awlen), 
      .m_axi_awsize(m_axi_awsize), 
      .m_axi_awburst(m_axi_awburst), 
      .m_axi_awlock(m_axi_awlock), 
      .m_axi_awcache(m_axi_awcache), 
      .m_axi_awprot(m_axi_awprot),
      .m_axi_awqos(m_axi_awqos), 
      .m_axi_awvalid(m_axi_awvalid), 
      .m_axi_awready(m_axi_awready),
      //write
      .m_axi_wdata(m_axi_wdata), 
      .m_axi_wstrb(m_axi_wstrb), 
      .m_axi_wlast(m_axi_wlast), 
      .m_axi_wvalid(m_axi_wvalid), 
      .m_axi_wready(m_axi_wready), 
      //write response
      .m_axi_bid(m_axi_bid), 
      .m_axi_bresp(m_axi_bresp), 
      .m_axi_bvalid(m_axi_bvalid), 
      .m_axi_bready(m_axi_bready),
      //address read
      .m_axi_arid(m_axi_arid), 
      .m_axi_araddr(m_axi_araddr), 
      .m_axi_arlen(m_axi_arlen), 
      .m_axi_arsize(m_axi_arsize), 
      .m_axi_arburst(m_axi_arburst), 
      .m_axi_arlock(m_axi_arlock), 
      .m_axi_arcache(m_axi_arcache), 
      .m_axi_arprot(m_axi_arprot), 
      .m_axi_arqos(m_axi_arqos), 
      .m_axi_arvalid(m_axi_arvalid), 
      .m_axi_arready(m_axi_arready), 
      //read 
      .m_axi_rid(m_axi_rid), 
      .m_axi_rdata(m_axi_rdata), 
      .m_axi_rresp(m_axi_rresp), 
      .m_axi_rlast(m_axi_rlast), 
      .m_axi_rvalid(m_axi_rvalid),  
      .m_axi_rready(m_axi_rready)
      );
*/

`endif

versat_instance #(.ADDR_W(ADDR_W),.DATA_W(DATA_W)) xversat(
      .valid(valid),
      .wstrb(wstrb),
      .addr(address),
      .rdata(rdata),
      .wdata(wdata),
      .ready(ready),

`ifdef VERSAT_IO
      .m_databus_ready(m_databus_ready),
      .m_databus_valid(m_databus_valid),
      .m_databus_addr(m_databus_addr),
      .m_databus_rdata(m_databus_rdata),
      .m_databus_wdata(m_databus_wdata),
      .m_databus_wstrb(m_databus_wstrb),
`endif

      .clk(clk),
      .rst(rst)
   ); 

endmodule

module xmerge
  #(
    parameter N_MASTERS = 2,
    parameter DATA_W = 32,
    parameter ADDR_W = 32
    )
   (

    input                              clk,
    input                              rst,

    //masters interface
    input [N_MASTERS*`REQ_W-1:0]       m_req,
    output reg [N_MASTERS*`RESP_W-1:0] m_resp,

    //slave interface
    output reg [`REQ_W-1:0]            s_req,
    input [`RESP_W-1:0]                s_resp
    );


   localparam Nb=$clog2(N_MASTERS)+($clog2(N_MASTERS)==0);
   

   //                               
   //priority encoder: most significant bus has priority   
   //
   reg [Nb-1:0]                        sel, sel_reg;
   
   wire tempA = s_req[`valid(0)];
   wire tempB = s_resp[`ready(0)];

   //select enable
   reg                                 sel_en; 
   always @(posedge clk, posedge rst)
     if(rst)
       sel_en <= 1'b1;
     else if(tempA)
       sel_en <= 1'b0;
     else if(~tempB)
       sel_en <= ~tempA;

   
   //select master
   integer                             k; 
   always @* begin
      sel = {Nb{1'b0}};
      for (k=0; k<N_MASTERS; k=k+1)
        if (~sel_en)
          sel = sel_reg;
        else if( m_req[`valid(k)] )
          sel = k[Nb-1:0];          
   end
   
   //
   //route master request to slave
   //  
   integer                             i;
   always @* begin
      s_req = {`REQ_W{1'b0}};
      for (i=0; i<N_MASTERS; i=i+1)
        if( i == sel )
          s_req = m_req[`req(i)];
   end

   //
   //route response from slave to previously selected master
   //

   //register master selection
   always @( posedge clk, posedge rst ) begin
      if( rst )
        sel_reg <= {Nb{1'b0}};
      else
        sel_reg <= sel;
   end
   
   //route
   integer                             j;
   always @* begin
      for (j=0; j<N_MASTERS; j=j+1)
        if( j == sel_reg )
          m_resp[`resp(j)] = s_resp;
        else
          m_resp[`resp(j)] = {`RESP_W{1'b0}};
   end

   
endmodule

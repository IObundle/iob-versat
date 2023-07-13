   //
   // VERSAT
   //

   `include "versat_defs.vh"

   `ifdef VERSAT_IO
      initial $display("Versat IO in use\n");
   `endif
   wire [32-1:0] versat_awaddr;
   wire [32-1:0] versat_araddr;
   wire m_axi_awlock_extra;
   wire m_axi_arlock_extra;

   iob_versat #(
       .AXI_ID_W(1)
   ) versat ( 
      // AXI4 master interface
      `ifdef VERSAT_IO
      //address write
      .m_axi_awid(m_axi_awid[1*1+:1]), 
      .m_axi_awaddr(m_axi_awaddr[1*`DDR_ADDR_W+:`DDR_ADDR_W]), 
      .m_axi_awlen(m_axi_awlen[1*8+:8]), 
      .m_axi_awsize(m_axi_awsize[1*3+:3]), 
      .m_axi_awburst(m_axi_awburst[1*2+:2]), 
      .m_axi_awlock({m_axi_awlock_extra,m_axi_awlock[1*1+:1]}), 
      .m_axi_awcache(m_axi_awcache[1*4+:4]), 
      .m_axi_awprot(m_axi_awprot[1*3+:3]),
      .m_axi_awqos(m_axi_awqos[1*4+:4]), 
      .m_axi_awvalid(m_axi_awvalid[1*1+:1]), 
      .m_axi_awready(m_axi_awready[1*1+:1]), 
      //write
      .m_axi_wdata(m_axi_wdata[1*32+:32]), 
      .m_axi_wstrb(m_axi_wstrb[1*32/8+:32/8]), 
      .m_axi_wlast(m_axi_wlast[1*1+:1]), 
      .m_axi_wvalid(m_axi_wvalid[1*1+:1]), 
      .m_axi_wready(m_axi_wready[1*1+:1]), 
      //write response
      .m_axi_bid(m_axi_bid[1*1+:1]), 
      .m_axi_bresp(m_axi_bresp[1*2+:2]), 
      .m_axi_bvalid(m_axi_bvalid[1*1+:1]), 
      .m_axi_bready(m_axi_bready[1*1+:1]), 
      //address read
      .m_axi_arid(m_axi_arid[1*1+:1]), 
      .m_axi_araddr(m_axi_araddr[1*`DDR_ADDR_W+:`DDR_ADDR_W]), 
      .m_axi_arlen(m_axi_arlen[1*8+:8]), 
      .m_axi_arsize(m_axi_arsize[1*3+:3]), 
      .m_axi_arburst(m_axi_arburst[1*2+:2]), 
      .m_axi_arlock({m_axi_arlock_extra,m_axi_arlock[1*1+:1]}), 
      .m_axi_arcache(m_axi_arcache[1*4+:4]), 
      .m_axi_arprot(m_axi_arprot[1*3+:3]), 
      .m_axi_arqos(m_axi_arqos[1*4+:4]), 
      .m_axi_arvalid(m_axi_arvalid[1*1+:1]), 
      .m_axi_arready(m_axi_arready[1*1+:1]), 
      //read 
      .m_axi_rid(m_axi_rid[1*1:+1]), 
      .m_axi_rdata(m_axi_rdata[1*32+:32]), 
      .m_axi_rresp(m_axi_rresp[1*2+:2]), 
      .m_axi_rlast(m_axi_rlast[1*1+:1]), 
      .m_axi_rvalid(m_axi_rvalid[1*1+:1]),  
      .m_axi_rready(m_axi_rready[1*1+:1]),
      `endif

`ifdef VERSAT_EXTERNAL_MEMORY
      `include "versat_external_memory_portmap.vh"
`endif

      .valid(slaves_req[`valid(`VERSAT)]),
      .address(slaves_req[`address(`VERSAT,`VERSAT_ADDR_W+2)-2]),
      .wdata(slaves_req[`wdata(`VERSAT)-(`DATA_W-`VERSAT_WDATA_W)]),
      .wstrb(slaves_req[`wstrb(`VERSAT)]),
      .rdata(slaves_resp[`rdata(`VERSAT)]),
      .ready(slaves_resp[`ready(`VERSAT)]),

      `ifdef EXTERNAL_PORTS
      .in0(versat_in0),
      .in1(versat_in1),
      .out0(versat_out),
      `endif

      //CPU interface
      .clk       (clk),
      .rst       (cpu_reset)
      );

   `ifndef VERSAT_IO
      initial $display("No Versat IO\n");

      `ifdef VERSAT_ARCH_HAS_IO
      assign m_axi_awid[1*1+:1] = 1'b0;
      assign m_axi_awaddr[1*`DDR_ADDR_W+:`DDR_ADDR_W] = 0;
      assign m_axi_awlen[1*8+:8] = 0;
      assign m_axi_awsize[1*3+:3] = 0;
      assign m_axi_awburst[1*2+:2] = 0;
      assign m_axi_awlock[1*1+:1] = 0;
      assign m_axi_awcache[1*4+:4] = 0;
      assign m_axi_awprot[1*3+:3] = 0;
      assign m_axi_awqos[1*4+:4] = 0;
      assign m_axi_awvalid[1*1+:1] = 0; // 

      assign m_axi_wdata[1*32+:32] = 0;
      assign m_axi_wstrb[1*32/8+:32/8] = 0;
      assign m_axi_wlast[1*1+:1] = 0;
      assign m_axi_wvalid[1*1+:1] = 0; //

      assign m_axi_bready[1*1+:1] = 1'b1; // To prevent lookup, "ready" for everything 

      assign m_axi_arid[1*1+:1] = 0;
      assign m_axi_araddr[1*`DDR_ADDR_W+:`DDR_ADDR_W] = 0;
      assign m_axi_arlen[1*8+:8] = 0;
      assign m_axi_arsize[1*3+:3] = 0;
      assign m_axi_arburst[1*2+:2] = 0;
      assign m_axi_arlock[1*2+:2] = 0;
      assign m_axi_arcache[1*4+:4] = 0;
      assign m_axi_arprot[1*3+:3] = 0;
      assign m_axi_arqos[1*4+:4] = 0;
      assign m_axi_arvalid[1*1+:1] = 0; //

      assign m_axi_rready[1*1+:1] = 1'b1; // To prevent lookup, "ready" for everything
      `endif // VERSAT_ARCH_HAS_IO
   `endif // VERSAT_IO


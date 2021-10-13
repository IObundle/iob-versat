   //
   // VERSAT
   //

   iob_versat versat
      ( 
      //CPU interface
      .clk       (clk),
      .rst       (reset),
      .valid(slaves_req[`valid(`VERSAT)]),
      .address(slaves_req[`address(`VERSAT,`VERSAT_ADDR_W+2)-2]),
      .wdata(slaves_req[`wdata(`VERSAT)-(`DATA_W-`VERSAT_WDATA_W)]),
      .wstrb(slaves_req[`wstrb(`VERSAT)]),
      .rdata(slaves_resp[`rdata(`VERSAT)]),
      .ready(slaves_resp[`ready(`VERSAT)])
      );

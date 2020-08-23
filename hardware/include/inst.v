   //
   // VERSAT
   //

   xversat # (
	     .ADDR_W (ADDR_W-2)
	     ) versat (
      .clk (clk),
      .rst (reset),
      
      //cpu interface
      .valid (slaves_req[`valid(`VERSAT)]),
      .addr  (slaves_req[`address(`VERSAT, ADDR_W)-2]),
      .rdata (slaves_req[`wdata(`VERSAT)]),
      .we    (|slaves_req[`wstrb(`VERSAT)]),
      .wdata (slaves_resp[`rdata(`VERSAT)]),
      .ready (slaves_resp[`ready(`VERSAT)])
      );

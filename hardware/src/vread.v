`timescale 1ns / 1ps

`include "xdefs.vh"
`include "xversat.vh"
`include "xmemdefs.vh"
`include "versat-io.vh"

module vread #(
               parameter DATA_W=32
               )
   (
   input                       clk,
   input                       rst,

    input                       run,
    output                      done,

    // Native interface
    input                       databus_ready,
    output                      databus_valid,
    output [`IO_ADDR_W-1:0]     databus_addr,
    input [DATA_W-1:0]          databus_rdata,
    output [DATA_W-1:0]         databus_wdata,
    output [DATA_W/8-1:0]       databus_wstrb,

    // input / output data
    output [DATA_W-1:0]         out0,

    // configurations
    input [`VI_CONFIG_BITS-1:0] configdata
   );

   // output databus
   wire [DATA_W-1:0]            outB;
   wire doneA,doneB;
   assign out0 = outB;
   assign done = doneA & doneB;

   function [`MEM_ADDR_W-1:0] reverseBits;
      input [`MEM_ADDR_W-1:0]   word;
      integer                   i;

      begin
        for (i=0; i < `MEM_ADDR_W; i=i+1)
          reverseBits[i] = word[`MEM_ADDR_W-1 - i];
      end
   endfunction

   // unpack configuration bits
   wire [`IO_ADDR_W-1:0]  ext_addr  = configdata[`VI_CONFIG_BITS-1 -: `IO_ADDR_W];
   wire [`MEM_ADDR_W-1:0] int_addr  = configdata[`VI_CONFIG_BITS-`IO_ADDR_W-1 -: `MEM_ADDR_W];
   wire [`IO_SIZE_W-1:0]  size      = configdata[`VI_CONFIG_BITS-`IO_ADDR_W-`MEM_ADDR_W-1 -: `IO_SIZE_W];
   wire [1:0]             direction = 2'b01;
   wire [`MEM_ADDR_W-1:0] iterA     = configdata[`VI_CONFIG_BITS-`IO_ADDR_W-`IO_SIZE_W-`MEM_ADDR_W-1 -: `MEM_ADDR_W];
   wire [`PERIOD_W-1:0]   perA      = configdata[`VI_CONFIG_BITS-`IO_ADDR_W-`IO_SIZE_W-2*`MEM_ADDR_W-1 -: `PERIOD_W];
   wire [`PERIOD_W-1:0]   dutyA     = configdata[`VI_CONFIG_BITS-`IO_ADDR_W-`IO_SIZE_W-2*`MEM_ADDR_W-`PERIOD_W-1 -: `PERIOD_W];
   wire [`MEM_ADDR_W-1:0] startA    = `MEM_ADDR_W'd0;
   wire [`MEM_ADDR_W-1:0] shiftA    = configdata[`VI_CONFIG_BITS-`IO_ADDR_W-`IO_SIZE_W-2*`MEM_ADDR_W-2*`PERIOD_W-1 -: `MEM_ADDR_W];
   wire [`MEM_ADDR_W-1:0] incrA     = configdata[`VI_CONFIG_BITS-`IO_ADDR_W-`IO_SIZE_W-3*`MEM_ADDR_W-2*`PERIOD_W-1 -: `MEM_ADDR_W];
   wire [`PERIOD_W-1:0]   delayA    = `PERIOD_W'd0;

   wire [`MEM_ADDR_W-1:0] iterB     = configdata[`VI_MEMP_CONF_BITS-1 -: `MEM_ADDR_W];
   wire [`PERIOD_W-1:0]   perB      = configdata[`VI_MEMP_CONF_BITS-`MEM_ADDR_W-1 -: `PERIOD_W];
   wire [`PERIOD_W-1:0]   dutyB     = configdata[`VI_MEMP_CONF_BITS-`MEM_ADDR_W-`PERIOD_W-1 -: `PERIOD_W];
   wire [`MEM_ADDR_W-1:0] startB    = configdata[`VI_MEMP_CONF_BITS-`MEM_ADDR_W-2*`PERIOD_W-1 -: `MEM_ADDR_W];
   wire [`MEM_ADDR_W-1:0] shiftB    = configdata[`VI_MEMP_CONF_BITS-`MEM_ADDR_W-2*`PERIOD_W-`MEM_ADDR_W-1 -: `MEM_ADDR_W];
   wire [`MEM_ADDR_W-1:0] incrB     = configdata[`VI_MEMP_CONF_BITS-`MEM_ADDR_W-2*`PERIOD_W-2*`MEM_ADDR_W-1 -: `MEM_ADDR_W];
   wire [`PERIOD_W-1:0]   delayB    = configdata[`VI_MEMP_CONF_BITS-`MEM_ADDR_W-2*`PERIOD_W-3*`MEM_ADDR_W-1 -: `PERIOD_W];
   wire                   reverseB  = configdata[`VI_MEMP_CONF_BITS-`MEM_ADDR_W-2*`PERIOD_W-3*`MEM_ADDR_W-`PERIOD_W-1 -: 1];
   wire                   extB      = configdata[`VI_MEMP_CONF_BITS-`MEM_ADDR_W-2*`PERIOD_W-3*`MEM_ADDR_W-`PERIOD_W-1-1 -: 1];
   wire [`MEM_ADDR_W-1:0] iter2B    = configdata[`VI_MEMP_CONF_BITS-4*`MEM_ADDR_W-3*`PERIOD_W-2-1 -: `MEM_ADDR_W];
   wire [`PERIOD_W-1:0]   per2B     = configdata[`VI_MEMP_CONF_BITS-5*`MEM_ADDR_W-3*`PERIOD_W-2-1 -: `PERIOD_W];
   wire [`MEM_ADDR_W-1:0] shift2B   = configdata[`VI_MEMP_CONF_BITS-5*`MEM_ADDR_W-4*`PERIOD_W-2-1 -: `MEM_ADDR_W];
   wire [`MEM_ADDR_W-1:0] incr2B    = configdata[`VI_MEMP_CONF_BITS-6*`MEM_ADDR_W-4*`PERIOD_W-2-1 -: `MEM_ADDR_W];

   // mem enables output by addr gen
   wire enA = req;
   wire enB;

   // write enables
   wire wrA = req & ~rnw;

   // port addresses and enables
   wire [`MEM_ADDR_W-1:0] addrA, addrA_int, addrA_int2;
   wire [`MEM_ADDR_W-1:0] addrB, addrB_int, addrB_int2;

   // data inputs
   wire [DATA_W-1:0]      inA;

   wire                   req;
   wire                   rnw;
   wire [DATA_W-1:0]      data_in = 0;

   wire [DATA_W-1:0]      data_to_wrA = inA;

   // address generators
   ext_addrgen #(
                 .DATA_W(DATA_W)
                 )
   addrgenA (
            .clk(clk),
            .rst(rst),

            // Control
            .run(run),
            .done(doneA),

            // Configuration
            .ext_addr(ext_addr),
            .int_addr(int_addr),
            .size(size),
            .direction(direction),
            .iterations(iterA),
            .period(perA),
            .duty(dutyA),
            .start(startA),
            .shift(shiftA),
            .incr(incrA),
            .delay(delayA),

            // Databus interface
            .databus_ready(databus_ready),
            .databus_valid(databus_valid),
            .databus_addr(databus_addr),
            .databus_rdata(databus_rdata),
            .databus_wdata(databus_wdata),
            .databus_wstrb(databus_wstrb),

            // internal memory interface
            .req(req),
            .rnw(rnw),
            .addr(addrA_int),
            .data_out(inA),
            .data_in(data_in)
           );

    xaddrgen2 addrgen2B (
                       .clk(clk),
                       .rst(rst),
                       .run(run),
                       .iterations(iterB),
                       .period(perB),
                       .duty(dutyB),
                       .start(startB),
                       .shift(shiftB),
                       .incr(incrB),
                       .delay(delayB),
                       .iterations2(iter2B),
                       .period2(per2B),
                       .shift2(shift2B),
                       .incr2(incr2B),
                       .addr(addrB_int),
                       .mem_en(enB),
                       .done(doneB)
                       );

   assign addrA = addrA_int2;
   assign addrB = addrB_int2;

   assign addrA_int2 = addrA_int;
   assign addrB_int2 = reverseB? reverseBits(addrB_int) : addrB_int;
   
   iob_2p_ram #(
               .DATA_W(DATA_W),
               .ADDR_W(`MEM_ADDR_W)
               )
   mem (
        .clk(clk),

        // Writting port
        .w_en(enA & wrA),
        .w_addr(addrA),
        .data_in(data_to_wrA),

        // Reading port
        .r_en(enB),
        .r_addr(addrB),
        .data_out(outB)
        );

endmodule

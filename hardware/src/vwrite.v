`timescale 1ns / 1ps

`include "xdefs.vh"
`include "xversat.vh"
`include "xmemdefs.vh"
`include "versat-io.vh"

module vwrite #(
                parameter DATA_W=32
                )
   (
   input                   clk,
   input                   rst,

    input                  run,
    output                 done,

    // Databus interface
    input                  databus_ready,
    output                 databus_valid,
    output[`IO_ADDR_W-1:0] databus_addr,
    input [DATA_W-1:0]     databus_rdata,
    output [DATA_W-1:0]    databus_wdata,
    output [DATA_W/8-1:0]  databus_wstrb,

    // input / output data
    input [DATA_W-1:0]     in0,
    output [DATA_W-1:0]    out0,

    // configurations
   input [`IO_ADDR_W-1:0]  ext_addr,
   input [`MEM_ADDR_W-1:0] int_addr,
   input [`IO_SIZE_W-1:0]  size,
   input [`MEM_ADDR_W-1:0] iterA,
   input [`PERIOD_W-1:0]   perA,
   input [`PERIOD_W-1:0]   dutyA,
   input [`MEM_ADDR_W-1:0] shiftA,
   input [`MEM_ADDR_W-1:0] incrA,
   input                   pingPong,

   input [`MEM_ADDR_W-1:0] iterB,
   input [`PERIOD_W-1:0]   perB,
   input [`PERIOD_W-1:0]   dutyB,
   input [`MEM_ADDR_W-1:0] startB,
   input [`MEM_ADDR_W-1:0] shiftB,
   input [`MEM_ADDR_W-1:0] incrB,
   input [32-1:0]          delay0, // delayB
   input                   reverseB,
   input                   extB,
   input [`MEM_ADDR_W-1:0] iter2B,
   input [`PERIOD_W-1:0]   per2B,
   input [`MEM_ADDR_W-1:0] shift2B,
   input [`MEM_ADDR_W-1:0] incr2B
   );

   wire doneA,doneB;
   assign done = doneA & doneB;

   function [`MEM_ADDR_W-1:0] reverseBits;
      input [`MEM_ADDR_W-1:0]   word;
      integer                   i;

      begin
        for (i=0; i < `MEM_ADDR_W; i=i+1)
          reverseBits[i] = word[`MEM_ADDR_W-1 - i];
      end
   endfunction

   wire [`MEM_ADDR_W-1:0] startA    = `MEM_ADDR_W'd0;
   wire [1:0]             direction = 2'b10;
   wire [`PERIOD_W-1:0]   delayA    = `PERIOD_W'd0;

   // port addresses and enables
   wire [`MEM_ADDR_W-1:0] addrA, addrA_int, addrA_int2;
   wire [`MEM_ADDR_W-1:0] addrB, addrB_int, addrB_int2;

   // data inputs
   wire                   req;
   wire                   rnw;
   wire [DATA_W-1:0]      data_out;

   // mem enables output by addr gen
   wire enA = req;
   wire enB;

   // write enables
   wire wrB = (enB & ~extB); //addrgen on & input on & input isn't address

   wire [DATA_W-1:0]      data_to_wrB = in0;

   //address generators
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
            .data_out(data_out),
            .data_in(out0)
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
                       .delay(delay0[9:0]),
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

        // Reading port
        .r_en(enA),
        .r_addr(addrA),
        .r_data(out0),

        // Writing port
        .w_en(enB & wrB),
        .w_addr(addrB),
        .w_data(data_to_wrB)
        );

endmodule

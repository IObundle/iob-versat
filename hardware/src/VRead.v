`timescale 1ns / 1ps

`include "xversat.vh"
`include "xmemdefs.vh"
`include "versat-io.vh"
`include "xdefs.vh"

module VRead #(
               parameter DATA_W = 32,
               parameter ADDR_W = 10
               )
   (
   input                   clk,
   input                   rst,

    input                  run,
    output                 done,

    // Native interface
    input                  databus_ready,
    output                 databus_valid,
    output[`IO_ADDR_W-1:0] databus_addr,
    input [DATA_W-1:0]     databus_rdata,
    output [DATA_W-1:0]    databus_wdata,
    output [DATA_W/8-1:0]  databus_wstrb,

    // input / output data
    (* latency=1 *) output [DATA_W-1:0]    out0,

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
   input [32-1:0]          delay0,// delayB
   input                   reverseB,
   input                   extB,
   input [`MEM_ADDR_W-1:0] iter2B,
   input [`PERIOD_W-1:0]   per2B,
   input [`MEM_ADDR_W-1:0] shift2B,
   input [`MEM_ADDR_W-1:0] incr2B
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

   wire [1:0]             direction = 2'b01;
   wire [`MEM_ADDR_W-1:0] startA    = `MEM_ADDR_W'd0;
   wire [`PERIOD_W-1:0]   delayA    = `PERIOD_W'd0;

   // port addresses and enables
   wire [`MEM_ADDR_W-1:0] addrA, addrA_int, addrA_int2;
   wire [`MEM_ADDR_W-1:0] addrB, addrB_int, addrB_int2;

   // data inputs
   wire [DATA_W-1:0]      inA;

   wire                   req;
   wire                   rnw;
   wire [DATA_W-1:0]      data_in = 0;

   reg                    pingPongState;
   wire [ADDR_W-1:0]      int_addr_inst;
   wire [ADDR_W-1:0]      startB_inst;

   // mem enables output by addr gen
   wire enA = req;
   wire enB;

   // write enables
   wire wrA = req & ~rnw;

   wire [DATA_W-1:0]      data_to_wrA = inA;

   assign int_addr_inst = pingPong ? {pingPongState,int_addr[ADDR_W-2:0]} : int_addr;
   assign startB_inst   = pingPong ? {pingPongState,startB[ADDR_W-2:0]} : startB;

   // Ping pong 
   always @(posedge clk,posedge rst)
   begin
      if(rst)
         pingPongState <= 0;
      else if(run)
         pingPongState <= pingPong ? (!pingPongState) : 1'b0;
   end

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
            .int_addr(int_addr_inst),
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
                       .start(startB_inst),
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
   assign addrB_int2 = reverseB ? reverseBits(addrB_int) : addrB_int;
   
   iob_2p_ram #(
               .DATA_W(DATA_W),
               .ADDR_W(ADDR_W)
               )
   mem (
        .clk(clk),

        // Writting port
        .w_en(enA & wrA),
        .w_addr(addrA),
        .w_data(data_to_wrA),

        // Reading port
        .r_en(enB),
        .r_addr(addrB),
        .r_data(outB)
        );

endmodule

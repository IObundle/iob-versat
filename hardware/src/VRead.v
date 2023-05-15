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
   input                  clk,
   input                  rst,

   input                  running,
   input                  run,
   output                 done,

   // Databus interface
   input                       databus_ready,
   output                      databus_valid,
   output reg [`IO_ADDR_W-1:0] databus_addr,
   input [DATA_W-1:0]          databus_rdata,
   output [DATA_W-1:0]         databus_wdata,
   output [DATA_W/8-1:0]       databus_wstrb,
   output [7:0]                databus_len,
   input                       databus_last,

   // input / output data
   (* versat_latency = 1 *) output [DATA_W-1:0]    out0,

   // External memory
   output [ADDR_W-1:0]     ext_2p_addr_out_0,
   output [ADDR_W-1:0]     ext_2p_addr_in_0,
   output                  ext_2p_write_0,
   output                  ext_2p_read_0,
   input  [DATA_W-1:0]     ext_2p_data_in_0,
   output [DATA_W-1:0]     ext_2p_data_out_0,

   // configurations
   input [`IO_ADDR_W-1:0]  ext_addr,
   input [`MEM_ADDR_W-1:0] int_addr,
   input [`IO_SIZE_W-1:0]  size,
   input [`MEM_ADDR_W-1:0] iterA,
   input [`PERIOD_W-1:0]   perA,
   input [`PERIOD_W-1:0]   dutyA,
   input [`MEM_ADDR_W-1:0] shiftA,
   input [`MEM_ADDR_W-1:0] incrA,
   input [7:0]             length,
   input                   pingPong,

   input [`MEM_ADDR_W-1:0] iterB,
   input [`PERIOD_W-1:0]   perB,
   input [`PERIOD_W-1:0]   dutyB,
   input [`MEM_ADDR_W-1:0] startB,
   input [`MEM_ADDR_W-1:0] shiftB,
   input [`MEM_ADDR_W-1:0] incrB,
   input                   reverseB,
   input                   extB,
   input [`MEM_ADDR_W-1:0] iter2B,
   input [`PERIOD_W-1:0]   per2B,
   input [`MEM_ADDR_W-1:0] shift2B,
   input [`MEM_ADDR_W-1:0] incr2B,

   input [31:0]            delay0
   );

   assign databus_wdata = 0;
   assign databus_wstrb = 4'b0000;
   assign databus_len = length;
   
   // output databus
   wire [DATA_W-1:0]            outB;
   
   wire gen_done;
   reg doneA;
   reg doneB;
   wire doneB_int;
   assign out0 = outB;
   assign done = doneA & doneB;

   always @(posedge clk,posedge rst)
   begin
      if(rst) begin
         doneA <= 1'b1;
         doneB <= 1'b1;
      end else if(run) begin
         doneA <= 1'b0;
         doneB <= 1'b0;
      end else begin 
         if(databus_valid && databus_ready && databus_last)
            doneA <= 1'b1;
         if(doneB_int)
            doneB <= 1'b1;
      end
   end

   function [`MEM_ADDR_W-1:0] reverseBits;
      input [`MEM_ADDR_W-1:0]   word;
      integer                   i;

      begin
        for (i=0; i < `MEM_ADDR_W; i=i+1)
          reverseBits[i] = word[`MEM_ADDR_W-1 - i];
      end
   endfunction

   wire [1:0]             direction = 2'b01;
   reg [`MEM_ADDR_W-1:0] startA;

   always @*
   begin
      startA = 0;
      startA[`MEM_ADDR_W-1] = !pingPongState;
   end

   wire [31:0]   delayA    = 0;

   // port addresses and enables
   wire [`MEM_ADDR_W-1:0] addrA, addrA_int, addrA_int2;
   wire [`MEM_ADDR_W-1:0] addrB, addrB_int, addrB_int2;

   // data inputs
   wire [DATA_W-1:0]      inA;

   wire                   req;
   wire                   rnw;
   wire [DATA_W-1:0]      data_in = 0;

   reg                    pingPongState;
   wire [ADDR_W-1:0]      startB_inst = pingPong ? {pingPongState,startB[ADDR_W-2:0]} : startB;

   // mem enables output by addr gen
   wire enA = req;
   wire enB;

   // write enables
   wire wrA = req & ~rnw;

   wire [DATA_W-1:0]      data_to_wrA = inA;

   // Ping pong 
   always @(posedge clk,posedge rst)
   begin
      if(rst)
         pingPongState <= 0;
      else if(run)
         pingPongState <= pingPong ? (!pingPongState) : 1'b0;
   end

   wire next;
   wire gen_valid,gen_ready;
   wire [`MEM_ADDR_W-1:0] gen_addr;

   always @(posedge clk,posedge rst)
   begin
      if(rst)
         databus_addr <= 0;
      else if(run)
         databus_addr <= ext_addr;
   end

   MyAddressGen addrgenA(
      .clk(clk),
      .rst(rst),
      .run(run),

      //configurations 
      .iterations(iterA),
      .period(perA),
      .duty(dutyA),
      .delay(delayA),
      .start(startA),
      .shift(shiftA),
      .incr(incrA),

      //outputs 
      .valid(gen_valid),
      .ready(gen_ready),
      .addr(gen_addr),
      .done(gen_done)
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
                       .done(doneB_int)
                       );

   assign addrA = addrA_int2;
   assign addrB = addrB_int2;

   assign addrA_int2 = addrA_int;
   assign addrB_int2 = reverseB ? reverseBits(addrB_int) : addrB_int;
   
   wire write_en;
   wire [`MEM_ADDR_W-1:0] write_addr;
   wire [DATA_W-1:0] write_data;
   
   MemoryWriter #(.ADDR_W(`MEM_ADDR_W)) 
   writer(
      .gen_valid(gen_valid),
      .gen_ready(gen_ready),
      .gen_addr(gen_addr),

      // Slave connected to data source
      .data_valid(databus_ready),
      .data_ready(databus_valid),
      .data_in(databus_rdata),

      // Connect to memory
      .mem_enable(write_en),
      .mem_addr(write_addr),
      .mem_data(write_data),

      .clk(clk),
      .rst(rst)
      );

   assign ext_2p_write_0 = write_en;
   assign ext_2p_addr_out_0 = write_addr;
   assign ext_2p_data_out_0 = write_data;

   assign ext_2p_read_0 = enB;
   assign ext_2p_addr_in_0 = addrB;
   assign outB = ext_2p_data_in_0;

endmodule

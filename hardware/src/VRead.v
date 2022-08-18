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

   wire next;
   wire [`MEM_ADDR_W-1:0] addressA;

   assign databus_wstrb = 4'b0000;

   wire genDone;

   MyAddressGen addrgenA(
      .clk(clk),
      .rst(rst),
      .run(run),
      .next(next),

      //configurations 
      .iterations(iterA),
      .period(perA),
      .duty(dutyA),
      .delay(delayA),
      .start(startA),
      .shift(shiftA),
      .incr(incrA),

      //outputs 
      .valid(),
      .addr(addressA),
      .done(genDone)
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
   
   wire write_en;
   wire [`MEM_ADDR_W-1:0] write_addr;
   wire [DATA_W-1:0] write_data;


   IObToMem #(.DATA_W(DATA_W)) IobToMem(
      .ext_addr(ext_addr),

      .write_done(genDone),
      .write_addr(addressA),
      .next(next),

      .mem_en(write_en),
      .mem_addr(write_addr),
      .mem_data(write_data),

      .ready(databus_ready),
      .valid(databus_valid),
      .rdata(databus_rdata),
      .addr(databus_addr),

      .run(run),
      .done(doneA),

      .clk(clk),
      .rst(rst)
   );

   iob_2p_ram #(
               .DATA_W(DATA_W),
               .ADDR_W(ADDR_W)
               )
   mem (
        .clk(clk),

        // Writting port
        .w_en(write_en),
        .w_addr(write_addr),
        .w_data(write_data),

        // Reading port
        .r_en(enB),
        .r_addr(addrB),
        .r_data(outB)
        );

endmodule

module IObToMem #(
      parameter DATA_W = 32
   )(
      input [`IO_ADDR_W-1:0] ext_addr,

      input write_done,
      input [`MEM_ADDR_W-1:0] write_addr,
      output reg next,

      output reg mem_en,
      output reg [`MEM_ADDR_W-1:0]  mem_addr,
      output [DATA_W-1:0] mem_data,

      input                       ready,
      output reg [`IO_ADDR_W-1:0] addr,
      output reg                  valid,
      input reg [DATA_W-1:0]      rdata,

      input run,
      output reg done,

      input clk,
      input rst
   );

reg [3:0] state;
reg [31:0] counter;

always @(posedge clk,posedge rst)
begin
   if(rst) begin
      state <= 0;
      counter <= 0;
      done <= 0;
   end else if(run) begin
      state <= 4'h1;
      counter <= 0;
      done <= 0;
   end else begin
      case(state)
      4'h0: begin
         state <= 4'h0;
      end
      4'h1: begin
         state <= 4'h2;
         addr <= ext_addr + counter;
         counter <= counter + 4;
      end
      4'h2: begin
         if(ready) begin
            mem_addr <= write_addr;
            mem_data <= rdata;
            state <= 4'h3;
         end
      end
      4'h3: begin
         if(write_done) begin
            state <= 4'h0;
            done <= 1'b1;
         end else begin
            state <= 4'h1;
         end
      end
      default: begin
         state <= 4'h0;
      end
      endcase
   end
end

always @*
begin
   valid = 1'b0;
   next = 1'b0;
   mem_en = 1'b0;

   case(state)
   4'h1: begin
   end
   4'h2: begin
      valid = 1'b1;
      if(ready) begin
         valid = 1'b0;
      end
   end
   4'h3: begin
      mem_en = 1'b1;
      if(!write_done) begin
         next = 1'b1;
      end
   end
   default: begin
   end
   endcase
end

endmodule

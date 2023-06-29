`timescale 1ns / 1ps

module VWrite #(
   parameter DATA_W = 32,
   parameter ADDR_W = 10,
   parameter MEM_ADDR_W = 10,
   parameter PERIOD_W = 10,
   parameter AXI_ADDR_W = 64
   )
   (
   input                  clk,
   input                  rst,

   input                  running,
   input                  run,
   output                 done,

   // Databus interface
   input                  databus_ready_0,
   output                 databus_valid_0,
   output[AXI_ADDR_W-1:0] databus_addr_0,
   input [DATA_W-1:0]     databus_rdata_0,
   output [DATA_W-1:0]    databus_wdata_0,
   output [DATA_W/8-1:0]  databus_wstrb_0,
   output [7:0]           databus_len_0,
   input                  databus_last_0,

   // input / output data
   input [DATA_W-1:0]     in0,

   // External memory
   output [ADDR_W-1:0]   ext_2p_addr_out_0,
   output [ADDR_W-1:0]   ext_2p_addr_in_0,
   output                ext_2p_write_0,
   output                ext_2p_read_0, // Changes without clk change
   input  [DATA_W-1:0]   ext_2p_data_in_0,
   output [DATA_W-1:0]   ext_2p_data_out_0,

   // configurations
   input [AXI_ADDR_W-1:0] ext_addr,
   input [MEM_ADDR_W-1:0] int_addr,
   input [10:0]           size,
   input [MEM_ADDR_W-1:0] iterA,
   input [PERIOD_W-1:0]   perA,
   input [PERIOD_W-1:0]   dutyA,
   input [MEM_ADDR_W-1:0] shiftA,
   input [MEM_ADDR_W-1:0] incrA,
   input [7:0]             length,
   input                   pingPong,

   input [MEM_ADDR_W-1:0] iterB,
   input [PERIOD_W-1:0]   perB,
   input [PERIOD_W-1:0]   dutyB,
   input [MEM_ADDR_W-1:0] startB,
   input [MEM_ADDR_W-1:0] shiftB,
   input [MEM_ADDR_W-1:0] incrB,
   input [31:0]            delay0, // delayB
   input                   reverseB,
   input                   extB,
   input [MEM_ADDR_W-1:0] iter2B,
   input [PERIOD_W-1:0]   per2B,
   input [MEM_ADDR_W-1:0] shift2B,
   input [MEM_ADDR_W-1:0] incr2B
   );

   assign databus_addr_0 = ext_addr;
   assign databus_wstrb_0 = 4'b1111;
   assign databus_len_0 = length;

   wire gen_done;
   reg doneA;
   reg doneB;
   wire doneB_int;
   assign done = doneA & doneB;

   always @(posedge clk,posedge rst)
   begin
      if(rst) begin
         doneA <= 1'b1;
         doneB <= 1'b1;
      end else if(run) begin
         doneA <= 1'b0;
         doneB <= 1'b0;
      end else  begin
         doneB <= doneB_int;
         if(databus_valid_0 && databus_ready_0 && databus_last_0)
            doneA <= 1'b1;
      end
   end

   function [MEM_ADDR_W-1:0] reverseBits;
      input [MEM_ADDR_W-1:0]   word;
      integer                   i;

      begin
        for (i=0; i < MEM_ADDR_W; i=i+1)
          reverseBits[i] = word[MEM_ADDR_W-1 - i];
      end
   endfunction

   reg pingPongState;

   reg [MEM_ADDR_W-1:0] startA;
   always @*
   begin
      startA = 0;
      startA[MEM_ADDR_W-1] = pingPong ? !pingPongState : 0;
   end

   wire [1:0]    direction = 2'b10;
   wire [31:0]   delayA    = 0;

   // port addresses and enables
   wire [MEM_ADDR_W-1:0] addrA, addrA_int, addrA_int2;
   wire [MEM_ADDR_W-1:0] addrB, addrB_int, addrB_int2;

   wire [MEM_ADDR_W-1:0]      startB_inst = pingPong ? {pingPongState,startB[MEM_ADDR_W-2:0]} : startB;

   // data inputs
   wire                   req;
   wire                   rnw;
   wire [DATA_W-1:0]      data_out;

   // Ping pong 
   always @(posedge clk,posedge rst)
   begin
      if(rst)
         pingPongState <= 0;
      else if(run)
         pingPongState <= pingPong ? (!pingPongState) : 1'b0;
   end

   // mem enables output by addr gen
   wire enA = req;
   wire enB;

   // write enables
   wire wrB = (enB & ~extB); //addrgen on & input on & input isn't address

   wire [DATA_W-1:0]      data_to_wrB = in0;

   wire gen_valid,gen_ready;
   wire [MEM_ADDR_W-1:0] gen_addr;

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
   assign addrB_int2 = reverseB? reverseBits(addrB_int) : addrB_int;
   
   wire read_en;
   wire [MEM_ADDR_W-1:0] read_addr;
   wire [DATA_W-1:0] read_data;

   MemoryReader #(.ADDR_W(MEM_ADDR_W))
   reader(
      // Slave
      .s_valid(gen_valid),
      .s_ready(gen_ready),
      .s_addr(gen_addr),

      // Master
      .m_valid(databus_valid_0),
      .m_ready(databus_ready_0),
      .m_addr(),
      .m_data(databus_wdata_0),

      // Connect to memory
      .mem_enable(read_en),
      .mem_addr(read_addr),
      .mem_data(read_data),

      .clk(clk),
      .rst(rst)
   );

   assign ext_2p_write_0 = enB & wrB;
   assign ext_2p_addr_out_0 = addrB;
   assign ext_2p_data_out_0 = data_to_wrB;

   assign ext_2p_read_0 = read_en;
   assign ext_2p_addr_in_0 = read_addr;
   assign read_data = ext_2p_data_in_0;

endmodule


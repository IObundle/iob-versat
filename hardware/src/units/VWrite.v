`timescale 1ns / 1ps

module VWrite #(
   parameter DATA_W     = 32,
   parameter ADDR_W     = 14,
   parameter PERIOD_W   = 12, // Must be 2 less than ADDR_W (boundary of 4) (for 32 bit DATA_W)
   parameter AXI_ADDR_W = 32,
   parameter AXI_DATA_W = 32,
   parameter DELAY_W    = 7,
   parameter LEN_W      = 8
) (
   input clk,
   input rst,

   input  running,
   input  run,
   output done,

   // Databus interface
   input                     databus_ready_0,
   output                    databus_valid_0,
   output [  AXI_ADDR_W-1:0] databus_addr_0,
   input  [  AXI_DATA_W-1:0] databus_rdata_0,
   output [  AXI_DATA_W-1:0] databus_wdata_0,
   output [AXI_DATA_W/8-1:0] databus_wstrb_0,
   output [       LEN_W-1:0] databus_len_0,
   input                     databus_last_0,

   // input / output data
   input [DATA_W-1:0] in0,

   // External memory
   output [    ADDR_W-1:0] ext_2p_addr_out_0,
   output [    ADDR_W-1:0] ext_2p_addr_in_0,
   output                  ext_2p_write_0,
   output                  ext_2p_read_0,
   input  [AXI_DATA_W-1:0] ext_2p_data_in_0,
   output [    DATA_W-1:0] ext_2p_data_out_0,

   // configurations
   input [AXI_ADDR_W-1:0] ext_addr,
   input [  PERIOD_W-1:0] perA,
   input [    ADDR_W-1:0] incrA,
   input [     LEN_W-1:0] length,
   input                  pingPong,

   // configurations
`ifdef COMPLEX_INTERFACE
   input [PERIOD_W-1:0] dutyA,
   input [  ADDR_W-1:0] int_addr,
   input [  ADDR_W-1:0] iterA,
   input [  ADDR_W-1:0] shiftA,
`endif

   input [  ADDR_W-1:0] iterB,
   input [PERIOD_W-1:0] perB,
   input [PERIOD_W-1:0] dutyB,
   input [  ADDR_W-1:0] startB,
   input [  ADDR_W-1:0] shiftB,
   input [  ADDR_W-1:0] incrB,
   input [ DELAY_W-1:0] delay0,    // delayB
   input                reverseB,
   input                extB,
   input [  ADDR_W-1:0] iter2B,
   input [PERIOD_W-1:0] per2B,
   input [  ADDR_W-1:0] shift2B,
   input [  ADDR_W-1:0] incr2B,

   input enableWrite
);

   assign databus_addr_0  = ext_addr;
   assign databus_wstrb_0 = ~0;
   assign databus_len_0   = length;

   wire gen_done;
   reg  doneWrite; // Databus write part
   reg  doneStore;
   wire doneStore_int;
   assign done = (doneWrite & doneStore);

   always @(posedge clk, posedge rst) begin
      if (rst) begin
         doneWrite <= 1'b1;
         doneStore <= 1'b1;
      end else if (run) begin
         doneWrite <= !enableWrite;
         doneStore <= 1'b0;
      end else if (running) begin
         doneStore <= doneStore_int;
         if (databus_valid_0 && databus_ready_0 && databus_last_0) doneWrite <= 1'b1;
      end
   end

   function [ADDR_W-1:0] reverseBits;
      input [ADDR_W-1:0] word;
      integer i;

      begin
         for (i = 0; i < ADDR_W; i = i + 1) reverseBits[i] = word[ADDR_W-1-i];
      end
   endfunction

   reg              pingPongState;

   reg [ADDR_W-1:0] startA;
   always @* begin
      startA           = 0;
      startA[ADDR_W-1] = pingPong ? !pingPongState : 0;
   end

   wire [ 1:0] direction = 2'b10;
   wire [31:0] delayA = 0;

   // port addresses and enables
   wire [ADDR_W-1:0] addrB, addrB_int, addrB_int2;

   wire [ADDR_W-1:0] startB_inst = pingPong ? {pingPongState, startB[ADDR_W-2:0]} : startB;

   // data inputs
   wire rnw;
   wire [DATA_W-1:0] data_out;

   // Ping pong 
   always @(posedge clk, posedge rst) begin
      if (rst) pingPongState <= 0;
      else if (run) pingPongState <= pingPong ? (!pingPongState) : 1'b0;
   end

   // mem enables output by addr gen
   wire enB;

   // write enables
   wire wrB = (enB & ~extB);  //addrgen on & input on & input isn't address

   wire [DATA_W-1:0] data_to_wrB = in0;

   wire gen_valid, gen_ready;
   wire [ADDR_W-1:0] gen_addr;

   localparam DIFF = AXI_DATA_W / DATA_W;
   localparam DIFF_BIT_W = $clog2(DIFF);

   SimpleAddressGen #(
      .ADDR_W(ADDR_W),
      .DATA_W(AXI_DATA_W),
      .PERIOD_W(PERIOD_W)
   ) addrgenWrite (
      .clk_i(clk),
      .rst_i(rst),
      .run_i(run && enableWrite),

      //configurations 
      .period_i(perA),
      .delay_i (delayA),
      .start_i (startA),
      .incr_i  (incrA),

`ifdef COMPLEX_INTERFACE
      .iterations_i(iterA),
      .duty_i      (dutyA),
      .shift_i     (shiftA),
`endif

      //outputs 
      .valid_o(gen_valid),
      .ready_i(gen_ready),
      .addr_o (gen_addr),
      .done_o (gen_done)
   );

   SimpleAddressGen #(
      .ADDR_W(ADDR_W),
      .DATA_W(DATA_W),
      .DELAY_W(DELAY_W),
      .PERIOD_W(PERIOD_W)
   ) addrgenStore (
      .clk_i(clk),
      .rst_i(rst),
      .run_i(run),

      //configurations 
      .period_i(perB),
      .delay_i (delay0),
      .start_i (startB_inst),
      .incr_i  (incrB),

`ifdef COMPLEX_INTERFACE
      .iterations_i(iterB),
      .duty_i      (dutyB),
      .shift_i     (shiftB),
`endif

      //outputs 
      .valid_o(enB),
      .ready_i(1'b1),
      .addr_o (addrB_int),
      .done_o (doneStore_int)
   );

   /*
    xaddrgen2 #(.MEM_ADDR_W(ADDR_W)) addrgen2B (
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
                       .done(doneStore_int)
                       );
   */

   assign addrB_int2 = addrB_int;  //(reverseB ? reverseBits(addrB_int) : addrB_int) << OFFSET_W;
   assign addrB      = addrB_int2;

   wire                  read_en;
   wire [    ADDR_W-1:0] read_addr;
   wire [AXI_DATA_W-1:0] read_data;

   wire                  m_valid;

   MemoryReader #(
      .ADDR_W(ADDR_W),
      .DATA_W(AXI_DATA_W)
   ) reader (
      // Slave
      .s_valid_i(gen_valid),
      .s_ready_o(gen_ready),
      .s_addr_i (gen_addr),

      // Master
      .m_valid_o(m_valid),
      .m_ready_i(databus_ready_0),
      .m_addr_o (),
      .m_data_o (databus_wdata_0),
      .m_last_i (databus_last_0),

      // Connect to memory
      .mem_enable_o(read_en),
      .mem_addr_o  (read_addr),
      .mem_data_i  (read_data),

      .clk_i(clk),
      .rst_i(rst)
   );

   /*
   wire [ADDR_W-1:0] true_read_addr;
   generate
   if(AXI_DATA_W == 32) begin
   assign true_read_addr = read_addr;
   end   
   else if(AXI_DATA_W == 64) begin
   assign true_read_addr = (read_addr >> 1);      
   end
   else if(AXI_DATA_W == 128) begin
   assign true_read_addr = (read_addr >> 2);      
   end
   else if(AXI_DATA_W == 256) begin
   assign true_read_addr = (read_addr >> 3);      
   end
   else if(AXI_DATA_W == 512) begin
   assign true_read_addr = (read_addr >> 4);      
   end else begin
      initial begin $display("NOT IMPLEMENTED\n"); $finish(); end
   end
   endgenerate
   */

   assign databus_valid_0   = (m_valid & !doneWrite);

   assign ext_2p_write_0    = wrB;
   assign ext_2p_addr_out_0 = addrB;
   assign ext_2p_data_out_0 = data_to_wrB;

   assign ext_2p_read_0     = read_en;
   assign ext_2p_addr_in_0  = read_addr;
   assign read_data         = ext_2p_data_in_0;

endmodule  // VWrite


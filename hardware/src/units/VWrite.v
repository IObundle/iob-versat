`timescale 1ns / 1ps

`define COMPLEX_INTERFACE

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
   (* versat_stage="Write" *) input [AXI_ADDR_W-1:0] ext_addr,
   (* versat_stage="Write" *) input [  PERIOD_W-1:0] write_per,
   (* versat_stage="Write" *) input [    ADDR_W-1:0] write_incr,
   (* versat_stage="Write" *) input [PERIOD_W-1:0]   write_duty,

   (* versat_stage="Write" *) input [  ADDR_W-1:0]   write_iter,
   (* versat_stage="Write" *) input [  ADDR_W-1:0]   write_shift,

   (* versat_stage="Write" *) input [     LEN_W-1:0] write_length,
   (* versat_stage="Write" *) input                  write_enabled,

   input                  pingPong,

   input [  ADDR_W-1:0] input_iter,
   input [PERIOD_W-1:0] input_per,
   input [PERIOD_W-1:0] input_duty,
   input [  ADDR_W-1:0] input_start,
   input [  ADDR_W-1:0] input_shift,
   input [  ADDR_W-1:0] input_incr,

   input [  ADDR_W-1:0] input_iter2,
   input [PERIOD_W-1:0] input_per2,
   input [  ADDR_W-1:0] input_shift2,
   input [  ADDR_W-1:0] input_incr2,

   input [ DELAY_W-1:0] delay0
);

   assign databus_addr_0  = ext_addr;
   assign databus_wstrb_0 = ~0;
   assign databus_len_0   = write_length;

   reg  doneWrite; // Databus write part
   reg  doneStore;
   wire doneStore_int;
   assign done = (doneWrite & doneStore);

   always @(posedge clk, posedge rst) begin
      if (rst) begin
         doneWrite <= 1'b1;
         doneStore <= 1'b1;
      end else if (run) begin
         doneWrite <= !write_enabled;
         doneStore <= 1'b0;
      end else if (running) begin
         doneStore <= doneStore_int;
         if (databus_valid_0 && databus_ready_0 && databus_last_0) doneWrite <= 1'b1;
      end
   end

   // Ping pong and related logic for the initial address
   reg pingPongState;

   wire [ADDR_W-1:0] write_start = {pingPong && !pingPongState , {(ADDR_W-1){1'b0}}};

   // port addresses and enables
   wire [ADDR_W-1:0] store_addr;
   wire [ADDR_W-1:0] input_start_inst = {pingPong && pingPongState, input_start[ADDR_W-2:0]};

   // Ping pong 
   always @(posedge clk, posedge rst) begin
      if (rst) pingPongState <= 0;
      else if (run) pingPongState <= pingPong ? (!pingPongState) : 1'b0;
   end

   // mem enables output by addr gen
   wire store_en,do_store;

   wire [DATA_W-1:0] store_data = in0;

   wire gen_valid, gen_ready;
   wire [ADDR_W-1:0] gen_addr;

   AddressGen3 #(
      .ADDR_W(ADDR_W),
      .DATA_W(AXI_DATA_W),
      .PERIOD_W(PERIOD_W)
   ) addrgenWrite (
      .clk_i(clk),
      .rst_i(rst),
      .run_i(run && write_enabled),

      //configurations 
      .period_i(write_per),
      .delay_i (0),
      .start_i (write_start),
      .incr_i  (write_incr),

`ifdef COMPLEX_INTERFACE
      .iterations_i(write_iter),
      .duty_i      (write_duty),
      .shift_i     (write_shift),
`endif

      .period2_i(0),
      .incr2_i(0),
      .iterations2_i(0),
      .shift2_i(0),

      .period3_i(0),
      .incr3_i(0),
      .iterations3_i(0),
      .shift3_i(0),

      //outputs 
      .valid_o(gen_valid),
      .ready_i(gen_ready),
      .addr_o (gen_addr),
      .store_o(),
      .done_o ()
   );

   AddressGen3 #(
      .ADDR_W(ADDR_W),
      .DATA_W(DATA_W),
      .DELAY_W(DELAY_W),
      .PERIOD_W(PERIOD_W)
   ) addrgenStore (
      .clk_i(clk),
      .rst_i(rst),
      .run_i(run),

      //configurations 
      .period_i(input_per),
      .delay_i (delay0),
      .start_i (input_start_inst),
      .incr_i  (input_incr),

      .iterations_i(input_iter),
      .duty_i      (input_duty),
      .shift_i     (input_shift),

      .period2_i(input_per2),
      .incr2_i(input_incr2),
      .iterations2_i(input_iter2),
      .shift2_i(input_shift2),

      .period3_i(0),
      .incr3_i(0),
      .iterations3_i(0),
      .shift3_i(0),

      //outputs 
      .valid_o(store_en),
      .ready_i(1'b1),
      .addr_o (store_addr),
      .store_o(do_store),
      .done_o (doneStore_int)
   );

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

   assign databus_valid_0   = (m_valid & !doneWrite);

   assign ext_2p_write_0    = store_en && do_store;
   assign ext_2p_addr_out_0 = store_addr;
   assign ext_2p_data_out_0 = store_data;

   assign ext_2p_read_0     = read_en;
   assign ext_2p_addr_in_0  = read_addr;
   assign read_data         = ext_2p_data_in_0;

endmodule  // VWrite


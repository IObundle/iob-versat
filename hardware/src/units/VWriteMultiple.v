`timescale 1ns / 1ps

module VWriteMultiple #(
   parameter SIZE_W     = 32,
   parameter DATA_W     = 32,
   parameter ADDR_W     = 20,
   parameter PERIOD_W   = 18, // Must be 2 less than ADDR_W (boundary of 4) (for 32 bit DATA_W)
   parameter AXI_ADDR_W = 32,
   parameter AXI_DATA_W = 32,
   parameter DELAY_W    = 7,
   parameter LEN_W      = 16
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
   (* versat_stage="Write" *) input [    ADDR_W-1:0] write_start,
   (* versat_stage="Write" *) input [  PERIOD_W-1:0] write_per,
   (* versat_stage="Write" *) input [    ADDR_W-1:0] write_incr,
   (* versat_stage="Write" *) input [PERIOD_W-1:0]   write_duty,

   (* versat_stage="Write" *) input [  ADDR_W-1:0]   write_iter,
   (* versat_stage="Write" *) input [  ADDR_W-1:0]   write_shift,

   (* versat_stage="Write" *) input [    ADDR_W-1:0] write_amount_minus_one,
   (* versat_stage="Write" *) input [     LEN_W-1:0] write_length,
   (* versat_stage="Write" *) input                  write_enabled,
   (* versat_stage="Write" *) input [AXI_ADDR_W-1:0] write_addr_shift,

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

   input                input_ignore_first,
   input [ 20-1:0]      input_extra_delay,

   input [ DELAY_W-1:0] delay0
);

   //reg  doneWrite; // Databus write part
   wire transferDone;
   reg  doneStore;
   wire doneStore_int;
   assign done = (transferDone & doneStore);

   wire data_valid,data_ready,data_last;
   wire [AXI_DATA_W-1:0] data_data;

   always @(posedge clk, posedge rst) begin
      if (rst) begin
         //doneWrite <= 1'b1;
         doneStore <= 1'b1;
      end else if (run) begin
         //doneWrite <= !write_enabled;
         doneStore <= 1'b0;
      end else if (running) begin
         doneStore <= doneStore_int;
         //if (databus_valid_0 && databus_ready_0 && databus_last_0) doneWrite <= 1'b1;
      end
   end

   // Ping pong and related logic for the initial address
   reg pingPongState;

   // Ping pong 
   always @(posedge clk, posedge rst) begin
      if (rst) pingPongState <= 0;
      else if (run) pingPongState <= pingPong ? (!pingPongState) : 1'b0;
   end

   //wire [ADDR_W-1:0] write_start = {pingPong && !pingPongState , {(ADDR_W-1){1'b0}}};

   // port addresses and enables
   wire [ADDR_W-1:0] store_addr_temp;
   wire [ADDR_W-1:0] input_start_inst = {pingPong && pingPongState, input_start[ADDR_W-2:0]};

   // mem enables output by addr gen
   wire store_en,do_store;

   wire [DATA_W-1:0] store_data = in0;

   wire gen_valid, gen_ready;
   wire [ADDR_W-1:0] gen_addr_temp;

   SuperAddress #(
      .AXI_ADDR_W(AXI_ADDR_W),
      .AXI_DATA_W(AXI_DATA_W),
      .LEN_W(LEN_W),
      .COUNT_W(ADDR_W),
      .ADDR_W(ADDR_W),
      .DATA_W(SIZE_W),
      .PERIOD_W(PERIOD_W),
      .DELAY_W(1)
      ) writer (
      .clk_i(clk),
      .rst_i(rst),
      .run_i(run && write_enabled),
      .done_o(transferDone),

      .ignore_first_i(1'b0),

      //configurations 
      .period_i(write_per),
      .delay_i (1'b0),
      //.start_i (0),
      .start_i (write_start),
      .incr_i  (write_incr),
      .iterations_i(write_iter),
      .duty_i      (write_duty),
      .shift_i     (write_shift),

      .period2_i({PERIOD_W{1'b0}}),
      .incr2_i({20{1'b0}}),
      .iterations2_i({20{1'b0}}),
      .shift2_i({20{1'b0}}),

      .period3_i({PERIOD_W{1'b0}}),
      .incr3_i({20{1'b0}}),
      .iterations3_i({20{1'b0}}),
      .shift3_i({20{1'b0}}),

      .doneDatabus(),
      .doneAddress(),

      //outputs 
      .valid_o(gen_valid),
      .ready_i(gen_ready),
      .addr_o (gen_addr_temp),
      .store_o(),

      .databus_ready(databus_ready_0),
      .databus_valid(databus_valid_0),
      .databus_addr(databus_addr_0),
      .databus_len(databus_len_0),
      .databus_last(databus_last_0),

      // Data interface
      .data_valid_i(data_valid),
      .data_ready_i(1'b1),
      .reading(1'b0),
      .data_last_o(data_last),

      .count_i(write_amount_minus_one + 19'b1),
      .start_address_i(ext_addr),
      .address_shift_i(write_addr_shift),
      .databus_length(write_length)
   );

   assign data_ready = databus_ready_0;
   assign databus_wstrb_0 = 4'b1111;
   assign databus_wdata_0 = data_data; 

   wire [ADDR_W-1:0] gen_addr = {pingPong ? !pingPongState : gen_addr_temp[ADDR_W-1],gen_addr_temp[ADDR_W-2:0]};

   AddressGen3 #(
      .ADDR_W(ADDR_W),
      .DATA_W(DATA_W),
      .DELAY_W(20),
      .PERIOD_W(PERIOD_W)
   ) addrgenStore (
      .clk_i(clk),
      .rst_i(rst),
      .run_i(run),

      .ignore_first_i(input_ignore_first),

      //configurations 
      .period_i(input_per),
      .delay_i ({13'b0,delay0} + input_extra_delay),
      .start_i ({1'b0,input_start[ADDR_W-2:0]}),
      //.start_i (input_start_inst),
      .incr_i  (input_incr),

      .iterations_i(input_iter),
      .duty_i      (input_duty),
      .shift_i     (input_shift),

      .period2_i(input_per2),
      .incr2_i(input_incr2),
      .iterations2_i(input_iter2),
      .shift2_i(input_shift2),

      .period3_i({PERIOD_W{1'b0}}),
      .incr3_i({20{1'b0}}),
      .iterations3_i({20{1'b0}}),
      .shift3_i({20{1'b0}}),

      //outputs 
      .valid_o(store_en),
      .ready_i(1'b1),
      .addr_o (store_addr_temp),
      .store_o(do_store),
      .done_o (doneStore_int)
   );

   wire [ADDR_W-1:0] store_addr = {pingPong ? pingPongState : store_addr_temp[ADDR_W-1],store_addr_temp[ADDR_W-2:0]};

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
      .m_valid_o(data_valid),
      .m_ready_i(data_ready),
      .m_addr_o (),
      .m_data_o (data_data),
      .m_last_i (data_last),

      // Connect to memory
      .mem_enable_o(read_en),
      .mem_addr_o  (read_addr),
      .mem_data_i  (read_data),

      .clk_i(clk),
      .rst_i(rst)
   );

   assign ext_2p_write_0    = store_en && do_store;
   assign ext_2p_addr_out_0 = store_addr;
   assign ext_2p_data_out_0 = store_data;

   assign ext_2p_read_0     = read_en;
   assign ext_2p_addr_in_0  = read_addr;
   assign read_data         = ext_2p_data_in_0;

   // Reports most common errors
   always @* begin
      if(pingPong && gen_addr_temp[ADDR_W-1]) begin
         $display("%m: Overflow of memory when using PingPong for reading");
      end
      if(pingPong && store_addr_temp[ADDR_W-1]) begin
         $display("%m: Overflow of write memory when using PingPong for outputting");
      end
      if(pingPong && input_start[ADDR_W-1]) begin
         $display("%m: Last bit of output start ignored when using PingPong");
      end
   end

endmodule  // VWrite

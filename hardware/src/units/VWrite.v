`timescale 1ns / 1ps

module VWrite #(
   parameter DATA_W     = 32,  // Internal datapath width
   parameter ADDR_W     = 16,
   parameter PERIOD_W   = 14,  // Must be 2 less than ADDR_W (boundary of 4) (for 32 bit DATA_W)
   parameter AXI_ADDR_W = 32,
   parameter AXI_DATA_W = 32,  // External databus width
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

   (* versat_stage="Write" *) input [    ADDR_W-1:0] amount_minus_one,
   (* versat_stage="Write" *) input [     LEN_W-1:0] length,
   (* versat_stage="Write" *) input                  enabled,
   (* versat_stage="Write" *) input [AXI_ADDR_W-1:0] addr_shift,

   input pingPong,

   input [  ADDR_W-1:0] iter,
   input [PERIOD_W-1:0] per,
   input [PERIOD_W-1:0] duty,
   input [  ADDR_W-1:0] start,
   input [  ADDR_W-1:0] shift,
   input [  ADDR_W-1:0] incr,

   input [  ADDR_W-1:0] iter2,
   input [PERIOD_W-1:0] per2,
   input [  ADDR_W-1:0] shift2,
   input [  ADDR_W-1:0] incr2,

   input [  ADDR_W-1:0] iter3,
   input [PERIOD_W-1:0] per3,
   input [  ADDR_W-1:0] shift3,
   input [  ADDR_W-1:0] incr3,

   input          ignore_first,
   input [20-1:0] extra_delay,

   input [DELAY_W-1:0] delay0
);
   localparam DELAY_STORE_W = (DELAY_W > 20) ? DELAY_W : 20;
   localparam EXTRA_DELAY_W = 20;

   //reg  doneWrite; // Databus write part
   wire transferDone;
   reg  doneStore;
   wire doneStore_int;
   assign done = (transferDone & doneStore);

   wire data_valid, data_ready;
   wire [   AXI_DATA_W-1:0] data_data;
   wire [DELAY_STORE_W-1:0] delay_store;

   generate
      if (DELAY_W < EXTRA_DELAY_W) begin : gen_delay_smaller
         assign delay_store = {{(EXTRA_DELAY_W - DELAY_W) {1'b0}}, delay0} + extra_delay;
      end else if (DELAY_W > EXTRA_DELAY_W) begin : gen_delay_bigger
         assign delay_store = {{(DELAY_W - EXTRA_DELAY_W) {1'b0}}, extra_delay} + delay0;
      end else begin : gen_delay_equal
         assign delay_store = delay0 + extra_delay;
      end
   endgenerate

   always @(posedge clk, posedge rst) begin
      if (rst) begin
         doneStore <= 1'b1;
      end else if (run) begin
         doneStore <= 1'b0;
      end else if (running) begin
         doneStore <= doneStore_int;
      end
   end

   // Ping pong and related logic for the initial address
   reg pingPongState;

   // Ping pong 
   always @(posedge clk, posedge rst) begin
      if (rst) pingPongState <= 0;
      else if (run) pingPongState <= pingPong ? (!pingPongState) : 1'b0;
   end

   //wire [ADDR_W-1:0] start = {pingPong && !pingPongState , {(ADDR_W-1){1'b0}}};

   // port addresses and enables
   wire [ADDR_W-1:0] store_addr_temp;

   // mem enables output by addr gen
   wire store_en, do_store;

   wire [DATA_W-1:0] store_data = in0;

   reg  [ADDR_W-1:0] gen_addr_temp;
   reg               gen_valid;
   wire              gen_ready;

   localparam OFFSET_TEMP = AXI_DATA_W / 8;
   localparam [ADDR_W-1:0] OFFSET_W = OFFSET_TEMP[ADDR_W-1:0];

   always @(posedge clk, posedge rst) begin
      if (rst) begin
         gen_addr_temp <= 0;
      end else if(run && enabled && length != 0) begin
         gen_addr_temp <= 0;
         gen_valid     <= 1'b1;
      end else begin
         if (gen_valid && gen_ready) begin
            gen_addr_temp <= gen_addr_temp + OFFSET_W;
         end
         if (!running) begin
            gen_valid <= 0;
         end
      end
   end

   wire [ADDR_W-1:0] amount;
   assign amount = amount_minus_one + 1'b1;

   SuperAddress #(
      .AXI_ADDR_W(AXI_ADDR_W),
      .LEN_W     (LEN_W),
      .COUNT_W   (ADDR_W),
      .ADDR_W    (ADDR_W),
      .DATA_W    (DATA_W),
      .PERIOD_W  (PERIOD_W),
      .DELAY_W   (1)
   ) writer (
      .clk_i (clk),
      .rst_i (rst),
      .run_i (run && enabled && (length != 0)),
      .done_o(transferDone),

      .ignore_first_i(1'b0),

      //configurations 
      .per_i  ({PERIOD_W{1'b0}}),
      .delay_i(1'b0),
      //.start_i (0),
      .start_i(0),
      .incr_i ({ADDR_W{1'b0}}),
      .iter_i ({ADDR_W{1'b0}}),
      .duty_i ({PERIOD_W{1'b0}}),
      .shift_i({ADDR_W{1'b0}}),

      .per2_i  ({PERIOD_W{1'b0}}),
      .incr2_i ({ADDR_W{1'b0}}),
      .iter2_i ({ADDR_W{1'b0}}),
      .shift2_i({ADDR_W{1'b0}}),

      .per3_i  ({PERIOD_W{1'b0}}),
      .incr3_i ({ADDR_W{1'b0}}),
      .iter3_i ({ADDR_W{1'b0}}),
      .shift3_i({ADDR_W{1'b0}}),

      .doneDatabus(),
      .doneAddress(),

      .valid_o(),
      .ready_i(1'b1),
      .addr_o (),
      .store_o(),

      .databus_ready(databus_ready_0),
      .databus_valid(databus_valid_0),
      .databus_addr (databus_addr_0),
      .databus_len  (databus_len_0),
      .databus_last (databus_last_0),

      // Data interface
      .data_valid_i(data_valid),
      .data_ready_i(1'b1),
      .reading     (1'b0),

      .count_i        (amount),
      .start_address_i(ext_addr),
      .address_shift_i(addr_shift),
      .databus_length (length)
   );

   assign data_ready      = databus_ready_0;
   assign databus_wstrb_0 = ~0;
   assign databus_wdata_0 = data_data;

   wire [ADDR_W-1:0] gen_addr = {
      pingPong ? !pingPongState : gen_addr_temp[ADDR_W-1], gen_addr_temp[ADDR_W-2:0]
   };

   AddressGen3 #(
      .ADDR_W  (ADDR_W),
      .DATA_W  (DATA_W),
      .DELAY_W (DELAY_STORE_W),
      .PERIOD_W(PERIOD_W)
   ) addrgenStore (
      .clk_i(clk),
      .rst_i(rst),
      .run_i(run),

      .ignore_first_i(ignore_first),

      //configurations 
      .per_i  (per),
      .delay_i(delay_store),
      .start_i({1'b0, start[ADDR_W-2:0]}),
      .incr_i (incr),

      .iter_i (iter),
      .duty_i (duty),
      .shift_i(shift),

      .per2_i  (per2),
      .incr2_i (incr2),
      .iter2_i (iter2),
      .shift2_i(shift2),

      .per3_i  (per3),
      .incr3_i (incr3),
      .iter3_i (iter3),
      .shift3_i(shift3),

      //outputs 
      .valid_o(store_en),
      .ready_i(1'b1),
      .addr_o (store_addr_temp),
      .store_o(do_store),
      .done_o (doneStore_int)
   );

   wire [ADDR_W-1:0] store_addr = {
      pingPong ? pingPongState : store_addr_temp[ADDR_W-1], store_addr_temp[ADDR_W-2:0]
   };

   wire read_en;
   wire [ADDR_W-1:0] read_addr;
   wire [AXI_DATA_W-1:0] read_data;

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
      .m_last_i (1'b0),

      // Connect to memory
      .mem_enable_o(read_en),
      .mem_addr_o  (read_addr),
      .mem_data_i  (read_data),

      .force_reset_i(!running || run),

      .clk_i(clk),
      .rst_i(rst)
   );

   assign ext_2p_write_0    = store_en && do_store;
   assign ext_2p_addr_out_0 = store_addr;
   assign ext_2p_data_out_0 = store_data;

   assign ext_2p_read_0     = read_en;
   assign ext_2p_addr_in_0  = read_addr;
   assign read_data         = ext_2p_data_in_0;

   reg reportedB;
   reg reportedC;

   // Reports most common errors

   /*
   reg reportedA;
   TODO: The value of gen_addr_temp goes past the values actually used. This causes this warning to 
         trigger even when we do not actually read past the values of the memory.
         If we want to keep the warnings around, need to fix this before restauring the warning logic.

   always @(posedge clk) begin
      if (run) begin
         reportedA <= 1'b0;
      end else if (pingPong && gen_addr_temp[ADDR_W-1] && reportedA == 1'b0) begin
         $display("%m: Overflow of memory when using PingPong for reading");
         reportedA <= 1'b1;
      end
   end
   */

   always @(posedge clk) begin
      if (run) begin
         reportedB <= 1'b0;
      end else if (pingPong && store_addr_temp[ADDR_W-1] && reportedB == 1'b0) begin
         $display("%m: Overflow of write memory when using PingPong for outputting");
         reportedB <= 1'b1;
      end
   end

   always @(posedge clk) begin
      if (run) begin
         reportedC <= 1'b0;
      end else if (pingPong && start[ADDR_W-1] && reportedC == 1'b0) begin
         $display("%m: Last bit of output start ignored when using PingPong");
         reportedC <= 1'b1;
      end
   end

endmodule  // VWrite

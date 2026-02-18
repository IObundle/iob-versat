`timescale 1ns / 1ps

module VRead #(
   parameter DATA_W     = 32,
   parameter ADDR_W     = 18,
   parameter PERIOD_W   = 16, // Must be 2 less than ADDR_W (boundary of 4) (for 32 bit DATA_W)
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
   input                         databus_ready_0,
   output                        databus_valid_0,
   output     [  AXI_ADDR_W-1:0] databus_addr_0,
   input      [  AXI_DATA_W-1:0] databus_rdata_0,
   output     [  AXI_DATA_W-1:0] databus_wdata_0,
   output     [AXI_DATA_W/8-1:0] databus_wstrb_0,
   output     [       LEN_W-1:0] databus_len_0,
   input                         databus_last_0,

   // input / output data
   (* versat_latency = 1 *) output [DATA_W-1:0] out0,

   // External memory
   output [    ADDR_W-1:0] ext_2p_addr_out_0,
   output [    ADDR_W-1:0] ext_2p_addr_in_0,
   output                  ext_2p_write_0,
   output                  ext_2p_read_0,
   input  [AXI_DATA_W-1:0] ext_2p_data_in_0,
   output [AXI_DATA_W-1:0] ext_2p_data_out_0,

   (* versat_stage="Read" *) input [AXI_ADDR_W-1:0] ext_addr,
   (* versat_stage="Read" *) input                  pingPong,

   (* versat_stage="Read" *) input [    ADDR_W-1:0] amount_minus_one,
   (* versat_stage="Read" *) input [     LEN_W-1:0] length,
   (* versat_stage="Read" *) input [AXI_ADDR_W-1:0] addr_shift,

   (* versat_stage="Read" *) input enabled,

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

   input [DELAY_W-1:0]  extra_delay,
   input                ignore_first,

   input [DELAY_W-1:0] delay0
);

   //assign databus_wdata_0 = 0;
   //assign databus_wstrb_0 = 0;
   //assign databus_len_0   = length;

   // output databus
   wire              transferDone;
   reg               doneOutput;
   wire              doneOutput_int;

   assign done = (transferDone  & doneOutput);

   wire data_valid,data_ready;
   wire [AXI_DATA_W-1:0] data_data;

   always @(posedge clk, posedge rst) begin
      if (rst) begin
         doneOutput <= 1'b1;
      end else if (run) begin
         doneOutput <= 1'b0;
      end else begin
         if (doneOutput_int) doneOutput <= 1'b1;
      end
   end

   // Ping pong and related logic for the initial address
   reg pingPongState;

   // port addresses and enables
   wire [ADDR_W-1:0] output_addr_temp;

   // Ping pong 
   always @(posedge clk, posedge rst) begin
      if (rst) pingPongState <= 0;
      else if (run) pingPongState <= pingPong ? (!pingPongState) : 1'b0;
   end

   wire [ADDR_W-1:0] amount;
   assign amount = amount_minus_one + 1'b1;
   
   //wire [ADDR_W-1:0] gen_addr_temp;
   //wire gen_valid, gen_ready;

   reg [ADDR_W-1:0] gen_addr_temp;
   reg gen_valid;
   wire gen_ready;

   localparam OFFSET_TEMP = AXI_DATA_W / 8;
   localparam [ADDR_W-1:0] OFFSET_W = OFFSET_TEMP[ADDR_W-1:0];

   always @(posedge clk,posedge rst) begin
      if(rst) begin
         gen_addr_temp <= 0;
      end else if(run && enabled && length != 0) begin
         gen_addr_temp <= 0;
         gen_valid <= 1'b1;
      end else begin
         if(gen_valid && gen_ready) begin
            gen_addr_temp <= gen_addr_temp + OFFSET_W;
         end
         if(!running) begin
            gen_valid <= 0;
         end
      end
   end

   SuperAddress #(
      .AXI_ADDR_W(AXI_ADDR_W),
      .LEN_W(LEN_W),
      .COUNT_W(ADDR_W),
      .ADDR_W(ADDR_W),
      .DATA_W(DATA_W),
      .PERIOD_W(PERIOD_W),
      .DELAY_W(1)
      ) reader (
      .clk_i(clk),
      .rst_i(rst),
      .run_i(run && enabled && length != 0),
      .done_o(transferDone),

      .ignore_first_i(1'b0),

      .per_i({PERIOD_W{1'b0}}),
      .delay_i (1'b0),
      .start_i ({ADDR_W{1'b0}}),
      .incr_i  ({ADDR_W{1'b0}}),

      .iter_i({ADDR_W{1'b0}}),
      .duty_i      ({PERIOD_W{1'b0}}),
      .shift_i     ({ADDR_W{1'b0}}),

      .per2_i({PERIOD_W{1'b0}}),
      .incr2_i({ADDR_W{1'b0}}),
      .iter2_i({ADDR_W{1'b0}}),
      .shift2_i({ADDR_W{1'b0}}),

      .per3_i({PERIOD_W{1'b0}}),
      .incr3_i({ADDR_W{1'b0}}),
      .iter3_i({ADDR_W{1'b0}}),
      .shift3_i({ADDR_W{1'b0}}),

      .doneDatabus(),
      .doneAddress(),

      //outputs 
      //.valid_o(gen_valid), // gen_valid
      //.ready_i(gen_ready), // gen_ready
      //.addr_o (gen_addr_temp), // gen_addr_temp

      .valid_o(),
      .ready_i(1'b1),
      .addr_o (),

      .store_o(),

      .databus_ready(databus_ready_0),
      .databus_valid(databus_valid_0),
      .databus_addr(databus_addr_0),
      .databus_len(databus_len_0),
      .databus_last(databus_last_0),

      // Data interface
      .data_valid_i(1'b0),
      .data_ready_i(data_ready),
      .reading(1'b1),

      .count_i(amount),
      .start_address_i(ext_addr),
      .address_shift_i(addr_shift),
      .databus_length(length)
   );

assign databus_wdata_0 = 0;
assign databus_wstrb_0 = 0;
assign data_valid = databus_ready_0;
assign data_data = databus_rdata_0;

   wire [ADDR_W-1:0] gen_addr = {pingPong ? !pingPongState : gen_addr_temp[ADDR_W-1],gen_addr_temp[ADDR_W-2:0]};

   // mem enables output by addr gen
   wire output_enabled;

   AddressGen3 #(
      .ADDR_W(ADDR_W),
      .DATA_W(DATA_W),
      .PERIOD_W(PERIOD_W),
      .DELAY_W(DELAY_W)
   ) addrgenOutput (
      .clk_i(clk),
      .rst_i(rst),
      .run_i(run),

      .ignore_first_i(ignore_first),

      //configurations 
      .per_i(per),
      .delay_i (delay0 + extra_delay),
      .start_i ({1'b0,start[ADDR_W-2:0]}),
      .incr_i  (incr),

      .iter_i(iter),
      .duty_i      (duty),
      .shift_i     (shift),

      .per2_i(per2),
      .incr2_i(incr2),
      .iter2_i(iter2),
      .shift2_i(shift2),

      .per3_i(per3),
      .incr3_i(incr3),
      .iter3_i(iter3),
      .shift3_i(shift3),

      //outputs 
      .valid_o(output_enabled),
      .ready_i(1'b1),
      .addr_o (output_addr_temp),
      .store_o(),
      .done_o (doneOutput_int)
   );

   wire [ADDR_W-1:0] true_output_addr = output_addr_temp;

   /*
      Basically, I need to have VRead simulate a initial loop of 17 but then go back to looping 16.

   */

   wire [ADDR_W-1:0] output_addr = {pingPong ? pingPongState : true_output_addr[ADDR_W-1],true_output_addr[ADDR_W-2:0]};

   wire                  write_en;
   wire [    ADDR_W-1:0] write_addr;
   wire [AXI_DATA_W-1:0] write_data;

   JoinTwoHandshakes #(
      .FIRST_DATA_W(ADDR_W),
      .SECOND_DATA_W(AXI_DATA_W)
   ) writer (
      .first_valid_i(gen_valid),
      .first_ready_o(gen_ready),
      .first_data_i(gen_addr),

      .second_valid_i(data_valid),
      .second_ready_o(data_ready),
      .second_data_i(data_data),

      .result_valid_o(write_en),
      .result_ready_i(1'b1),
      .result_first_data_o(write_addr),
      .result_second_data_o(write_data),

      .forceReset(!running || run),

      .clk_i(clk),
      .rst_i(rst)
   );

   localparam DIFF = AXI_DATA_W / DATA_W;
   localparam DECISION_BIT_W = $clog2(DIFF);
   localparam DECISION_BIT_START = $clog2(DATA_W / 8);

   generate
      if (AXI_DATA_W > DATA_W) begin
         reg [DECISION_BIT_W-1:0] sel_0;  // Matches addr_0_port_0
         always @(posedge clk, posedge rst) begin
            if (rst) begin
               sel_0 <= 0;
            end else begin
               sel_0 <= output_addr[DECISION_BIT_START+:DECISION_BIT_W];
            end
         end

         WideAdapter #(
            .INPUT_W (AXI_DATA_W),
            .OUTPUT_W(DATA_W),
            .SIZE_W  (DATA_W)
         ) adapter (
            .sel_i(sel_0),
            .in_i (ext_2p_data_in_0),
            .out_o(out0)
         );
      end else begin
         assign out0 = ext_2p_data_in_0;
      end  // if(AXI_DATA_W > DATA_W)
   endgenerate

   assign ext_2p_write_0    = write_en;
   assign ext_2p_addr_out_0 = write_addr;
   assign ext_2p_data_out_0 = write_data;

   assign ext_2p_read_0     = output_enabled;
   assign ext_2p_addr_in_0  = output_addr;

   reg reportedB;
   reg reportedC;

   // Reports most common errors
   /*
   reg reportedA;
   TODO: The value of gen_addr_temp goes past the values actually used. This causes this warning to 
         trigger even when we do not actually read past the values of the memory.
         If we want to keep the warnings around, need to fix this before restauring the warning logic.
   always @(posedge clk) begin
      if(run) begin
         reportedA <= 1'b0;
      end else if(pingPong && gen_addr_temp[ADDR_W-1] && reportedA == 1'b0) begin
         $display("%m: Overflow of memory when using PingPong for reading");
         reportedA <= 1'b1;
      end
   end
   */

   always @(posedge clk) begin
      if(run) begin
         reportedB <= 1'b0;
      end else if(pingPong && true_output_addr[ADDR_W-1] && reportedB == 1'b0) begin
         $display("%m: Overflow of write memory when using PingPong for outputting");
         reportedB <= 1'b1;
      end
   end

   always @(posedge clk) begin
      if(run) begin
         reportedC <= 1'b0;
      end else if(pingPong && start[ADDR_W-1] && reportedC == 1'b0) begin
         $display("%m: Last bit of output start ignored when using PingPong");
         reportedC <= 1'b1;
      end
   end


endmodule

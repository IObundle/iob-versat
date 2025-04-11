`timescale 1ns / 1ps

module VReadMultiple #(
   parameter SIZE_W     = 32,
   parameter DATA_W     = 32,
   parameter ADDR_W     = 17,
   parameter PERIOD_W   = 15, // Must be 2 less than ADDR_W (boundary of 4) (for 32 bit DATA_W)
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
   output reg [  AXI_ADDR_W-1:0] databus_addr_0,
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

   (* versat_stage="Read" *) input [    ADDR_W-1:0] read_amount_minus_one,
   (* versat_stage="Read" *) input [     LEN_W-1:0] read_length,
   (* versat_stage="Read" *) input [AXI_ADDR_W-1:0] read_addr_shift,

   (* versat_stage="Read" *) input read_enabled,

   input [  ADDR_W-1:0] output_iter,
   input [PERIOD_W-1:0] output_per,
   input [PERIOD_W-1:0] output_duty,
   input [  ADDR_W-1:0] output_start,
   input [  ADDR_W-1:0] output_shift,
   input [  ADDR_W-1:0] output_incr,
   input [  ADDR_W-1:0] output_iter2,
   input [PERIOD_W-1:0] output_per2,
   input [  ADDR_W-1:0] output_shift2,
   input [  ADDR_W-1:0] output_incr2,
   input [  ADDR_W-1:0] output_iter3,
   input [PERIOD_W-1:0] output_per3,
   input [  ADDR_W-1:0] output_shift3,
   input [  ADDR_W-1:0] output_incr3,

   input [ 20-1:0]      output_extra_delay,
   input                output_ignore_first,

   input [DELAY_W-1:0] delay0
);

   //assign databus_wdata_0 = 0;
   //assign databus_wstrb_0 = 0;
   //assign databus_len_0   = read_length;

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

   wire [ADDR_W-1:0] constant1 = 1;
   
   //wire [ADDR_W-1:0] gen_addr_temp;
   //wire gen_valid, gen_ready;

   reg [ADDR_W-1:0] gen_addr_temp;
   reg gen_valid;
   wire gen_ready;

   localparam SIZE_W_TEMP = (SIZE_W / 8);
   localparam [ADDR_W-1:0] OFFSET_W = SIZE_W_TEMP[ADDR_W-1:0];

   always @(posedge clk,posedge rst) begin
      if(rst) begin
         gen_addr_temp <= 0;
      end else if(run && read_enabled) begin
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
      .AXI_DATA_W(AXI_DATA_W),
      .LEN_W(LEN_W),
      .COUNT_W(ADDR_W),
      .ADDR_W(ADDR_W),
      .DATA_W(SIZE_W),
      .PERIOD_W(PERIOD_W),
      .DELAY_W(1)
      ) reader (
      .clk_i(clk),
      .rst_i(rst),
      .run_i(run && read_enabled),
      .done_o(transferDone),

      .ignore_first_i(1'b0),

      //configurations 
      //.period_i(read_per),
      //.delay_i (1'b0),
      //.start_i (read_start),
      //.incr_i  (read_incr),

      //.iterations_i(read_iter),
      //.duty_i      (read_duty),
      //.shift_i     (read_shift),

      .period_i({PERIOD_W{1'b0}}),
      .delay_i (1'b0),
      .start_i ({ADDR_W{1'b0}}),
      .incr_i  ({ADDR_W{1'b0}}),

      .iterations_i({ADDR_W{1'b0}}),
      .duty_i      ({PERIOD_W{1'b0}}),
      .shift_i     ({ADDR_W{1'b0}}),

      .period2_i({PERIOD_W{1'b0}}),
      .incr2_i({ADDR_W{1'b0}}),
      .iterations2_i({ADDR_W{1'b0}}),
      .shift2_i({ADDR_W{1'b0}}),

      .period3_i({PERIOD_W{1'b0}}),
      .incr3_i({ADDR_W{1'b0}}),
      .iterations3_i({ADDR_W{1'b0}}),
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
      .data_last_o(),

      .count_i(read_amount_minus_one + constant1),
      .start_address_i(ext_addr),
      .address_shift_i(read_addr_shift),
      .databus_length(read_length)
   );

assign databus_wdata_0 = 0;
assign databus_wstrb_0 = 0;
assign data_valid = databus_ready_0;
assign data_data = databus_rdata_0;

   wire [ADDR_W-1:0] gen_addr = {pingPong ? !pingPongState : gen_addr_temp[ADDR_W-1],gen_addr_temp[ADDR_W-2:0]};

   // mem enables output by addr gen
   wire output_enabled,output_store;

   AddressGen3 #(
      .ADDR_W(ADDR_W),
      .DATA_W(DATA_W),
      .PERIOD_W(PERIOD_W),
      .DELAY_W(21)
   ) addrgenOutput (
      .clk_i(clk),
      .rst_i(rst),
      .run_i(run),

      .ignore_first_i(output_ignore_first),

      //configurations 
      .period_i(output_per),
      .delay_i ({13'b0,delay0} + output_extra_delay),
      .start_i ({1'b0,output_start[ADDR_W-2:0]}),
      .incr_i  (output_incr),

      .iterations_i(output_iter),
      .duty_i      (output_duty),
      .shift_i     (output_shift),

      .period2_i(output_per2),
      .incr2_i(output_incr2),
      .iterations2_i(output_iter2),
      .shift2_i(output_shift2),

      .period3_i(output_per3),
      .incr3_i(output_incr3),
      .iterations3_i(output_iter3),
      .shift3_i(output_shift3),

      //outputs 
      .valid_o(output_enabled),
      .ready_i(1'b1),
      .addr_o (output_addr_temp),
      .store_o(output_store),
      .done_o (doneOutput_int)
   );

   reg [ADDR_W-1:0] last_output_addr;

   always @(posedge clk,posedge rst) begin
      if(rst) begin
         last_output_addr <= 0;
      end 
      else if(output_enabled && output_store) begin
         last_output_addr <= output_addr_temp;
      end
   end

   //wire [ADDR_W-1:0] true_output_addr = output_ignore_first ? last_output_addr : output_addr_temp;
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

      .forceReset(!running),

      .clk_i(clk),
      .rst_i(rst)
   );

   localparam DIFF = AXI_DATA_W / DATA_W;
   localparam DECISION_BIT_W = $clog2(DIFF);

   function [ADDR_W-DECISION_BIT_W-1:0] symbolSpaceConvert(input [ADDR_W-1:0] in);
      reg [ADDR_W-1:0] noPingPong;
      reg [ADDR_W-1:0] shiftRes;
      begin
         noPingPong           = in;
         noPingPong[ADDR_W-1] = 1'b0;
         shiftRes             = noPingPong >> DECISION_BIT_W;
         symbolSpaceConvert   = shiftRes[ADDR_W-DECISION_BIT_W-1:0];
      end
   endfunction

   reg [ADDR_W-1:0] addr_out;

   generate
      if (AXI_DATA_W > DATA_W) begin
         always @* begin
            addr_out                            = 0;
            addr_out[ADDR_W-DECISION_BIT_W-1:0] = symbolSpaceConvert(output_addr[ADDR_W-1:0]);
            addr_out[ADDR_W-1]                  = pingPong && output_addr[ADDR_W-1];
         end

         reg [DECISION_BIT_W-1:0] sel_0;  // Matches addr_0_port_0
         always @(posedge clk, posedge rst) begin
            if (rst) begin
               sel_0 <= 0;
            end else begin
               sel_0 <= output_addr[DECISION_BIT_W-1:0];
            end
         end

         WideAdapter #(
            .INPUT_W (AXI_DATA_W),
            .OUTPUT_W(DATA_W),
            .SIZE_W  (SIZE_W)
         ) adapter (
            .sel_i(sel_0),
            .in_i (ext_2p_data_in_0),
            .out_o(out0)
         );
      end else begin
         always @* begin
            addr_out = output_addr;
         end
         assign out0 = ext_2p_data_in_0;
      end  // if(AXI_DATA_W > DATA_W)
   endgenerate

   assign ext_2p_write_0    = write_en;
   assign ext_2p_addr_out_0 = write_addr;
   assign ext_2p_data_out_0 = write_data;

   assign ext_2p_read_0     = output_enabled;
   assign ext_2p_addr_in_0  = addr_out;

   reg reportedA;
   reg reportedB;
   reg reportedC;

   // Reports most common errors
   always @(posedge clk) begin
      if(run) begin
         reportedA <= 1'b0;
      end else if(pingPong && gen_addr_temp[ADDR_W-1] && reportedA == 1'b0) begin
         $display("%m: Overflow of memory when using PingPong for reading");
         reportedA <= 1'b1;
      end
   end

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
      end else if(pingPong && output_start[ADDR_W-1] && reportedC == 1'b0) begin
         $display("%m: Last bit of output start ignored when using PingPong");
         reportedC <= 1'b1;
      end
   end


endmodule

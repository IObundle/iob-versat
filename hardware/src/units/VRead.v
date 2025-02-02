`timescale 1ns / 1ps

module VRead #(
   parameter SIZE_W     = 32,
   parameter DATA_W     = 32,
   parameter ADDR_W     = 18,
   parameter PERIOD_W   = 18, // Must be 2 less than ADDR_W (boundary of 4) (for 32 bit DATA_W)
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

   (* versat_stage="Read" *) input [  PERIOD_W-1:0] read_per,
   (* versat_stage="Read" *) input [    ADDR_W-1:0] read_incr,
   (* versat_stage="Read" *) input [  PERIOD_W-1:0] read_duty,

   (* versat_stage="Read" *) input [    ADDR_W-1:0] read_iter,
   (* versat_stage="Read" *) input [    ADDR_W-1:0] read_shift,

   (* versat_stage="Read" *) input [     LEN_W-1:0] read_length,

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

   input [DELAY_W-1:0] delay0
);

   assign databus_wdata_0 = 0;
   assign databus_wstrb_0 = 0;
   assign databus_len_0   = read_length;

   // output databus
   reg               doneRead;
   reg               doneOutput;
   wire              doneOutput_int;

   assign done = (doneRead  & doneOutput);

   always @(posedge clk, posedge rst) begin
      if (rst) begin
         doneRead <= 1'b1;
         doneOutput <= 1'b1;
      end else if (run) begin
         doneRead <= !read_enabled;
         doneOutput <= 1'b0;
      end else begin
         if (databus_valid_0 && databus_ready_0 && databus_last_0) doneRead <= 1'b1;
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

   wire [ADDR_W-1:0] gen_addr_temp;
   wire gen_valid, gen_ready;

   always @(posedge clk, posedge rst) begin
      if (rst) databus_addr_0 <= 0;
      else if (run && read_enabled) databus_addr_0 <= ext_addr;
   end

   AddressGen3 #(
      .ADDR_W(ADDR_W),
      .DATA_W(SIZE_W),
      .PERIOD_W(PERIOD_W)
   ) addrgenRead (
      .clk_i(clk),
      .rst_i(rst),
      .run_i(run && read_enabled),

      //configurations 
      .period_i(read_per),
      .delay_i (0),
      .start_i (0),
      .incr_i  (read_incr),

      .iterations_i(read_iter),
      .duty_i      (read_duty),
      .shift_i     (read_shift),

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
      .addr_o (gen_addr_temp),
      .store_o(),
      .done_o ()
   );

   wire [ADDR_W-1:0] gen_addr = {pingPong ? !pingPongState : gen_addr_temp[ADDR_W-1],gen_addr_temp[ADDR_W-2:0]};

   // mem enables output by addr gen
   wire output_enabled;

   AddressGen3 #(
      .ADDR_W(ADDR_W),
      .DATA_W(DATA_W),
      .PERIOD_W(PERIOD_W)
   ) addrgenOutput (
      .clk_i(clk),
      .rst_i(rst),
      .run_i(run),

      //configurations 
      .period_i(output_per),
      .delay_i (delay0),
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
      .store_o(),
      .done_o (doneOutput_int)
   );

   wire [ADDR_W-1:0] output_addr = {pingPong ? pingPongState : output_addr_temp[ADDR_W-1],output_addr_temp[ADDR_W-2:0]};

   wire                  write_en;
   wire [    ADDR_W-1:0] write_addr;
   wire [AXI_DATA_W-1:0] write_data;

   wire                  data_ready;

   JoinTwoHandshakes #(
      .FIRST_DATA_W(ADDR_W),
      .SECOND_DATA_W(AXI_DATA_W)
   ) writer (
      .first_valid_i(gen_valid),
      .first_ready_o(gen_ready),
      .first_data_i(gen_addr),

      .second_valid_i(databus_ready_0),
      .second_ready_o(data_ready),
      .second_data_i(databus_rdata_0),

      .result_valid_o(write_en),
      .result_ready_i(1'b1),
      .result_first_data_o(write_addr),
      .result_second_data_o(write_data),

      .clk_i(clk),
      .rst_i(rst)
   );

   assign databus_valid_0 = (data_ready && !doneRead);

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

   // Reports most common errors
   always @* begin
      if(pingPong && gen_addr_temp[ADDR_W-1]) begin
         $display("%m: Overflow of memory when using PingPong for reading");
      end
      if(pingPong && output_addr_temp[ADDR_W-1]) begin
         $display("%m: Overflow of write memory when using PingPong for outputting");
      end
      if(pingPong && output_start[ADDR_W-1]) begin
         $display("%m: Last bit of output start ignored when using PingPong");
      end
   end

endmodule

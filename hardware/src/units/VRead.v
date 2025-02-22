`timescale 1ns / 1ps

module VRead #(
   parameter SIZE_W     = 32,
   parameter DATA_W     = 32,
   parameter ADDR_W     = 20,
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

   input [DELAY_W-1:0] delay0
);

// VRead is basically a VReadMultiple with only one transfer.
// 

 VReadMultiple #(
   .SIZE_W(SIZE_W),
   .DATA_W(DATA_W),
   .ADDR_W(ADDR_W),
   .PERIOD_W(PERIOD_W),
   .AXI_ADDR_W(AXI_ADDR_W),
   .AXI_DATA_W(AXI_DATA_W),
   .DELAY_W(DELAY_W),
   .LEN_W(LEN_W)
) actualReader (
   .clk(clk),
   .rst(rst),
   .running(running),
   .run(run),
   .done(done),

   // Databus interface
   .databus_ready_0(databus_ready_0),
   .databus_valid_0(databus_valid_0),
   .databus_addr_0(databus_addr_0),
   .databus_rdata_0(databus_rdata_0),
   .databus_wdata_0(databus_wdata_0),
   .databus_wstrb_0(databus_wstrb_0),
   .databus_len_0(databus_len_0),
   .databus_last_0(databus_last_0),

   .out0(out0),

   .ext_2p_addr_out_0(ext_2p_addr_out_0),
   .ext_2p_addr_in_0(ext_2p_addr_in_0),
   .ext_2p_write_0(ext_2p_write_0),
   .ext_2p_read_0(ext_2p_read_0),
   .ext_2p_data_in_0(ext_2p_data_in_0),
   .ext_2p_data_out_0(ext_2p_data_out_0),

   .ext_addr(ext_addr),
   .pingPong(pingPong),
   .read_start(0),
   .read_per(read_per),
   .read_incr(read_incr),
   .read_duty(read_duty),
   .read_iter(read_iter),
   .read_shift(read_shift),

   .read_amount_minus_one(0),
   .read_length(read_length),
   .read_addr_shift(0),

   .read_enabled(read_enabled),
   .output_iter(output_iter),
   .output_per(output_per),
   .output_duty(output_duty),
   .output_start(output_start),
   .output_shift(output_shift),
   .output_incr(output_incr),
   .output_iter2(output_iter2),
   .output_per2(output_per2),
   .output_shift2(output_shift2),
   .output_incr2(output_incr2),
   .output_iter3(0),
   .output_per3(0),
   .output_shift3(0),
   .output_incr3(0),
   .output_extra_delay(0),
   .output_ignore_first(0),
   .delay0(delay0)
);

endmodule

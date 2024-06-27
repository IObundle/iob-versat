`timescale 1ns / 1ps

module DebugReadMem #(
   parameter DATA_W     = 32,
   parameter ADDR_W     = 14,
   parameter AXI_ADDR_W = 32,
   parameter AXI_DATA_W = 32,
   parameter LEN_W      = 8
) (
   input clk,
   input rst,

   input  running,
   input  run,
   output reg done,
   input signal_loop,

   // Databus interface
   input                     databus_ready_0,
   output reg                databus_valid_0,
   output [  AXI_ADDR_W-1:0] databus_addr_0,
   input  [  AXI_DATA_W-1:0] databus_rdata_0,
   output [  AXI_DATA_W-1:0] databus_wdata_0,
   output [AXI_DATA_W/8-1:0] databus_wstrb_0,
   output [       LEN_W-1:0] databus_len_0,
   input                     databus_last_0,

   input [AXI_ADDR_W-1:0]    address,

   output reg [DATA_W-1:0]   lastRead
);

   assign databus_addr_0  = address;
   assign databus_wstrb_0 = 0;
   assign databus_len_0   = 4;
   assign databus_wdata_0 = 0;

   always @(posedge clk, posedge rst) begin
      if (rst) begin
         databus_valid_0 <= 1'b0;
         lastRead <= 0;
         done <= 1'b1;
      end else if (run) begin
         databus_valid_0 <= 1'b1;
         lastRead <= 0;
         done <= 1'b0;
      end else if (running) begin
         if(databus_ready_0) begin
            databus_valid_0 <= 1'b0;
            lastRead <= databus_rdata_0;
            done <= 1'b1;
         end
      end
   end

endmodule  // DebugReadMem


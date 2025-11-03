`timescale 1ns / 1ps

module DebugWriteMem #(
   parameter AXI_ADDR_W = 32,
   parameter AXI_DATA_W = 32,
   parameter LEN_W      = 8
) (
   input clk,
   input rst,

   input  running,
   input  run,
   output reg done,

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
   input [31:0]              content,
   input [LEN_W-1:0]         length
);

   assign databus_addr_0  = address;
   assign databus_wstrb_0 = {(AXI_DATA_W/8){1'b1}};
   assign databus_len_0   = length;
   assign databus_wdata_0 = content;

   always @(posedge clk, posedge rst) begin
      if (rst) begin
         databus_valid_0 <= 1'b0;
         done <= 1'b1;
      end else if (run && length != 0) begin
         databus_valid_0 <= 1'b1;
         done <= 1'b0;
      end else if (running) begin
         if(databus_ready_0 && databus_last_0) begin
            databus_valid_0 <= 1'b0;
            done <= 1'b1;
         end
      end
   end

endmodule  // DebugReadMem
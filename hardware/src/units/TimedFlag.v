`timescale 1ns / 1ps

module TimedFlag #(
   parameter DATA_W = 32,
   parameter ADDR_W = 10,
   parameter MEM_ADDR_W = 8
   )(
   input                  clk,
   input                  rst,

   input                  running,
   input                  run,
   output reg             done,

   //databus interface
   input [DATA_W/8-1:0]          wstrb,
   input [ADDR_W-1:0]            addr,
   input [DATA_W-1:0]            wdata,
   input                         valid,
   output reg                    ready,
   output [DATA_W-1:0]           rdata,

   input [31:0]                  amount,

   // read port
   output [ADDR_W-1:0]   ext_dp_addr_0_port_0,
   output [DATA_W-1:0]   ext_dp_out_0_port_0,
   input  [DATA_W-1:0]   ext_dp_in_0_port_0,
   output                ext_dp_enable_0_port_0,
   output                ext_dp_write_0_port_0,

   // connect directly to databus
   output [ADDR_W-1:0]   ext_dp_addr_0_port_1,
   output [DATA_W-1:0]   ext_dp_out_0_port_1,
   input  [DATA_W-1:0]   ext_dp_in_0_port_1,
   output                ext_dp_enable_0_port_1,
   output                ext_dp_write_0_port_1,

   input [31:0]          in0,

   (* versat_latency = 1 *) output reg [31:0] out0, // Current value
   (* versat_latency = 1 *) output reg [31:0] out1, // Signal indicating change

   input [31:0] delay0
);

assign ext_dp_addr_0_port_1 = addr;
assign ext_dp_out_0_port_1 = wdata;
assign ext_dp_enable_0_port_1 = valid;
assign ext_dp_write_0_port_1 = valid;

always @(posedge clk,posedge rst) begin
   if(rst) begin
      ready <= 1'b0;
   end if(valid) begin
      ready <= 1'b1;
   end else begin
      ready <= 1'b0;
   end
end

assign ext_dp_write_0_port_0 = 1'b0;

reg [31:0] cycle;
wire [31:0] currentValue;

wire loadNext;
reg nextValue;

MemoryScanner #(.DATA_W(DATA_W),.ADDR_W(ADDR_W)) scanner(
   .addr(ext_dp_addr_0_port_0),
   .dataIn(ext_dp_in_0_port_0),
   .enable(ext_dp_enable_0_port_0),
   
   .nextValue(nextValue),
   .currentValue(currentValue),

   .reset(run),

   .clk(clk),
   .rst(rst)
   );

reg [31:0] needToSee;
assign done = (needToSee == 0);

assign nextValue = (running && cycle == currentValue);

reg [31:0] delay;
always @(posedge clk,posedge rst) begin
   if(rst) begin
      cycle <= 0;
      needToSee <= 0;
   end else if(run) begin
      cycle <= 0;
      needToSee <= 0;
      delay <= delay0;
      needToSee <= amount;
   end else if(|delay) begin
      delay <= delay - 1;
   end else if(delay == 0 && needToSee != 0) begin
      cycle <= cycle + 1;
      if(cycle == currentValue) begin
         out0 <= 1;
         needToSee <= needToSee - 1;
      end else begin
         out0 <= 0;
      end
   end
end

endmodule

module MemoryScanner #(
      parameter DATA_W = 32,
      parameter ADDR_W = 10
   )(
   output reg [ADDR_W-1:0] addr,
   input  [DATA_W-1:0]     dataIn,
   output                  enable,
   
   input               nextValue,
   output [DATA_W-1:0] currentValue,

   input reset,

   input clk,
   input rst
);

reg hasValueStored;

assign currentValue = dataIn;

assign enable = nextValue || (!nextValue && !hasValueStored);

always @(posedge clk,posedge rst) begin
   if(rst) begin
      addr <= 0;
   end else if(reset) begin
      addr <= 0;
      hasValueStored <= 1'b0;
   end else begin
      if(enable) begin
         hasValueStored <= 1'b1;
      end
      if(nextValue) begin
         addr <= addr + 1;
      end
   end
end

endmodule

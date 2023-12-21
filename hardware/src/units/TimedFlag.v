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
   output reg                    rvalid,
   output [DATA_W-1:0]           rdata,

   input [31:0]                  amount,
   input [31:0]                  period,
   input                         disabled,

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

   (* versat_latency = 1 *) output reg [31:0] out0, // Signal indicating change

   input [31:0] delay0
);

assign ext_dp_addr_0_port_1 = addr;
assign ext_dp_out_0_port_1 = wdata;
assign ext_dp_enable_0_port_1 = valid;
assign ext_dp_write_0_port_1 = valid;

// Does not allow memory mapped to read data.
assign rdata = 0;

always @(posedge clk,posedge rst) begin
   if(rst) begin
      rvalid <= 1'b0;
   end if(valid && wstrb == 0) begin
      rvalid <= 1'b1;
   end else begin
      rvalid <= 1'b0;
   end
end

assign ext_dp_write_0_port_0 = 1'b0;

//reg [31:0] cycle;
wire [31:0] currentValue;

wire loadNext;
reg nextValue;

MemoryScanner #(.DATA_W(DATA_W),.ADDR_W(ADDR_W)) scanner(
   .addr_o(ext_dp_addr_0_port_0),
   .dataIn_i(ext_dp_in_0_port_0),
   .enable_o(ext_dp_enable_0_port_0),
   
   .nextValue_i(nextValue),
   .currentValue_o(currentValue),

   .reset_i(run),

   .clk_i(clk),
   .rst_i(rst)
   );

reg [31:0] needToSee;
assign done = (needToSee == 0);

assign nextValue = (running && in0 == currentValue);
assign out0 = ((delay == 0 && !disabled) ? {32{(in0 == currentValue)}} : 0);

reg [31:0] delay;
always @(posedge clk,posedge rst) begin
   if(rst) begin
      needToSee <= 0;
   end else if(run && !disabled) begin
      delay <= delay0;
      needToSee <= amount;
   end else if(|delay) begin
      delay <= delay - 1;
   end else if(delay == 0 && needToSee != 0) begin
      if(in0 == currentValue) begin
         needToSee <= needToSee - 1;
      end
   end
end

endmodule

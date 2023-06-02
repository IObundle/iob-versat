`timescale 1ns / 1ps

(* source *) module SignalMemStorage #(
         parameter MEM_INIT_FILE="none",
         parameter DATA_W = 32,
         parameter ADDR_W = 12
              )
    (
   //control
   input                         clk,
   input                         rst,

   input                         running,   
   input                         run,
   
   //databus interface
   input [DATA_W/8-1:0]          wstrb,
   input [ADDR_W-1:0]            addr,
   input [DATA_W-1:0]            wdata,
   input                         valid,
   output reg                    ready,
   output [DATA_W-1:0]           rdata,

   //input / output data
   input [DATA_W-1:0]            in0, // Data
   input [DATA_W-1:0]            in1, // Signal

   // External memory
   // Memory writter
   output reg [ADDR_W-1:0]       ext_dp_addr_0_port_0,
   output reg [DATA_W-1:0]       ext_dp_out_0_port_0,
   input      [DATA_W-1:0]       ext_dp_in_0_port_0,
   output reg                    ext_dp_enable_0_port_0,
   output                        ext_dp_write_0_port_0,

   // Memory mapping reader
   output [ADDR_W-1:0]           ext_dp_addr_0_port_1,
   output [DATA_W-1:0]           ext_dp_out_0_port_1,
   input  [DATA_W-1:0]           ext_dp_in_0_port_1,
   output                        ext_dp_enable_0_port_1,
   output                        ext_dp_write_0_port_1,

   input [31:0]                  delay0,

   // Configuration
   output reg [ADDR_W-1:0]             stored
   );

reg [31:0] delay;

assign ext_dp_write_0_port_0 = 1'b1;

assign ext_dp_addr_0_port_1 = addr;
assign rdata = ext_dp_in_0_port_1;
assign ext_dp_enable_0_port_1 = valid;
assign ext_dp_write_0_port_1 = 1'b0;

always @(posedge clk,posedge rst)
begin
   ext_dp_addr_0_port_0 <= 0;
   ext_dp_out_0_port_0 <= 0;
   ext_dp_enable_0_port_0 <= 0;

   if(rst) begin
      delay <= 0;      
      stored <= 0;
   end else if(run) begin
      delay <= delay0;
      stored <= 0;
   end else if(|delay) begin
      delay <= delay - 1;
   end else begin
      if(|in1) begin
         stored <= stored + 1;
         ext_dp_addr_0_port_0 <= stored;
         ext_dp_out_0_port_0 <= in0;
         ext_dp_enable_0_port_0 <= 1;
      end
   end
end

reg ready_delay0,ready_delay1;

always @(posedge clk,posedge rst)
begin
   if(rst) begin
      ready_delay1 <= 0;
      ready_delay0 <= 0;
      ready <= 0;
   end else begin   
      ready_delay0 <= valid;
      ready_delay1 <= valid;
      ready <= ready_delay1;
   end
end

endmodule

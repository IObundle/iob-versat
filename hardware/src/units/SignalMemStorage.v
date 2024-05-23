`timescale 1ns / 1ps

(* source *) module SignalMemStorage #(
   parameter MEM_INIT_FILE = "none",
   parameter DATA_W        = 32,
   parameter DELAY_W       = 7,
   parameter ADDR_W        = 12
) (
   //control
   input clk,
   input rst,

   input running,
   input run,

   //databus interface
   input      [DATA_W/8-1:0] wstrb,
   input      [  ADDR_W-1:0] addr,
   input      [  DATA_W-1:0] wdata,
   input                     valid,
   output reg                rvalid,
   output     [  DATA_W-1:0] rdata,

   //input / output data
   input [DATA_W-1:0] in0,  // Data
   input [DATA_W-1:0] in1,  // Signal

   // External memory
   // Memory writter
   output reg [ADDR_W-1:0] ext_dp_addr_0_port_0,
   output reg [DATA_W-1:0] ext_dp_out_0_port_0,
   input      [DATA_W-1:0] ext_dp_in_0_port_0,
   output reg              ext_dp_enable_0_port_0,
   output                  ext_dp_write_0_port_0,

   // Memory mapping reader
   output [ADDR_W-1:0] ext_dp_addr_0_port_1,
   output [DATA_W-1:0] ext_dp_out_0_port_1,
   input  [DATA_W-1:0] ext_dp_in_0_port_1,
   output              ext_dp_enable_0_port_1,
   output              ext_dp_write_0_port_1,

   input [DELAY_W-1:0] delay0,

   input pingPong,

   // Configuration
   output [ADDR_W-1:0] stored
);

   reg [DELAY_W-1:0] delay;

   reg [ADDR_W-1:0] currentStored;
   reg [ADDR_W-1:0] lastStored;

   assign stored = pingPong ? lastStored : currentStored;

   reg pingPongState;

   // Ping pong 
   always @(posedge clk, posedge rst) begin
      if (rst) pingPongState <= 0;
      else if (run) pingPongState <= pingPong ? (!pingPongState) : 1'b0;
   end

   assign ext_dp_write_0_port_0  = 1'b1;

   assign ext_dp_addr_0_port_1   = pingPong ? {pingPongState, addr[ADDR_W-2:0]} : addr;
   assign rdata                  = rvalid ? ext_dp_in_0_port_1 : 0;
   assign ext_dp_enable_0_port_1 = valid;
   assign ext_dp_write_0_port_1  = 1'b0;

   reg [ADDR_W-1:0] port_0_address;
   assign ext_dp_addr_0_port_0 = pingPong ? {!pingPongState,port_0_address[ADDR_W-2:0]} : port_0_address;

   always @(posedge clk, posedge rst) begin
      port_0_address         <= 0;
      ext_dp_out_0_port_0    <= 0;
      ext_dp_enable_0_port_0 <= 0;

      if (rst) begin
         delay         <= 0;
         currentStored <= 0;
      end else if (run) begin
         delay         <= delay0;
         lastStored    <= currentStored;
         currentStored <= 0;
      end else if (|delay) begin
         delay <= delay - 1;
      end else begin
         if (|in1) begin
            currentStored          <= currentStored + 1;
            port_0_address         <= currentStored;
            ext_dp_out_0_port_0    <= in0;
            ext_dp_enable_0_port_0 <= 1;
         end
      end
   end

   reg rvalid_delay0, rvalid_delay1;

   always @(posedge clk, posedge rst) begin
      if (rst) begin
         rvalid_delay1 <= 0;
         rvalid_delay0 <= 0;
         rvalid        <= 0;
      end else begin
         if (wstrb == 0) rvalid_delay0 <= valid;
         rvalid_delay1 <= rvalid_delay0;
         rvalid        <= rvalid_delay1;
      end
   end

endmodule

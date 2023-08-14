`timescale 1ns / 1ps

// A transfer is equal to a N amount of AXI bursts intended to transfer a variable amount of bytes.
// 

`default_nettype none
module transfer_controller #(
   parameter AXI_ADDR_W = 32,
   parameter AXI_DATA_W = 32,
   parameter LEN_W = 32
   )
   (
      input [AXI_ADDR_W-1:0] address, // Must remain constant during transfer
      input [LEN_W-1:0]      length, // In bytes

      input transfer_start, // Assert one cycle before starting a transfer. Set address and length to the correct values beforehand
      input burst_start,   // At the start of a burst (after values accepted by axi), assert this signal for one cycle. This updates the stored values and prepares the values of the next cycle

      output reg [AXI_ADDR_W-1:0] true_axi_axaddr, /* registered */

      // All these signals are combinatorial. Register them outside this module if needed
      output reg [AXI_DATA_W/8-1:0] initial_strb, // First strobe of the transfer. The rest is always full 1
      output reg [AXI_DATA_W/8-1:0] final_strb,   // Last strobe of the transfer. Only valid if last_transfer is asserted

      output [31:0] symbolsToRead, // How many symbols (data transfers) to expect from the source

      output reg [7:0] true_axi_axlen,
      output reg last_transfer,
   
      input clk,
      input rst
   );

localparam OFFSET_W = calculate_AXI_OFFSET_W(AXI_DATA_W);
localparam STROBE_W = AXI_DATA_W/8;

wire [AXI_DATA_W-1:0] OFFSET_MASK = (0 | {OFFSET_W{1'b1}});

function [15:0] min(input [15:0] a,b);
    begin
        min = (a < b) ? a : b;
    end
endfunction

// State variables
reg [LEN_W-1:0] stored_len;
reg first_transfer;

// Generic variables outside generate
reg [LEN_W-1:0] transfer_byte_size;
reg [LEN_W-1:0] last_transfer_len;

wire [12:0] max_transfer_len; // Assigned inside the generate
wire [12:0] max_transfer_len_minus_one; // Assigned inside the generate

wire [12:0] first_transfer_len = (max_transfer_len_minus_one + (STROBE_W - address[OFFSET_W-1:0]));
wire [12:0] boundary_transfer_len = (13'h1000 - true_axi_axaddr[11:0]);
wire [AXI_ADDR_W-1:0] true_address = address & (~OFFSET_MASK);

wire last_transfer_next = (transfer_byte_size == stored_len);

always @* begin
   if(first_transfer)
      transfer_byte_size = min(boundary_transfer_len,min(stored_len,first_transfer_len));
   else
      transfer_byte_size = min(boundary_transfer_len,min(stored_len,max_transfer_len));

   if(last_transfer_next)
      true_axi_axlen = last_transfer_len;
   else
      true_axi_axlen = (transfer_byte_size - 1) >> OFFSET_W;
end

integer i;
always @*
begin
   initial_strb = 0;
   for(i = 0; i < (AXI_DATA_W/8); i = i + 1) begin
      if(i >= address[OFFSET_W-1:0]) initial_strb[i] = 1'b1;
   end

   final_strb = ~0;
   for(i = 0; i < (AXI_DATA_W/8); i = i + 1) begin
      if(i >= address[OFFSET_W-1:0]) final_strb[i] = 1'b0;
   end
end

generate
if(AXI_DATA_W == 32) begin
assign symbolsToRead = length >> 2;
assign max_transfer_len = 13'h0400;
assign max_transfer_len_minus_one = 13'h03FC;
   always @* begin
      last_transfer_len = 0;

      if(address[1:0] == 2'b00 & stored_len[1:0] == 2'b00)
         last_transfer_len = stored_len[9:2] - 8'h1;
      else if((address[1:0] == 2'b10 && stored_len[1:0] == 2'b11) || (address[1:0] == 2'b11 && stored_len[1:0] >= 2'b10))
         last_transfer_len = stored_len[9:2] + 8'h1;
      else
         last_transfer_len = stored_len[9:2];
   end
end // if(AXI_DATA_W == 32)
if(AXI_DATA_W == 64) begin
assign symbolsToRead = length >> 3;
assign max_transfer_len = 13'h0800;
assign max_transfer_len_minus_one = 13'h07F8;
wire [2:0] address_plus_len = address[2:0] + stored_len[2:0];
   always @* begin
      last_transfer_len = 0;

      if(address[2:0] == 3'b000 && stored_len[2:0] == 3'b000)
         last_transfer_len = stored_len[10:3] - 8'h1;
      else if(!(address_plus_len >= address[2:0] || address_plus_len == 0))
         last_transfer_len = stored_len[10:3] + 8'h1;
      else
         last_transfer_len = stored_len[10:3];
   end
end // if(AXI_DATA_W == 64)
if(AXI_DATA_W == 128) begin
assign symbolsToRead = length >> 4;
assign max_transfer_len = 13'h1000;
assign max_transfer_len_minus_one = 13'h0FF0;
wire [3:0] address_plus_len = address[3:0] + stored_len[3:0];
   always @* begin
      last_transfer_len = 0;

      if(address[3:0] == 4'b0000 && stored_len[3:0] == 4'b0000)
         last_transfer_len = stored_len[11:4] - 8'h1;
      else if(!(address_plus_len >= address[3:0] || address_plus_len == 0))
         last_transfer_len = stored_len[11:4] + 8'h1;
      else
         last_transfer_len = stored_len[11:4];

   end
end // if(AXI_DATA_W == 128)
if(AXI_DATA_W == 256) begin
assign symbolsToRead = length >> 5;
assign max_transfer_len = 13'h1000;
assign max_transfer_len_minus_one = 13'h0FE0; // Because of boundary conditions, cannot go higher
wire [4:0] address_plus_len = address[4:0] + stored_len[4:0];
   always @* begin
      last_transfer_len = 0;

      if(address[4:0] == 5'b0000 && stored_len[4:0] == 5'b0000)
         last_transfer_len = stored_len[12:5] - 8'h1;
      else if(!(address_plus_len >= address[4:0] || address_plus_len == 0))
         last_transfer_len = stored_len[12:5] + 8'h1;
      else
         last_transfer_len = stored_len[12:5];

   end
end
if(AXI_DATA_W == 512) begin
assign symbolsToRead = length >> 5;
assign max_transfer_len = 13'h1000;
assign max_transfer_len_minus_one = 13'h0FC0; // Because of boundary conditions, cannot go higher
wire [5:0] address_plus_len = address[5:0] + stored_len[5:0];
   always @* begin
      last_transfer_len = 0;

      if(address[5:0] == 6'b0000 && stored_len[5:0] == 6'b0000)
         last_transfer_len = stored_len[13:6] - 8'h1;
      else if(!(address_plus_len >= address[5:0] || address_plus_len == 0))
         last_transfer_len = stored_len[13:6] + 8'h1;
      else
         last_transfer_len = stored_len[13:6];
   end
end
endgenerate

// Outside generate, logic is generic enough
always @(posedge clk,posedge rst) begin
   if(rst) begin
      first_transfer <= 1'b0;
      stored_len <= 0;
      last_transfer <= 0;
      true_axi_axaddr <= 0;
   end else begin
      if(transfer_start) begin
         first_transfer <= 1'b1;
         last_transfer <= last_transfer_next;
         true_axi_axaddr <= true_address;
         stored_len <= length;
      end
      if(burst_start) begin
         first_transfer <= 1'b0;
         last_transfer <= last_transfer_next;
         true_axi_axaddr <= true_axi_axaddr + (true_axi_axlen + 1) * (AXI_DATA_W / 8);
         stored_len <= stored_len - transfer_byte_size;
      end
   end
end

endmodule
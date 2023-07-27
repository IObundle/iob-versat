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
      output reg [AXI_DATA_W/8-1:0] final_strb,   // Last strobe of the transfer.

      output reg [7:0] true_axi_axlen,
      output reg last_transfer,
   
      input clk,
      input rst
   );

function [15:0] min(input [15:0] a,b);
    begin
        min = (a < b) ? a : b;
    end
endfunction

// State variables
reg [LEN_W-1:0] stored_len;
reg first_transfer;

// Generic variables outside generate
reg last_transfer_next;
reg [LEN_W-1:0] transfer_byte_size;

wire [AXI_ADDR_W-1:0] true_address;

//generate
//if(AXI_DATA_W == 32) begin

reg [15:0] boundary_transfer_len;
reg [LEN_W-1:0] last_transfer_len;

assign true_address = {address[AXI_ADDR_W-1:2],2'b00};

wire [15:0] first_transfer_len = (32'd1020 + (32'd4 - address[1:0]));

always @* begin
   last_transfer_len = 0;

   if(address[1:0] == 2'b00 & stored_len[1:0] == 2'b00)
      last_transfer_len = stored_len[9:2] - 8'h1;
   else if((address[1:0] == 2'b10 && stored_len[1:0] == 2'b11) || (address[1:0] == 2'b11 && stored_len[1:0] >= 2'b10))
      last_transfer_len = stored_len[9:2] + 8'h1;
   else
      last_transfer_len = stored_len[9:2];

    boundary_transfer_len = 16'h1000 - address[11:0]; // Maximum bytes that can be transfer before crossing a border

    if(first_transfer)
        transfer_byte_size = min(boundary_transfer_len,min(stored_len,first_transfer_len));
    else
        transfer_byte_size = min(boundary_transfer_len,stored_len);

    last_transfer_next = (transfer_byte_size == stored_len);

    if(last_transfer_next)
        true_axi_axlen = last_transfer_len;
    else
        true_axi_axlen = (transfer_byte_size - 1) >> 2;

    case(address[1:0])
        2'b00: initial_strb = 4'b1111;
        2'b01: initial_strb = 4'b1110;
        2'b10: initial_strb = 4'b1100;
        2'b11: initial_strb = 4'b1000;
    endcase
    
    case(address[1:0])
        2'b00: case(stored_len[1:0])
            2'b00: final_strb = 4'b1111; 
            2'b01: final_strb = 4'b0001;
            2'b10: final_strb = 4'b0011;
            2'b11: final_strb = 4'b0111;
        endcase
        2'b01: case(stored_len[1:0])
            2'b00: final_strb = 4'b0001; 
            2'b01: final_strb = 4'b0011;
            2'b10: final_strb = 4'b0111;
            2'b11: final_strb = 4'b1111;
        endcase
        2'b10: case(stored_len[1:0])
            2'b00: final_strb = 4'b0011; 
            2'b01: final_strb = 4'b0111;
            2'b10: final_strb = 4'b1111;
            2'b11: final_strb = 4'b0001;
        endcase
        2'b11: case(stored_len[1:0])
            2'b00: final_strb = 4'b0111; 
            2'b01: final_strb = 4'b1111;
            2'b10: final_strb = 4'b0001;
            2'b11: final_strb = 4'b0011;
        endcase
    endcase
end

//end // if(AXI_DATA_W == 32)
//endgenerate

// Outside generate, logic is generic enough
always @(posedge clk,posedge rst) begin
   if(rst) begin
      first_transfer <= 1'b0;
      stored_len <= 0;
      last_transfer <= 0;
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
         true_axi_axaddr <= true_axi_axaddr + (true_axi_axlen + 1) * 4;
         stored_len <= stored_len - transfer_byte_size;
      end
   end
end

endmodule
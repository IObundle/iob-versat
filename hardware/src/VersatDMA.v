`timescale 1ns / 1ps

// Simple DMA that uses databus connections to move data between versat and memory.

module VersatDMA #(
      parameter ADDR_W = 32,
      parameter DATA_W = 32,
      parameter LEN_W  = 20
   )(

   input      [LEN_W-1:0]    dma_length_i,
   input      [ADDR_W-1:0]   dma_address_inside_i,
   input      [ADDR_W-1:0]   dma_address_outside_i,

   output reg [ADDR_W-1:0]   dma_address_o,
   output     [DATA_W-1:0]   dma_data_o,
   output                    dma_valid_o,

   input                     databus_ready_0,
   output reg                databus_valid_0,
   output reg [  ADDR_W-1:0] databus_addr_0,
   input      [  DATA_W-1:0] databus_rdata_0,
   output     [  DATA_W-1:0] databus_wdata_0,
   output reg [DATA_W/8-1:0] databus_wstrb_0,
   output reg [LEN_W-1:0]    databus_len_0,
   input                     databus_last_0,

   input                     dma_rvalid,
   input  [  DATA_W-1:0]     dma_rdata,

   input write_not_read_i,
   input start_i,
   output done_o,

   input clk_i,
   input rst_i
   );


reg [1:0] dma_state;
assign done_o = (dma_state == 2'b00);
assign dma_data_o = databus_rdata_0;
assign dma_valid_o = databus_ready_0 && databus_valid_0;
assign databus_wdata_0 = dma_rdata;

always @(posedge clk_i,posedge rst_i) begin
   if(rst_i) begin
      dma_state <= 0;
      dma_address_o <= 0;
   end else begin
         case (dma_state)
         2'b00: begin
            if(start_i) begin
               dma_address_o <= dma_address_inside_i;
               if(write_not_read_i) begin
                  dma_state <= 2'b10;   
               end else begin
                  dma_state <= 2'b01;
               end
            end
         end
         2'b01: begin
            if (databus_ready_0 && databus_last_0) begin
               dma_state <= 0;
            end
            if (databus_ready_0) begin
               dma_address_o <= dma_address_o + 4;
            end
         end
         2'b10: begin // Write
         end
         2'b11: begin
         end
      endcase
   end
end

always @* begin
   databus_valid_0 = 1'b0;
   databus_addr_0  = dma_address_outside_i;
   databus_wstrb_0 = 0;  // Only read, for now
   databus_len_0   = dma_length_i;

   case (dma_state)
      2'b00: ;
      2'b01: begin
         databus_valid_0 = 1'b1;
      end
      2'b10: begin
      end
      2'b11: begin
      end
   endcase
end

endmodule

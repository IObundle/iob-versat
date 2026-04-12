`timescale 1ns / 1ps

// A fifo that does not instantiate block mem. It is mostly used for its simple FIFO like interface
// since some portions of the code are simpler when thinking in terms of fifo.
// Instantiating this with a high COUNT is probably not recommended since 

module SimpleFifo #(
   parameter DATA_W = 32,
   parameter COUNT = 1, // Max amount of elements to hold.

   parameter ADDR_W = $clog2(COUNT)
) (
   input clk_i,
   input rst_i,

   //write port
   input               w_en_i,
   input  [DATA_W-1:0] w_data_i,
   output              w_full_o,

   //read port
   input               r_en_i,
   output [DATA_W-1:0] r_data_o,
   output              r_empty_o,

   //FIFO level
   output [ADDR_W-1:0] level_o
);

reg [DATA_W-1:0] mem[(2**(ADDR_W))-1:0];
reg [ADDR_W-1:0] read;
reg [ADDR_W-1:0] write;

wire [ADDR_W+1-1:0] high_occupancy = {1'b1,write} - {1'b0,read};
assign level_o = high_occupancy[ADDR_W-1:0];

assign w_full_o = (high_occupancy >= COUNT);
assign r_empty_o = (high_occupancy == 0);

always @(posedge clk_i,posedge rst_i) begin
   if(rst_i) begin
      read <= 0;
      write <= 0;
   end else begin
      if(w_en_i) begin
         mem[write] <= w_data_i;
         write <= write + 1;
      end

      if(r_en_i) begin
         read <= read + 1;
      end
   end
end

assign r_data_o = mem[read];

endmodule

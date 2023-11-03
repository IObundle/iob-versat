`timescale 1ns / 1ps
// Comment so that verible-format will not put timescale and defaultt_nettype into same line
`default_nettype none

// Generates an address for a 8 bit memory. Byte space
module SimpleAddressGen 
  #(
    parameter ADDR_W = 10,
    parameter PERIOD_W = 10,
    parameter DELAY_W = 32,
    parameter DATA_W = 32 // Size of data. Addr is in byte space and the size of data tells how much to increment. A size of 32 with an incr of 1 leads to an increase of addr by 4.
    ) (
       input                       clk_i,
       input                       rst_i,

       input                       run_i,

       //configurations 
       input [PERIOD_W - 1:0]      period_i,
       input [DELAY_W - 1:0]       delay_i,
       input [ADDR_W - 1:0]        start_i,
       input signed [ADDR_W - 1:0] incr_i,

       //outputs 
       output reg                  valid_o,
       input                       ready_i,
       output reg [ADDR_W - 1:0]   addr_o,

       //output reg                  mem_en_o,
       output reg                  done_o
       );

localparam OFFSET_W = $clog2(DATA_W/8);

reg [DELAY_I_W-1:0] delay_iCounter;

reg [PERIOD_I_W - 1:0] per;

wire perCond = (per + 1) >= period_i; /* Do not know why there was a "period_i >> OFFSET_W" */

always @(posedge clk_i,posedge rst_i)
begin
   if(rst_i) begin
      delay_iCounter <= 0;
      addr_o <= 0;
      per <= 0;
      valid_o <= 0;
      done_o <= 1'b0;
   end else if(run_i) begin
      delay_iCounter <= delay_i;
      addr_o <= start_i;
      per <= 0;
      valid_o <= 0;
      done_o <= 1'b0;
      if(delay_i == 0) begin
         valid_o <= 1'b1;
      end
   end else if(|delay_iCounter) begin
      delay_iCounter <= delay_iCounter - 1;
      valid_o <= 1'b1;
   end else if(valid_o && ready_i) begin
      if(perCond) begin
         per <= 0;
         done_o <= 1'b1;
         valid_o <= 0;
      end else  begin
         addr_o <= addr_o + (incr_i << OFFSET_W);
         per <= per + 1;
      end
   end
end

endmodule // SimpleAddressGen

`default_nettype wire

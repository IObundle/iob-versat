`timescale 1ns / 1ps

// Generates an address for a 8 bit memory. Byte space
module SimpleAddressGen # (
   parameter ADDR_W = 10,
   parameter PERIOD_W = 10,
   parameter DELAY_W = 32,
   parameter DATA_W = 32 // Size of data. Addr is in byte space and the size of data tells how much to increment. A size of 32 with an incr of 1 leads to an increase of addr by 4.
   ) (
   input                          clk,
   input                          rst,

   input                          run,

   //configurations 
   input [PERIOD_W - 1:0]         period,
   input [DELAY_W - 1:0]          delay,
   input [ADDR_W - 1:0]           start,
   input signed [ADDR_W - 1:0]    incr,

   //outputs 
   output reg                     valid,
   input                          ready,
   output reg [ADDR_W - 1:0]      addr,

   //output reg                   mem_en,
   output reg                     done
);

localparam OFFSET_W = $clog2(DATA_W/8);

reg [DELAY_W-1:0] delayCounter;

reg [PERIOD_W - 1:0] per;

wire perCond = ((per + 1) >= (period >> OFFSET_W));

always @(posedge clk,posedge rst)
begin
   if(rst) begin
      delayCounter <= 0;
      addr <= 0;
      per <= 0;
      valid <= 0;
      done <= 1'b0;
   end else if(run) begin
      delayCounter <= delay;
      addr <= start;
      per <= 0;
      valid <= 0;
      done <= 1'b0;
      if(delay == 0) begin
         valid <= 1'b1;
      end
   end else if(|delayCounter) begin
      delayCounter <= delayCounter - 1;
      valid <= 1'b1;
   end else if(valid && ready) begin
      if(perCond) begin
         per <= 0;
         done <= 1'b1;
         valid <= 0;
      end else  begin
         addr <= addr + (incr << OFFSET_W);
         per <= per + 1;
      end
   end
end

endmodule

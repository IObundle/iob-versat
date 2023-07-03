`timescale 1ns / 1ps

module MyAddressGen # (
   parameter ADDR_W = 10,
   parameter PERIOD_W = 10,
   parameter DELAY_W = 32
   ) (
   input                              clk,
   input                              rst,

   input                              run,

   //configurations 
   input [ADDR_W - 1:0]           iterations,
   input [PERIOD_W - 1:0]             period,
   input [PERIOD_W - 1:0]             duty,
   input [DELAY_W - 1:0]              delay,
   input [ADDR_W - 1:0]           start,
   input signed [ADDR_W - 1:0]    shift,
   input signed [ADDR_W - 1:0]    incr,

   //outputs 
   output reg                         valid,
   input                              ready,
   output reg [ADDR_W - 1:0]      addr,

   //output reg                       mem_en,
   output reg                         done
);

reg [DELAY_W-1:0] delayCounter;

reg [ADDR_W - 1:0] iter;
reg [PERIOD_W - 1:0] per;

wire perCond = ((per + 1) >= period);
wire iterCond = ((iter + 1) >= iterations);

always @(posedge clk,posedge rst)
begin
   if(rst) begin
      delayCounter <= 0;
      addr <= 0;
      iter <= 0;
      per <= 0;
      valid <= 0;
      done <= 1'b0;
   end else if(run) begin
      delayCounter <= delay;
      addr <= start;
      iter <= 0;
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
      if(perCond && iterCond) begin
         per <= 0;
         iter <= 0;
         done <= 1'b1;
         valid <= 0;
      end
      if(perCond && !iterCond) begin
         addr <= addr + shift;
         per <= 0;
         iter <= iter + 1;
      end
      if(!perCond) begin
         if(per < duty) begin
            addr <= addr + incr;
         end
         per <= per + 1;
      end
   end
end

endmodule

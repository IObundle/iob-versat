`timescale 1ns / 1ps
`include "xversat.vh"

module MyAddressGen # (
   parameter           MEM_ADDR_W = `MEM_ADDR_W,
   parameter           PERIOD_W = `PERIOD_W
   ) (
   input                              clk,
   input                              rst,
   input                              run,

   input                              next,

   //configurations 
   input [MEM_ADDR_W - 1:0]           iterations,
   input [PERIOD_W - 1:0]             period,
   input [PERIOD_W - 1:0]             duty,
   input [PERIOD_W - 1:0]             delay,
   input [MEM_ADDR_W - 1:0]           start,
   input signed [MEM_ADDR_W - 1:0]    shift,
   input signed [MEM_ADDR_W - 1:0]    incr,

   //outputs 
   output reg                         valid,
   output reg [MEM_ADDR_W - 1:0]      addr,
   //output reg                         mem_en,
   output                             done
);

reg [PERIOD_W-1:0] delayCounter;

reg [MEM_ADDR_W - 1:0] iter;
reg [PERIOD_W - 1:0] per;

wire perCond = ((per + 1) >= period);
wire iterCond = ((iter + 1) >= iterations);

assign done = (perCond && iterCond);

always @(posedge clk,posedge rst)
begin
   if(rst) begin
      delayCounter <= 0;
      addr <= 0;
   end else if(run) begin
      delayCounter <= delay;
      addr <= start;
      iter <= 0;
      per <= 0;
      valid <= 0;
      if(delay == 0) begin
         valid <= 1'b1;
      end
   end else if(|delayCounter) begin
      delayCounter <= delayCounter - 1;
      valid <= 1'b1;
   end else if(next && !done) begin
      valid <= 1'b1;
      if(perCond && iterCond) begin
         per <= 0;
         iter <= 0;
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
   end else begin
      valid <= 1'b0;
   end
end

endmodule
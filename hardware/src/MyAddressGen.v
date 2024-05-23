`timescale 1ns / 1ps

module MyAddressGen #(
   parameter ADDR_W   = 10,
   parameter PERIOD_W = 10,
   parameter DELAY_W  = 7,
   parameter DATA_W   = 32
) (
   input clk_i,
   input rst_i,

   input run_i,

   //configurations 
   input        [  ADDR_W - 1:0] iterations_i,
   input        [PERIOD_W - 1:0] period_i,
   input        [PERIOD_W - 1:0] duty_i,
   input        [ DELAY_W - 1:0] delay_i,
   input        [  ADDR_W - 1:0] start_i,
   input signed [  ADDR_W - 1:0] shift_i,
   input signed [  ADDR_W - 1:0] incr_i,

   //outputs 
   output reg                valid_o,
   input                     ready_i,
   output reg [ADDR_W - 1:0] addr_o,

   //output reg                  mem_en_o,
   output reg done_o
);

   localparam OFFSET_W = $clog2(DATA_W / 8);

   reg                                           [   DELAY_W-1:0] delay_iCounter;

   reg                                           [  ADDR_W - 1:0] iter;
   reg                                           [PERIOD_W - 1:0] per;

   wire perCond = ((per + 1) >= period_i);
   wire iterCond = ((iter + 1) >= iterations_i);

   always @(posedge clk_i, posedge rst_i) begin
      if (rst_i) begin
         delay_iCounter <= 0;
         addr_o         <= 0;
         iter           <= 0;
         per            <= 0;
         valid_o        <= 0;
         done_o         <= 1'b1;
      end else if (run_i) begin
         delay_iCounter <= delay_i;
         addr_o         <= start_i;
         iter           <= 0;
         per            <= 0;
         valid_o        <= 0;
         done_o         <= 1'b0;
         if (delay_i == 0) begin
            valid_o <= 1'b1;
         end
      end else if (|delay_iCounter) begin
         delay_iCounter <= delay_iCounter - 1;
         valid_o        <= 1'b1;
      end else if (valid_o && ready_i) begin
         if (perCond && iterCond) begin
            per     <= 0;
            iter    <= 0;
            done_o  <= 1'b1;
            valid_o <= 0;
         end
         if (perCond && !iterCond) begin
            addr_o <= addr_o + (shift_i << OFFSET_W);
            per    <= 0;
            iter   <= iter + 1;
         end
         if (!perCond) begin
            if (per < duty_i) begin
               addr_o <= addr_o + (incr_i << OFFSET_W);
            end
            per <= per + 1;
         end
      end
   end

endmodule  // MyAddressGen

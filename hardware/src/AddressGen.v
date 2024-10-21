`timescale 1ns / 1ps

module AddressGen #(
   parameter ADDR_W   = 10,
   parameter PERIOD_W = 10,
   parameter DELAY_W  = 7,
   parameter DATA_W   = 32
) (
   input clk_i,
   input rst_i,

   input run_i,

   //configurations 
   input        [  ADDR_W - 1:0] start_i,
   input        [PERIOD_W - 1:0] duty_i,

   input        [PERIOD_W - 1:0] period_i,
   input signed [  ADDR_W - 1:0] incr_i,

   input        [  ADDR_W - 1:0] iterations_i,
   input signed [  ADDR_W - 1:0] shift_i,


   input        [ DELAY_W - 1:0] delay_i,

   //outputs 
   output                    valid_o,
   input                     ready_i,
   output reg [ADDR_W - 1:0] addr_o,
   output                    store_o,

   output reg done_o
);

   localparam OFFSET_W = $clog2(DATA_W / 8);

   reg                                           [   DELAY_W-1:0] delay_counter;

   reg                                           [  ADDR_W - 1:0] iter;
   reg                                           [PERIOD_W - 1:0] per;

   wire iterCond = (((iter + 1) == iterations_i) || (iterations_i == 0));
   wire perCond = (((per + 1) == period_i) || (period_i == 0));
   reg valid;

   assign store_o = (per < duty_i);
   assign valid_o = valid && (per < duty_i);

   always @(posedge clk_i, posedge rst_i) begin
      if (rst_i) begin
         delay_counter <= 0;
         addr_o        <= 0;
         iter          <= 0;
         per           <= 0;
         valid       <= 0;
         done_o        <= 1'b1;
      end else if (run_i) begin
         delay_counter <= delay_i;
         addr_o        <= start_i;
         iter          <= 0;
         per           <= 0;
         valid       <= 0;
         done_o        <= 1'b0;
         if (delay_i == 0) begin
            valid <= 1'b1;
         end
      end else if (|delay_counter) begin
         delay_counter <= delay_counter - 1;
         valid       <= (delay_counter == 1);
      end else if (valid && ready_i) begin
         if (perCond && iterCond) begin
            per     <= 0;
            iter    <= 0;
            done_o  <= 1'b1;
            valid <= 0;
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

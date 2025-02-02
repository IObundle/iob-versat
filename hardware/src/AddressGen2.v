`timescale 1ns / 1ps

module AddressGen2 #(
   parameter ADDR_W   = 10,
   parameter PERIOD_W = 10,
   parameter DELAY_W  = 7,
   parameter DATA_W   = 8
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

   input        [PERIOD_W - 1:0] period2_i,
   input signed [  ADDR_W - 1:0] incr2_i,

   input        [  ADDR_W - 1:0] iterations2_i,
   input signed [  ADDR_W - 1:0] shift2_i,

   input        [ DELAY_W - 1:0] delay_i,

   //outputs 
   output reg                valid_o,
   input                     ready_i,
   output reg [ADDR_W - 1:0] addr_o,
   output                    store_o,

   output reg done_o
);

localparam OFFSET_W = $clog2(DATA_W / 8);

reg                                           [   DELAY_W-1:0] delay_counter;

reg                                           [  ADDR_W - 1:0] iter,iter2;
reg                                           [PERIOD_W - 1:0] per,per2;

reg [ADDR_W-1:0] addr2;

wire iter2Cond = ((iter2 + 1) == iterations2_i || iterations2_i == 0);
wire per2Cond = ((per2 + 1) == period2_i || period2_i == 0);

wire iterCond = ((iter + 1) == iterations_i || iterations_i == 0);
wire perCond = ((per + 1) == period_i || period_i == 0);

wire [3:0] cases = {iter2Cond,per2Cond,iterCond,perCond};

wire [ADDR_W-1:0] debugAddr = ({1'b0,addr_o[ADDR_W-2:0]} >> OFFSET_W);

/* TODO:
   
   The address generation can be decoupled, but it is better if we separate it into individual modules.
   Because we must take into account future iter and per conditations.
   The idea of "runnning one ahead" does not work if we only take into account per. 

   Basically need a module that implements per and iter logic only.

*/

assign store_o = (per < duty_i);

always @(posedge clk_i, posedge rst_i) begin
   if (rst_i) begin
      delay_counter <= 0;
      addr_o        <= 0;
      addr2         <= 0;
      iter          <= 0;
      iter2         <= 0;
      per           <= 0;
      per2          <= 0;
      valid_o       <= 0;
      done_o        <= 1'b1;
   end else if (run_i) begin
      delay_counter <= delay_i;
      addr_o        <= start_i;
      addr2         <= start_i;
      iter          <= 0;
      iter2         <= 0;
      per           <= 0;
      per2          <= 0;
      valid_o       <= 0;
      done_o        <= 1'b0;
      if (delay_i == 0) begin
         valid_o <= 1'b1;
      end
   end else if (|delay_counter) begin
      delay_counter <= delay_counter - 1;
      valid_o       <= (delay_counter == 1);
   end else if (valid_o && ready_i) begin
      casez(cases)
      4'b???0: begin
         if (per < duty_i) begin
            addr_o <= addr_o + (incr_i << OFFSET_W);
         end
         per <= per + 1;         
      end
      4'b??01: begin
         addr_o <= addr_o + (shift_i << OFFSET_W);
         per    <= 0;
         iter   <= iter + 1;
      end
      4'b?011: begin
         addr2  <= addr2 + (incr2_i << OFFSET_W);
         addr_o <= addr2 + (incr2_i << OFFSET_W);
         per    <= 0;
         iter   <= 0;
         per2   <= per2 + 1;
      end
      4'b0111: begin
         addr2  <= addr2 + (shift2_i << OFFSET_W);
         addr_o <= addr2 + (shift2_i << OFFSET_W);
         per    <= 0;
         iter   <= 0;
         per2   <= 0;
         iter2  <= iter2 + 1;
      end
      4'b1111: begin
         done_o  <= 1'b1;
         valid_o <= 0;
      end
      endcase
   end
end

endmodule  // MyAddressGen

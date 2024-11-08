`timescale 1ns / 1ps

// Simple counter interface
module CountNLoops #(
   parameter COUNT_W = 0
   ) (
      input [COUNT_W-1:0] count_i,
      input start_i,

      input advance_i,

      output reg count_is_zero_o,

      input clk_i,
      input rst_i
   );

reg [COUNT_W-1:0] count;

always @(posedge clk_i,posedge rst_i) begin
   if(rst_i) begin
      count <= 0;
      count_is_zero_o <= 1'b0;
   end else if(start_i) begin
      count_is_zero_o <= 1'b0;
      count <= count_i;
      if(count_i == 0) begin
         count_is_zero_o <= 1'b1;
      end
   end else if(advance_i) begin
      if(|count) count <= count - 1;
      if(count == 1) begin
         count_is_zero_o <= 1'b1;
      end
   end
end

endmodule
`timescale 1ns / 1ps

module AxiDelay #(
   parameter MAX_DELAY = 3
) (
   input      s_valid_i,
   output reg s_ready_o,

   output reg m_valid_o,
   input      m_ready_i,

   input clk_i,
   input rst_i
);

   reg [$clog2(MAX_DELAY):0] counter;
   always @(posedge clk_i, posedge rst_i) begin
      if (rst_i) begin
         counter <= 0;
      end else begin
         if (counter == 0 && m_valid_o && m_ready_i) begin
            counter <= ($urandom % MAX_DELAY);
         end

         if (counter) counter <= counter - 1;
      end
   end

   always @* begin
      s_ready_o = 1'b0;
      m_valid_o = 1'b0;

      if (counter == 0) begin
         s_ready_o = m_ready_i;
         m_valid_o = s_valid_i;
      end
   end

endmodule  // AxiDelay

`timescale 1ns / 1ps

module AxiDelay #(
      parameter MAX_DELAY = 3
   )
   (
      input s_valid,
      output reg s_ready,

      output reg m_valid,
      input m_ready,

      input clk,
      input rst
   );

reg [$clog2(MAX_DELAY):0] counter;
always @(posedge clk,posedge rst)
begin
   if(rst) begin
      counter <= 0;
   end else begin
      if(counter == 0 && m_valid && m_ready) begin
         counter <= ($urandom % MAX_DELAY);
      end

      if(counter)
         counter <= counter - 1;
   end
end

always @*
begin
   s_ready = 1'b0;
   m_valid = 1'b0;

   if(counter == 0) begin
      s_ready = m_ready;
      m_valid = s_valid;
   end
end

endmodule
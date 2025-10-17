`timescale 1ns / 1ps

`define VERSAT_SIM_DELAY 0

`ifdef SIM

`undef VERSAT_SIM_DELAY

`ifdef VERSAT_SIM_AXI_DELAY
`define VERSAT_SIM_DELAY `VERSAT_SIM_AXI_DELAY
`else
`define VERSAT_SIM_DELAY 5
`endif

`endif

module VersatAxiDelay #(
   parameter MAX_DELAY = `VERSAT_SIM_DELAY
) (
   input      s_valid_i,
   output reg s_ready_o,

   output reg m_valid_o,
   input      m_ready_i,

   input clk_i,
   input rst_i
);

   generate 
   if(MAX_DELAY == 0) begin
      always @* begin
         s_ready_o = m_ready_i;
         m_valid_o = s_valid_i;
      end
   end else begin
      reg [$clog2(MAX_DELAY):0] counter;
      always @(posedge clk_i, posedge rst_i) begin
         if (rst_i) begin
            counter <= ($urandom % MAX_DELAY);
         end else begin
            if ((counter == 0) && m_valid_o && m_ready_i) begin
               counter <= ($urandom % MAX_DELAY);
            end

            if (counter && s_valid_i) counter <= counter - 1;
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
   end
   endgenerate

endmodule  // AxiDelay

`undef VERSAT_SIM_DELAY

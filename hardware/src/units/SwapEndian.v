`timescale 1ns / 1ps

module SwapEndian #(
   parameter DATA_W = 32
) (
   input running,

   (* versat_latency = 0 *) input [DATA_W-1:0] in0,

   //input / output data
   (* versat_latency = 0 *) output reg [DATA_W-1:0] out0,

   input enabled
);

   always @* begin
      out0 = {DATA_W{1'b0}};
      if (running) begin
          out0 = in0;
          if (enabled) begin
             out0[0+:8]  = in0[24+:8];
             out0[8+:8]  = in0[16+:8];
             out0[16+:8] = in0[8+:8];
             out0[24+:8] = in0[0+:8];
          end
      end
   end

endmodule

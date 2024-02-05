`timescale 1ns / 1ps

module FixedBuffer #(
   parameter ADDR_W = 6,
   parameter DATA_W = 32,
   parameter AMOUNT = 0
) (
   //control
   input clk,
   input rst,

   input running,
   input run,

   //input / output data
   input [DATA_W-1:0] in0,

   (* versat_latency = 1 *) output reg [DATA_W-1:0] out0
);

   generate
      if (AMOUNT == 0) begin
         always @(posedge clk, posedge rst) begin
            if (rst) out0 <= 0;
            else out0 <= in0;
         end
      end else begin
         integer        i;
         genvar         ii;
         reg     [31:0] bufferData[AMOUNT-1:0];
         reg     [31:0] bufferNextData[AMOUNT-1:0];

         // Looks strange but putting everything inside a always @(posedge clk,posedge rst) block was giving a verilator error in the lines bufferData[i] <= bufferData[i+1].
         // Somehow putting this logic as a non-delayed assignment and generating AMOUNT always cases seems to be accepted fine by the tools.
         always @* begin
            for (i = 0; i < AMOUNT - 1; i = i + 1) begin
               bufferNextData[i] = bufferData[i+1];
            end
            bufferNextData[AMOUNT-1] = in0;
         end

         for(ii = 0; ii < AMOUNT; ii = ii + 1) begin
            always @(posedge clk,posedge rst) begin
               if (rst) begin
                  bufferData[ii] <= 0;
               end else begin
                  bufferData[ii] <= bufferNextData[ii];
               end
            end
         end

         always @(posedge clk, posedge rst) begin
            if (rst) begin
               out0 <= 0;
            end else begin
               out0 <= bufferData[0];
            end
         end
      end
   endgenerate

endmodule

`timescale 1ns / 1ps

module ChangeDetect #(
   parameter DATA_W = 32,
   parameter DELAY_W = 7,
   parameter AMOUNT = 0
) (
   //control
   input clk,
   input rst,

   input running,
   input run,

   //input / output data
                            input [DATA_W-1:0] in0,
   (* versat_latency = 1 *) input [DATA_W-1:0] in1,

   input [DELAY_W-1:0] delay0,

   output [DATA_W-1:0] out0
);

   reg [DELAY_W-1:0] delay;
   reg        valid;

   always @(posedge clk, posedge rst) begin
      if (rst) begin
         delay <= 0;
         valid <= 0;
      end else if (run) begin
         delay <= delay0;
         valid <= 0;
      end else if (|delay) begin
         delay <= delay - 1;
      end else begin
         valid <= 1'b1;
      end
   end

   generate
      if (AMOUNT == 0) begin
         assign out0 = valid ? ((in0 != in1) ? ~0 : 0) : 0;
      end else begin
         reg [31:0] counterToMaintain;

         assign out0 = valid ? (|counterToMaintain) ? ~0 : ((in0 != in1) ? ~0 : 0) : 0;

         always @(posedge clk, negedge rst) begin
            if (rst) begin
               counterToMaintain <= 0;
            end else if (valid) begin
               if (in0 != in1) begin
                  counterToMaintain <= AMOUNT;
               end else if (running && |counterToMaintain) begin
                  counterToMaintain <= counterToMaintain - 1;
               end
            end
         end
      end
   endgenerate

endmodule

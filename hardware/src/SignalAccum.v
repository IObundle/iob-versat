`timescale 1ns / 1ps

module SignalAccum #(
         parameter DATA_W = 32
              )
    (
    //control
    input               clk,
    input               rst,
    
    input               running,
    input               run,

    //input / output data
    input [DATA_W-1:0] in0,
    input [DATA_W-1:0] in1, // Signal

    (* versat_latency = 1 *) output reg [DATA_W-1:0] out0,
    (* versat_latency = 1 *) output [DATA_W-1:0] out1,

    input [31:0]       delay0
     );

reg [31:0] delay;

assign out1 = in1;

always @(posedge clk,posedge rst)
begin
     if(rst) begin
          delay <= 0;
          out0 <= 0;
     end else if(run) begin
          delay <= delay0;
          out0 <= 0;
     end else if(|delay) begin
          delay <= delay - 1;
          out0 <= 0;
     end else begin
          if(|in1) begin
               out0 <= in0;
          end else begin
               out0 <= out0 + in0;
          end
     end
end

endmodule

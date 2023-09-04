`timescale 1ns / 1ps

module FloatMul #(
         parameter DELAY_W = 32,
         parameter DATA_W = 32
              )
    (
    //control
    input                         clk,
    input                         rst,
    
    input                         running,
    input                         run,

    //input / output data
    input [DATA_W-1:0]            in0,
    input [DATA_W-1:0]            in1,

    input [31:0]                  delay0,

    (* versat_latency = 4 *) output [DATA_W-1:0]       out0
    );

reg [31:0] delay;
always @(posedge clk,posedge rst) begin
     if(rst) begin
          delay <= 0;
     end else if(run) begin
          delay <= delay0 + 4;
     end else if(|delay) begin
          delay <= delay - 1;
     end
end

wire [DATA_W-1:0] mul_out;

fp_mul Mul(
    .clk(clk),
    .rst(rst),

    .start(1'b1),
    .done(),

    .op_a(in0),
    .op_b(in1),

    .overflow(),
    .underflow(),
    .exception(),

    .res(mul_out)
     );

assign out0 = (running && (|delay) == 0) ? mul_out : 0;

endmodule

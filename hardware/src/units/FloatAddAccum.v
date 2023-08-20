`timescale 1ns / 1ps

module FloatAddAccum #(
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
    input [DATA_W-1:0]            in1, // Signal
    
    input [31:0]                  delay0,

    (* versat_latency = 6 *) output reg [DATA_W-1:0] out0,
    (* versat_latency = 6 *) output [DATA_W-1:0] out1
    );

assign out1 = in1;

reg [31:0] delay;
reg [2:0] counter;
reg started;

wire [31:0] addResult;

localparam COUNTER_AMOUNT = 5;

always @(posedge clk,posedge rst) begin
     if(rst) begin
          out0 <= 0;
          delay <= 0;
          counter <= 0;
          started <= 0;
     end else if(run) begin
          delay <= delay0;
          counter <= 0;
          started <= 0;
     end else if(|delay) begin
          delay <= delay - 1;
     end else begin
          if(|counter) begin
               counter <= counter - 1;
          end else begin
               counter <= COUNTER_AMOUNT;
               out0 <= addResult;
          end

          if(|in1) begin
               out0 <= in0;
               counter <= COUNTER_AMOUNT;
          end

          if(!started) begin
               started <= 1;
               out0 <= in0;
          end
     end
end

fp_add adder(
     .start(1'b1),
     .done(),

     .op_a(in0),
     .op_b(out0),

     .res(addResult),

     .overflow(),
     .underflow(),
     .exception(),

     .clk(clk),
     .rst(rst)
     );

endmodule

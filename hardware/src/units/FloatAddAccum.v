`timescale 1ns / 1ps

module FloatAdd #(
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
    
    (* versat_latency = 6 *) output reg [DATA_W-1:0] out0
    );

reg [2:0] counter;
//reg [31:0] accum;

wire [31:0] addResult;

always @(posedge clk,posedge rst) begin
     if(rst) begin
          accum <= 0;
     end else begin
          if(in1) begin
               out0 <= in0;
               counter <= 5;               
          end

          if(counter) begin
               counter <= counter - 1;
          end else begin
               counter <= 5;
               out0 <= addResult;
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

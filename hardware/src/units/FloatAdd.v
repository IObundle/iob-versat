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
    input [DATA_W-1:0]            in1,

    input [31:0]                  delay0,
    
    (* versat_latency = 5 *) output [DATA_W-1:0]       out0
    );

reg [31:0] delay;
always @(posedge clk,posedge rst) begin
     if(rst) begin
          delay <= 0;
     end else if(run) begin
          delay <= delay0 + 5;
     end else if(|delay) begin
          delay <= delay - 1;
     end
end

wire [DATA_W-1:0] adder_out;

fp_add adder(
     .start(1'b1),
     .done(),

     .op_a(in0),
     .op_b(in1),

     .res(adder_out),

     .overflow(),
     .underflow(),
     .exception(),

     .clk(clk),
     .rst(rst)
     );

assign out0 = (running && (|delay) == 0) ? adder_out : 0;

endmodule

`timescale 1ns / 1ps

module FloatSub #(
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

reg [4:0] equalReg;

wire equal = (in0 == in1);

always @(posedge clk,posedge rst) begin
    if(rst) begin
        equalReg[0] <= 0;
        equalReg[1] <= 0;
        equalReg[2] <= 0;
        equalReg[3] <= 0;
        equalReg[4] <= 0;
    end else begin
        equalReg[4] <= equal;
        equalReg[3] <= equalReg[4];
        equalReg[2] <= equalReg[3];
        equalReg[1] <= equalReg[2];
        equalReg[0] <= equalReg[1];
    end
end

wire [DATA_W-1:0] negative = {~in1[31],in1[30:0]};
wire [DATA_W-1:0] out;

iob_fp_add adder(
    .clk_i(clk),
    .rst_i(rst),

    .start_i(1'b1),
    .done_o(),

    .op_a_i(in0),
    .op_b_i(negative),

    .overflow_o(),
    .underflow_o(),
    .exception_o(),

    .res_o(out)
     );

assign out0 = (running & (|delay) == 0) ? (equalReg[0] ? 0 : out) 
                                        : 0;

endmodule

`timescale 1ns / 1ps

module Conditional_tb (

);
  localparam DATA_W = 32;
  // Inputs
  reg [(DATA_W)-1:0] in0;
  reg [(DATA_W)-1:0] in1;
  reg [(DATA_W)-1:0] in2;
  // Outputs
  wire [(DATA_W)-1:0] out0;
  wire [(DATA_W)-1:0] out1;
  wire [(DATA_W)-1:0] out2;
  // Control
  reg [(1)-1:0] running;
  reg [(1)-1:0] clk;
  reg [(1)-1:0] rst;

  localparam CLOCK_PERIOD = 10;

  initial clk = 0;
  always #(CLOCK_PERIOD/2) clk = ~clk;
  `define ADVANCE @(posedge clk) #(CLOCK_PERIOD/2);

  Conditional uut (
    .in0(in0),
    .in1(in1),
    .in2(in2),
    .out0(out0),
    .running(running),
    .clk(clk),
    .rst(rst)
  );

  initial begin
    `ifdef VCD;
    $dumpfile("uut.vcd");
    $dumpvars();
    `endif // VCD;
    in0 = 0;
    in1 = 0;
    in2 = 0;
    running = 0;
    clk = 0;
    rst = 0;

    `ADVANCE;

    rst = 1;

    `ADVANCE;

    rst = 0;
    running = 1;
    in0 = 0;
    in1 = 32'hFFFFFFFF;
    in2 = 32'hFFFFFFFF;

    `ADVANCE;
    in0 = 32'hFFFFFFFF;
    in1 = 32'hFFFFFFFF;
    in2 = 32'hFFFFFFFF;

    `ADVANCE;

    in0 = 0;
    in1 = 32'h00000000;
    in2 = 32'h00000000;

    `ADVANCE;
    in0 = 32'hFFFFFFFF;
    in1 = 32'h00000000;
    in2 = 32'h00000000;

    `ADVANCE;

    running = 0;

    `ADVANCE;

    $finish();
  end


endmodule

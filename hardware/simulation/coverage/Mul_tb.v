`timescale 1ns / 1ps

// verilator coverage_off
module Mul_tb (

);
  localparam DATA_W = 2;
  // Inputs
  reg [(DATA_W)-1:0] in0;
  reg [(DATA_W)-1:0] in1;
  // Outputs
  wire [(DATA_W)-1:0] out0;
  // Control
  reg [(1)-1:0] running;
  reg [(1)-1:0] clk;
  reg [(1)-1:0] rst;

  integer i;

  localparam CLOCK_PERIOD = 10;

  initial clk = 0;
  always #(CLOCK_PERIOD/2) clk = ~clk;
  `define ADVANCE @(posedge clk) #(CLOCK_PERIOD/2);

  Mul #(
    .DATA_W(DATA_W)
  ) uut (
    .in0(in0),
    .in1(in1),
    .out0(out0),
    .running(running),
    .clk(clk),
    .rst(rst)
  );


  task RunMultiplication;
  begin
    running = 1;
    repeat(4) begin
        `ADVANCE;
    end
    running = 0;
  end
  endtask

  initial begin
    `ifdef VCD;
    $dumpfile("uut.vcd");
    $dumpvars();
    `endif // VCD;
    in0 = 0;
    in1 = 0;
    running = 0;
    clk = 0;
    rst = 0;

    `ADVANCE;

    rst = 1;

    `ADVANCE;

    for(i=0;i<((2**DATA_W)+1);i=i+1) begin
        rst = 0;
        in0 = 2'b01;
        in1 = i[DATA_W-1:0];
        RunMultiplication();
        rst = 1;
        `ADVANCE;
    end

    rst = 0;

    `ADVANCE;

    for(i=0;i<((2**DATA_W)+1);i=i+1) begin
        rst = 0;
        in0 = i[DATA_W-1:0];
        in1 = 2'b01;
        RunMultiplication();
        rst = 1;
        `ADVANCE;
    end

    $finish();
  end

endmodule
// verilator coverage_on

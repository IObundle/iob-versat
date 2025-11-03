`timescale 1ns / 1ps

// verilator coverage_off
module TestConst_tb (

);
  localparam DATA_W = 2;
  localparam DELAY_W = 2;
  // Outputs
  wire [(DATA_W)-1:0] out0;
  // Control
  reg [(1)-1:0] running;
  reg [(1)-1:0] run;
  reg [(1)-1:0] clk;
  reg [(1)-1:0] rst;
  // Config
  reg [(DATA_W)-1:0] constant;
  // Delays
  reg [(DELAY_W)-1:0] delay0;

  integer i;

  localparam CLOCK_PERIOD = 10;

  initial clk = 0;
  always #(CLOCK_PERIOD/2) clk = ~clk;
  `define ADVANCE @(posedge clk) #(CLOCK_PERIOD/2);

  TestConst #(
    .DATA_W(DATA_W),
    .DELAY_W(DELAY_W)
  ) uut (
    .out0(out0),
    .running(running),
    .run(run),
    .clk(clk),
    .rst(rst),
    .constant(constant),
    .delay0(delay0)
  );

  initial begin
    `ifdef VCD;
    $dumpfile("uut.vcd");
    $dumpvars();
    `endif // VCD;
    running = 0;
    run = 0;
    clk = 0;
    rst = 0;
    constant = 0;
    delay0 = 0;

    `ADVANCE;

    rst = 1;

    `ADVANCE;

    rst = 0;
    constant = 2'b11;
    delay0 = 2'b11;

    run = 1;
    running = 1;

    `ADVANCE;

    run = 0;
    for(i=0;i<delay0;i=i+1) begin
        `ADVANCE;
    end

    running = 0;

    `ADVANCE;

    constant = 0;
    delay0 = 0;

    run = 1;
    running = 1;

    `ADVANCE;

    run = 0;
    for(i=0;i<delay0;i=i+1) begin
        `ADVANCE;
    end

    running = 0;

    `ADVANCE;

    $finish();
  end

endmodule
// verilator coverage_on

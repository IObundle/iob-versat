`timescale 1ns / 1ps

// verilator coverage_off
module TimedMux_tb (

);
  localparam DATA_W = 1;
  localparam DELAY_W = 2;
  // Inputs
  reg [(DATA_W)-1:0] in0;
  reg [(DATA_W)-1:0] in1;
  // Outputs
  wire [(DATA_W)-1:0] out0;
  // Control
  reg [(1)-1:0] running;
  reg [(1)-1:0] run;
  reg [(1)-1:0] clk;
  reg [(1)-1:0] rst;
  // Delays
  reg [(DELAY_W)-1:0] delay0;

  integer i;

  localparam CLOCK_PERIOD = 10;

  initial clk = 0;
  always #(CLOCK_PERIOD/2) clk = ~clk;
  `define ADVANCE @(posedge clk) #(CLOCK_PERIOD/2);

  TimedMux #(
    .DATA_W(DATA_W),
    .DELAY_W(DELAY_W)
  ) uut (
    .in0(in0),
    .in1(in1),
    .out0(out0),
    .running(running),
    .run(run),
    .clk(clk),
    .rst(rst),
    .delay0(delay0)
  );

  initial begin
    `ifdef VCD;
    $dumpfile("uut.vcd");
    $dumpvars();
    `endif // VCD;
    in0 = 0;
    in1 = 0;
    running = 0;
    run = 0;
    clk = 0;
    rst = 0;
    delay0 = 0;

    `ADVANCE;

    rst = 1;

    `ADVANCE;

    rst = 0;

    `ADVANCE;

    in0 = 1'b1;
    in1 = 1'b1;
    delay0 = 2'b10;

    run = 1'b1;
    running = 1'b1;

    for(i=0;i<(delay0+1);i=i+1) begin
        `ADVANCE;
        in0 = i[0];
        run = 1'b0;
    end

    `ADVANCE;

    in1 = 1'b0;

    `ADVANCE;

    running = 1'b0;

    `ADVANCE;

    in0 = 1'b0;
    in1 = 1'b0;
    delay0 = 2'b11;
    run = 1'b1;
    running = 1'b1;

    `ADVANCE;

    running = 1'b0;
    delay0 = 2'b0;

    $finish();
  end

endmodule
// verilator coverage_on

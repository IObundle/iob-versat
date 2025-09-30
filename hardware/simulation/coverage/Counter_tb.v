`timescale 1ns / 1ps

module Counter_tb (

);
  // Outputs
  wire [(32)-1:0] out0;
  // Control
  reg [(1)-1:0] running;
  reg [(1)-1:0] run;
  reg [(1)-1:0] clk;
  reg [(1)-1:0] rst;
  // Config
  reg [(32)-1:0] start;

  localparam CLOCK_PERIOD = 10;

  initial clk = 0;
  always #(CLOCK_PERIOD/2) clk = ~clk;
  `define ADVANCE @(posedge clk) #(CLOCK_PERIOD/2);

  Counter uut (
    .out0(out0),
    .running(running),
    .run(run),
    .clk(clk),
    .rst(rst),
    .start(start)
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
    start = 0;

    `ADVANCE;

    rst = 1;
    start = 32'hFFFFFFFF;

    `ADVANCE;

    rst = 0;

    run = 1;
    running = 1;

    start = 32'hFFFFFFF0;

    `ADVANCE;

    run = 0;
    start = 0;

    // wait for the counter to wrap
    while (out0 != 0) begin
      `ADVANCE;
    end

    running = 0;

    `ADVANCE;

    run = 1;
    running = 1;
    start = 32'hFFFFFFFF;

    `ADVANCE;

    running = 0;

    $finish();
  end

endmodule

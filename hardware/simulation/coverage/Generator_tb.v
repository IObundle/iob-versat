`timescale 1ns / 1ps

// verilator coverage_off
module Generator_tb (

);
  localparam PERIOD_W = 2;
  localparam DELAY_W = 2;
  localparam ADDR_W = 4;
  // Outputs
  wire [(ADDR_W)-1:0] out0;
  // Control
  reg [(1)-1:0] running;
  reg [(1)-1:0] run;
  reg [(1)-1:0] clk;
  reg [(1)-1:0] rst;
  // Config
  reg [(ADDR_W)-1:0] iter;
  reg [(ADDR_W)-1:0] shift;
  reg [(PERIOD_W)-1:0] per;
  reg [(ADDR_W)-1:0] incr;
  reg [(ADDR_W)-1:0] iter2;
  reg [(ADDR_W)-1:0] shift2;
  reg [(PERIOD_W)-1:0] per2;
  reg [(ADDR_W)-1:0] incr2;
  reg [(ADDR_W)-1:0] iter3;
  reg [(ADDR_W)-1:0] shift3;
  reg [(PERIOD_W)-1:0] per3;
  reg [(ADDR_W)-1:0] incr3;
  reg [(PERIOD_W)-1:0] duty;
  reg [(ADDR_W)-1:0] start;
  // Delays
  reg [(DELAY_W)-1:0] delay0;

  integer i;

  localparam CLOCK_PERIOD = 10;

  initial clk = 0;
  always #(CLOCK_PERIOD/2) clk = ~clk;
  `define ADVANCE @(posedge clk) #(CLOCK_PERIOD/2);

  Generator #(
    .PERIOD_W(PERIOD_W),
    .DELAY_W(DELAY_W),
    .ADDR_W(ADDR_W)
  ) uut (
    .out0(out0),
    .running(running),
    .run(run),
    .clk(clk),
    .rst(rst),
    .iter(iter),
    .shift(shift),
    .per(per),
    .incr(incr),
    .iter2(iter2),
    .shift2(shift2),
    .per2(per2),
    .incr2(incr2),
    .iter3(iter3),
    .shift3(shift3),
    .per3(per3),
    .incr3(incr3),
    .duty(duty),
    .start(start),
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
    iter = 0;
    shift = 0;
    per = 0;
    incr = 0;
    iter2 = 0;
    shift2 = 0;
    per2 = 0;
    incr2 = 0;
    iter3 = 0;
    shift3 = 0;
    per3 = 0;
    incr3 = 0;
    duty = 0;
    start = 0;
    delay0 = 0;

    `ADVANCE;

    rst = 1;

    `ADVANCE;

    rst = 0;
    // configure max values
    iter = {ADDR_W{1'b1}};
    shift = {ADDR_W{1'b1}};
    per = {PERIOD_W{1'b1}};
    incr = {ADDR_W{1'b1}};
    iter2 = {ADDR_W{1'b1}};
    shift2 = {ADDR_W{1'b1}};
    per2 = {PERIOD_W{1'b1}};
    incr2 = {ADDR_W{1'b1}};
    iter3 = {ADDR_W{1'b1}};
    shift3 = {ADDR_W{1'b1}};
    per3 = {PERIOD_W{1'b1}};
    incr3 = {ADDR_W{1'b1}};
    duty = {PERIOD_W{1'b1}};
    start = {ADDR_W{1'b1}};
    delay0 = {DELAY_W{1'b1}};

    run = 1;
    running = 1;

    `ADVANCE;
    run = 0;

    for(i=0;i<(2**(PERIOD_W*DELAY_W*ADDR_W));i=i+1) begin
      `ADVANCE;
    end

    `ADVANCE;

    running = 0;

    // configure 0 values
    iter = {ADDR_W{1'b0}};
    shift = {ADDR_W{1'b0}};
    per = {PERIOD_W{1'b0}};
    incr = {ADDR_W{1'b0}};
    iter2 = {ADDR_W{1'b0}};
    shift2 = {ADDR_W{1'b0}};
    per2 = {PERIOD_W{1'b0}};
    incr2 = {ADDR_W{1'b0}};
    iter3 = {ADDR_W{1'b0}};
    shift3 = {ADDR_W{1'b0}};
    per3 = {PERIOD_W{1'b0}};
    incr3 = {ADDR_W{1'b0}};
    duty = {PERIOD_W{1'b0}};
    start = {ADDR_W{1'b0}};
    delay0 = {DELAY_W{1'b0}};

    `ADVANCE;

    rst = 1;

    `ADVANCE;

    rst = 0;

    `ADVANCE;

    run = 1;
    running = 1;

    `ADVANCE;

    run = 0;

    `ADVANCE;
    `ADVANCE;

    running = 0;
    rst = 1;

    `ADVANCE;

    rst = 0;

    // Small run
    iter = 2;
    shift = 2;
    per = 2;
    incr = 2;
    iter2 = 2;
    shift2 = 2;
    per2 = 2;
    incr2 = 2;
    iter3 = 2;
    shift3 = 2;
    per3 = 2;
    incr3 = 2;
    duty = 2;
    start = 2;
    delay0 = 2;

    run = 1;
    running = 1;

    `ADVANCE;

    run = 0;
    for (int i = 0; i < 1000; i++) begin
        `ADVANCE;
    end

    `ADVANCE;

    running = 0;

    $finish();
  end

endmodule
// verilator coverage_on

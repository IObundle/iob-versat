`timescale 1ns / 1ps

// verilator coverage_off
module Muladd_tb (

);
  localparam DELAY_W = 2;
  localparam DATA_W = 2;
  localparam LOOP_W = 2;
  // Inputs
  reg [(DATA_W)-1:0] in0;
  reg [(DATA_W)-1:0] in1;
  // Outputs
  wire [(DATA_W)-1:0] out0;
  // Control
  reg [(1)-1:0] running;
  reg [(1)-1:0] run;
  wire [(1)-1:0] done;
  reg [(1)-1:0] clk;
  reg [(1)-1:0] rst;
  // Config
  reg [(1)-1:0] opcode;
  reg [(LOOP_W)-1:0] iter;
  reg [(LOOP_W)-1:0] period;
  // Delays
  reg [(DELAY_W)-1:0] delay0;

  integer i;

  localparam CLOCK_PERIOD = 10;

  initial clk = 0;
  always #(CLOCK_PERIOD/2) clk = ~clk;
  `define ADVANCE @(posedge clk) #(CLOCK_PERIOD/2);

  Muladd #(
    .DELAY_W(DELAY_W),
    .DATA_W(DATA_W),
    .LOOP_W(LOOP_W)
  ) uut (
    .in0(in0),
    .in1(in1),
    .out0(out0),
    .running(running),
    .run(run),
    .done(done),
    .clk(clk),
    .rst(rst),
    .opcode(opcode),
    .iter(iter),
    .period(period),
    .delay0(delay0)
  );


  task RunAccelerator;
  begin
    run = 1;

    `ADVANCE;

    run = 0;
    running = 1;

    `ADVANCE;

    while(done) begin

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
    run = 0;
    clk = 0;
    rst = 0;
    opcode = 0;
    iter = 0;
    period = 0;
    delay0 = 0;

    `ADVANCE;

    rst = 1;

    `ADVANCE;

    rst = 0;

    opcode = 1;
    iter = 2'b11;
    period = 2'b11;
    delay0 = 2'b11;
    run = 1;
    running = 1;

    `ADVANCE;
    run = 0;
    
    i = 0;
    while (done == 0) begin
        in0 = 2'b01;
        in1 = i[DATA_W-1:0];
        `ADVANCE;
        i = i + 1;
    end

    running = 0;

    `ADVANCE;

    rst = 1;
    iter = 2'b00;
    period = 2'b00;
    delay0 = 2'b00;

    `ADVANCE;

    rst = 0;

    opcode = 0;
    iter = 2'b11;
    period = 2'b11;
    delay0 = 2'b01;
    run = 1;
    running = 1;

    `ADVANCE;
    run = 0;
    
    i = 0;
    while (done == 0) begin
        in0 = i[DATA_W-1:0];
        in1 = 2'b01;
        `ADVANCE;
        i = i + 1;
    end

    `ADVANCE;

    running = 0;

    `ADVANCE;

    rst = 1;
    iter = 2'b00;
    period = 2'b00;
    delay0 = 2'b00;

    `ADVANCE;

    rst = 0;

    // test iter == 0
    iter = 2'b00;
    run = 1;
    running = 1;

    `ADVANCE;
    run = 0;
    
    i = 0;
    while (done == 0) begin
        in0 = i[DATA_W-1:0];
        in1 = 2'b01;
        `ADVANCE;
        i = i + 1;
    end

    running = 0;

    `ADVANCE;

    // test iter == 0 (2nd time)
    iter = 2'b00;
    run = 1;
    running = 1;

    `ADVANCE;
    run = 0;
    
    i = 0;
    while (done == 0) begin
        in0 = i[DATA_W-1:0];
        in1 = 2'b01;
        `ADVANCE;
        i = i + 1;
    end

    running = 0;

    `ADVANCE;

    $finish();
  end

endmodule
// verilator coverage_on

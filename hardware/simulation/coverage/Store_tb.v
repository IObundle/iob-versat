`timescale 1ns / 1ps

// verilator coverage_off
module Store_tb (

);
  localparam DELAY_W = 2;
  localparam DATA_W = 1;
  // Inputs
  reg [(DATA_W)-1:0] in0;
  // Control
  reg [(1)-1:0] running;
  reg [(1)-1:0] run;
  wire [(1)-1:0] done;
  reg [(1)-1:0] clk;
  reg [(1)-1:0] rst;
  // State
  wire [(DATA_W)-1:0] currentValue;
  // Delays
  reg [(DELAY_W)-1:0] delay0;

  localparam CLOCK_PERIOD = 10;

  initial clk = 0;
  always #(CLOCK_PERIOD/2) clk = ~clk;
  `define ADVANCE @(posedge clk) #(CLOCK_PERIOD/2);

  Store #(
    .DELAY_W(DELAY_W),
    .DATA_W(DATA_W)
  ) uut (
    .in0(in0),
    .running(running),
    .run(run),
    .done(done),
    .clk(clk),
    .rst(rst),
    .currentValue(currentValue),
    .delay0(delay0)
  );


  task RunAccelerator;
  begin
    run = 1;

    `ADVANCE;

    run = 0;
    running = 1;

    `ADVANCE;

    while(~done) begin

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
    delay0 = 2'b11;
    RunAccelerator();

    in0 = 1'b0;
    delay0 = 2'b00;
    RunAccelerator();

    $finish();
  end

endmodule
// verilator coverage_on

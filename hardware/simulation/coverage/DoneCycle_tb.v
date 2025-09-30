`timescale 1ns / 1ps

// verilator coverage_off
module DoneCycle_tb (

);
  localparam DATA_W = 4;
  // Control
  reg [(1)-1:0] running;
  reg [(1)-1:0] run;
  wire [(1)-1:0] done;
  reg [(1)-1:0] clk;
  reg [(1)-1:0] rst;
  // Config
  reg [(DATA_W)-1:0] amount;

  localparam CLOCK_PERIOD = 10;

  initial clk = 0;
  always #(CLOCK_PERIOD/2) clk = ~clk;
  `define ADVANCE @(posedge clk) #(CLOCK_PERIOD/2);

  DoneCycle #(
    .DATA_W(DATA_W)
  ) uut (
    .running(running),
    .run(run),
    .done(done),
    .clk(clk),
    .rst(rst),
    .amount(amount)
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
    running = 0;
    run = 0;
    clk = 0;
    rst = 0;
    amount = 0;

    `ADVANCE;

    rst = 1;
    amount = 4'hF;

    `ADVANCE;

    rst = 0;
    amount = 4'hF;
    RunAccelerator();

    `ADVANCE;

    amount = 0;

    `ADVANCE;

    amount = 4'hF;
    RunAccelerator();

    $finish();
  end

endmodule
// verilator coverage_on

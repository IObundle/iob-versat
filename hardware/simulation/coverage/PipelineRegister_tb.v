`timescale 1ns / 1ps

// verilator coverage_off
module PipelineRegister_tb (

);
  localparam DATA_W = 1;
  // Inputs
  reg [(DATA_W)-1:0] in0;
  // Outputs
  wire [(DATA_W)-1:0] out0;
  // Control
  reg [(1)-1:0] running;
  reg [(1)-1:0] clk;
  reg [(1)-1:0] rst;

  localparam CLOCK_PERIOD = 10;

  initial clk = 0;
  always #(CLOCK_PERIOD/2) clk = ~clk;
  `define ADVANCE @(posedge clk) #(CLOCK_PERIOD/2);

  PipelineRegister #(
    .DATA_W(DATA_W)
  ) uut (
    .in0(in0),
    .out0(out0),
    .running(running),
    .clk(clk),
    .rst(rst)
  );


  task SetRunning;
  begin
    running = 1;

    `ADVANCE;

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
    clk = 0;
    rst = 0;

    `ADVANCE;

    rst = 1;

    `ADVANCE;

    rst = 0;

    `ADVANCE;

    in0 = 1;
    SetRunning();
    in0 = 0;
    SetRunning();

    $finish();
  end

endmodule
// verilator coverage_on

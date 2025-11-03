`timescale 1ns / 1ps

// verilator coverage_off
module StridedMerge_tb (

);
  localparam DELAY_W = 2;
  localparam DATA_W = 1;
  // Inputs
  reg [(DATA_W)-1:0] in0;
  reg [(DATA_W)-1:0] in1;
  reg [(DATA_W)-1:0] in2;
  reg [(DATA_W)-1:0] in3;
  reg [(DATA_W)-1:0] in4;
  reg [(DATA_W)-1:0] in5;
  reg [(DATA_W)-1:0] in6;
  reg [(DATA_W)-1:0] in7;
  reg [(DATA_W)-1:0] in8;
  reg [(DATA_W)-1:0] in9;
  reg [(DATA_W)-1:0] in10;
  reg [(DATA_W)-1:0] in11;
  reg [(DATA_W)-1:0] in12;
  reg [(DATA_W)-1:0] in13;
  reg [(DATA_W)-1:0] in14;
  reg [(DATA_W)-1:0] in15;
  // Outputs
  wire [(DATA_W)-1:0] out0;
  // Control
  reg [(1)-1:0] running;
  reg [(1)-1:0] run;
  reg [(1)-1:0] clk;
  reg [(1)-1:0] rst;
  // Config
  reg [(DELAY_W)-1:0] stride;
  // Delays
  reg [(DELAY_W)-1:0] delay0;

  integer i;

  localparam CLOCK_PERIOD = 10;

  initial clk = 0;
  always #(CLOCK_PERIOD/2) clk = ~clk;
  `define ADVANCE @(posedge clk) #(CLOCK_PERIOD/2);

  StridedMerge #(
    .DELAY_W(DELAY_W),
    .DATA_W(DATA_W)
  ) uut (
    .in0(in0),
    .in1(in1),
    .in2(in2),
    .in3(in3),
    .in4(in4),
    .in5(in5),
    .in6(in6),
    .in7(in7),
    .in8(in8),
    .in9(in9),
    .in10(in10),
    .in11(in11),
    .in12(in12),
    .in13(in13),
    .in14(in14),
    .in15(in15),
    .out0(out0),
    .running(running),
    .run(run),
    .clk(clk),
    .rst(rst),
    .stride(stride),
    .delay0(delay0)
  );


  task RunAccelerator;
  begin
    run <= 1;

    `ADVANCE;

    run <= 0;
    running <= 1;

    `ADVANCE;

    running <= 0;
  end
  endtask

  initial begin
    `ifdef VCD;
    $dumpfile("uut.vcd");
    $dumpvars();
    `endif // VCD;
    in0 = 0;
    in1 = 0;
    in2 = 0;
    in3 = 0;
    in4 = 0;
    in5 = 0;
    in6 = 0;
    in7 = 0;
    in8 = 0;
    in9 = 0;
    in10 = 0;
    in11 = 0;
    in12 = 0;
    in13 = 0;
    in14 = 0;
    in15 = 0;
    running = 0;
    run = 0;
    clk = 0;
    rst = 0;
    stride = 0;
    delay0 = 0;

    `ADVANCE;

    rst = 1;

    `ADVANCE;

    rst = 0;

    in0 = 1;
    in1 = 1;
    in2 = 1;
    in3 = 1;
    in4 = 1;
    in5 = 1;
    in6 = 1;
    in7 = 1;
    in8 = 1;
    in9 = 1;
    in10 = 1;
    in11 = 1;
    in12 = 1;
    in13 = 1;
    in14 = 1;
    in15 = 1;
    stride = 2'b11;
    delay0 = 2'b11;

    run = 1;
    running = 1;

    `ADVANCE;

    run = 0;

    // loop through all inputs
    for(i=0; i<(4+(16*4)); i=i+1) begin
        `ADVANCE;
    end

    running = 0;

    in0 = 0;
    in1 = 0;
    in2 = 0;
    in3 = 0;
    in4 = 0;
    in5 = 0;
    in6 = 0;
    in7 = 0;
    in8 = 0;
    in9 = 0;
    in10 = 0;
    in11 = 0;
    in12 = 0;
    in13 = 0;
    in14 = 0;
    in15 = 0;

    stride = 2'b00;
    delay0 = 2'b00;

    `ADVANCE;

    stride = 2'b11;
    delay0 = 2'b11;

    run = 1;
    running = 1;

    `ADVANCE;

    run = 0;

    // loop through all inputs
    for(i=0; i<(4+(16*4)); i=i+1) begin
        `ADVANCE;
    end

    $finish();
  end

endmodule
// verilator coverage_off

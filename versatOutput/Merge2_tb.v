`timescale 1ns / 1ps

module Merge2_tb (

);
  localparam DELAY_W = 7;
  localparam DATA_W = 32;
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
  reg [(DATA_W)-1:0] in16;
  reg [(DATA_W)-1:0] in17;
  reg [(DATA_W)-1:0] in18;
  reg [(DATA_W)-1:0] in19;
  reg [(DATA_W)-1:0] in20;
  reg [(DATA_W)-1:0] in21;
  reg [(DATA_W)-1:0] in22;
  reg [(DATA_W)-1:0] in23;
  reg [(DATA_W)-1:0] in24;
  reg [(DATA_W)-1:0] in25;
  reg [(DATA_W)-1:0] in26;
  reg [(DATA_W)-1:0] in27;
  reg [(DATA_W)-1:0] in28;
  reg [(DATA_W)-1:0] in29;
  reg [(DATA_W)-1:0] in30;
  reg [(DATA_W)-1:0] in31;
  // Outputs
  wire [(DATA_W)-1:0] out0;
  // Control
  reg [(1)-1:0] running;
  reg [(1)-1:0] run;
  reg [(1)-1:0] clk;
  reg [(1)-1:0] rst;
  // Delays
  reg [(DELAY_W)-1:0] delay0;

  localparam CLOCK_PERIOD = 10;

  initial clk = 0;
  always #(CLOCK_PERIOD/2) clk = ~clk;
  `define ADVANCE @(posedge clk) #(CLOCK_PERIOD/2);

  Merge2 #(
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
    .in16(in16),
    .in17(in17),
    .in18(in18),
    .in19(in19),
    .in20(in20),
    .in21(in21),
    .in22(in22),
    .in23(in23),
    .in24(in24),
    .in25(in25),
    .in26(in26),
    .in27(in27),
    .in28(in28),
    .in29(in29),
    .in30(in30),
    .in31(in31),
    .out0(out0),
    .running(running),
    .run(run),
    .clk(clk),
    .rst(rst),
    .delay0(delay0)
  );


  localparam SIM_TIME_NO_DONE = 10;
  integer i;
  task RunAccelerator;
  begin
    run = 1;

    `ADVANCE;

    run = 0;
    running = 1;

    `ADVANCE;

    for(i = 0 ; i < SIM_TIME_NO_DONE ; i = i + 1) begin
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
    in16 = 0;
    in17 = 0;
    in18 = 0;
    in19 = 0;
    in20 = 0;
    in21 = 0;
    in22 = 0;
    in23 = 0;
    in24 = 0;
    in25 = 0;
    in26 = 0;
    in27 = 0;
    in28 = 0;
    in29 = 0;
    in30 = 0;
    in31 = 0;
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

    // Insert logic after this point

    RunAccelerator();

    `ADVANCE;

    RunAccelerator();

    `ADVANCE;

    $finish();
  end


endmodule

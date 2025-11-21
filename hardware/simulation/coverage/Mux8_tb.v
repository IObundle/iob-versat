`timescale 1ns / 1ps

// verilator coverage_off
module Mux8_tb (

);
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
  // Outputs
  wire [(DATA_W)-1:0] out0;
  // Control
  reg [(1)-1:0] running;
  reg [(1)-1:0] clk;
  reg [(1)-1:0] rst;
  // Config
  reg [(3)-1:0] sel;

  integer i;

  localparam CLOCK_PERIOD = 10;

  initial clk = 0;
  always #(CLOCK_PERIOD/2) clk = ~clk;
  `define ADVANCE @(posedge clk) #(CLOCK_PERIOD/2);

  Mux8 #(
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
    .out0(out0),
    .running(running),
    .clk(clk),
    .rst(rst),
    .sel(sel)
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
    in1 = 0;
    in2 = 0;
    in3 = 0;
    in4 = 0;
    in5 = 0;
    in6 = 0;
    in7 = 0;
    running = 0;
    clk = 0;
    rst = 0;
    sel = 0;

    `ADVANCE;

    rst = 1;

    `ADVANCE;

    rst = 0;

    `ADVANCE;

    for(i=0; i<3*8;i=i+1) begin
        sel = i[2:0];
        in0 = ~i[3];
        in1 = ~i[3];
        in2 = ~i[3];
        in3 = ~i[3];
        in4 = ~i[3];
        in5 = ~i[3];
        in6 = ~i[3];
        in7 = ~i[3];

        SetRunning();
        `ADVANCE;
    end

    $finish();
  end

endmodule
// verilator coverage_on

`timescale 1ns / 1ps

// verilator coverage_off
module CombMux8_tb (

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
  // Config
  reg [(3)-1:0] sel;

  integer i;

  `define ADVANCE #(10);
  CombMux8 #(
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
    .sel(sel)
  );


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
    sel = 0;

    `ADVANCE;

    // All input 1
    in0 = 1;
    in1 = 1;
    in2 = 1;
    in3 = 1;
    in4 = 1;
    in5 = 1;
    in6 = 1;
    in7 = 1;

    for(i=0;i<8;i=i+1) begin
      sel = i[2:0];
      `ADVANCE;
    end

    // All input 0
    in0 = 0;
    in1 = 0;
    in2 = 0;
    in3 = 0;
    in4 = 0;
    in5 = 0;
    in6 = 0;
    in7 = 0;

    for(i=0;i<8;i=i+1) begin
      sel = i[2:0];
      `ADVANCE;
    end

    $finish();
  end

endmodule
// verilator coverage_on

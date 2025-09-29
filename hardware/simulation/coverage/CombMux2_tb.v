`timescale 1ns / 1ps

module CombMux2_tb (

);
  localparam DATA_W = 1;
  // Inputs
  reg [(DATA_W)-1:0] in0;
  reg [(DATA_W)-1:0] in1;
  // Outputs
  wire [(DATA_W)-1:0] out0;
  // Config
  reg [(1)-1:0] sel;

  `define ADVANCE #(10);
  CombMux2 #(
    .DATA_W(DATA_W)
  ) uut (
    .in0(in0),
    .in1(in1),
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
    sel = 0;

    `ADVANCE;

    in0 = 0;
    in1 = 1;
    sel = 0;

    `ADVANCE;

    in0 = 0;
    in1 = 0;
    sel = 1;

    `ADVANCE;

    in0 = 1;
    in1 = 0;
    sel = 0;

    `ADVANCE;

    in0 = 0;
    in1 = 1;
    sel = 0;

    `ADVANCE;

    $finish();
  end

endmodule

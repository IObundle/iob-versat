`timescale 1ns / 1ps

// verilator coverage_off
module CombMux4_tb (

);
  localparam DATA_W = 1;
  // Inputs
  reg [(DATA_W)-1:0] in0;
  reg [(DATA_W)-1:0] in1;
  reg [(DATA_W)-1:0] in2;
  reg [(DATA_W)-1:0] in3;
  // Outputs
  wire [(DATA_W)-1:0] out0;
  // Config
  reg [(2)-1:0] sel;

  integer i;

  `define ADVANCE #(10);
  CombMux4 #(
    .DATA_W(DATA_W)
  ) uut (
    .in0(in0),
    .in1(in1),
    .in2(in2),
    .in3(in3),
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
    sel = 0;

    `ADVANCE;

    // All input 1
    in0 = 1;
    in1 = 1;
    in2 = 1;
    in3 = 1;

    for(i=0;i<4;i=i+1) begin
      sel = i[1:0];
      `ADVANCE;
    end

    // All input 0
    in0 = 0;
    in1 = 0;
    in2 = 0;
    in3 = 0;

    for(i=0;i<4;i=i+1) begin
      sel = i[1:0];
      `ADVANCE;
    end

    $finish();
  end

endmodule
// verilator coverage_on

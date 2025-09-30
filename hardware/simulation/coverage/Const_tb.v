`timescale 1ns / 1ps

// verilator coverage_off
module Const_tb (

);
  localparam DATA_W = 1;
  // Outputs
  wire [(DATA_W)-1:0] out0;
  // Config
  reg [(DATA_W)-1:0] constant;

  `define ADVANCE #(10);
  Const #(
    .DATA_W(DATA_W)
  ) uut (
    .out0(out0),
    .constant(constant)
  );


  initial begin
    `ifdef VCD;
    $dumpfile("uut.vcd");
    $dumpvars();
    `endif // VCD;
    constant = 0;

    `ADVANCE;

    constant = 1;

    `ADVANCE;

    constant = 0;

    `ADVANCE;

    $finish();
  end


endmodule
// verilator coverage_on

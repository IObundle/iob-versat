// SPDX-FileCopyrightText: 2025 IObundle, Lda
//
// SPDX-License-Identifier: MIT
//
// Py2HWSW Version 0.81 has generated this code (https://github.com/IObundle/py2hwsw).

`timescale 1ns / 1ps

// verilator coverage_off
module SwapEndian_tb (

);
  localparam DATA_W = 32;
  // Inputs
  reg [(DATA_W)-1:0] in0;
  // Outputs
  wire [(DATA_W)-1:0] out0;
  // Control
  reg [(1)-1:0] running;
  // Config
  reg [(1)-1:0] enabled;

  integer i;

  `define ADVANCE #(10);
  SwapEndian #(
    .DATA_W(DATA_W)
  ) uut (
    .in0(in0),
    .out0(out0),
    .running(running),
    .enabled(enabled)
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
    enabled = 0;

    `ADVANCE;

    enabled = 1;
    for(i=0;i<4;i=i+1) begin
        in0 = {DATA_W{i[0]}};
        enabled = i[1];
        SetRunning();
    end
    enabled = 0;

    `ADVANCE;

    $finish();
  end

endmodule
// verilator coverage_on

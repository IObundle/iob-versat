`timescale 1ns / 1ps

// verilator coverage_off
module Buffer_tb (

);
  localparam DELAY_W = 4;
  localparam DATA_W = 1;
  // Inputs
  reg [(DATA_W)-1:0] in0;
  // Outputs
  wire [(DATA_W)-1:0] out0;
  // Control
  reg [(1)-1:0] running;
  reg [(1)-1:0] clk;
  reg [(1)-1:0] rst;
  // Config
  reg [(DELAY_W)-1:0] amount;

  integer i, j;

  localparam CLOCK_PERIOD = 10;

  initial clk = 0;
  always #(CLOCK_PERIOD/2) clk = ~clk;
  `define ADVANCE @(posedge clk) #(CLOCK_PERIOD/2);

  Buffer #(
    .DELAY_W(DELAY_W),
    .DATA_W(DATA_W)
  ) uut (
    .in0(in0),
    .out0(out0),
    .running(running),
    .clk(clk),
    .rst(rst),
    .amount(amount)
  );

  initial begin
    `ifdef VCD;
    $dumpfile("uut.vcd");
    $dumpvars();
    `endif // VCD;
    in0 = 0;
    running = 0;
    clk = 0;
    rst = 0;
    amount = 0;

    `ADVANCE;

    rst = 1;

    `ADVANCE;

    rst = 0;

    `ADVANCE;

    amount = {DELAY_W{1'b1}};

    for(i=0;i<(2**(DELAY_W+1));i=i+1) begin
      running = 1;
      in0 = i[0];
      `ADVANCE;
    end

    running = 0;
    amount = 0;

    `ADVANCE;
    
    rst = 1;

    `ADVANCE;

    rst = 0;

    for(i=0;i<(2**(DELAY_W+3));i=i+1) begin
      running = 1;
      in0 = i[DELAY_W+1];
      `ADVANCE;
    end


    $finish();
  end

endmodule
// verilator coverage_on

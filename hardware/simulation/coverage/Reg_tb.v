`timescale 1ns / 1ps

// verilator coverage_off
module Reg_tb (

);
  localparam DELAY_W = 2;
  localparam DATA_W = 8;
  // Inputs
  reg [(DATA_W)-1:0] in0;
  // Outputs
  wire [(DATA_W)-1:0] out0;
  // Control
  reg [(1)-1:0] running;
  reg [(1)-1:0] run;
  wire [(1)-1:0] done;
  reg [(1)-1:0] clk;
  reg [(1)-1:0] rst;
  // Config
  reg [(1)-1:0] disabled;
  // State
  wire [(DATA_W)-1:0] currentValue;
  // Delays
  reg [(DELAY_W)-1:0] delay0;
  // MemoryMapped
  reg [(1)-1:0] valid;
  reg [(2)-1:0] addr;
  reg [(DATA_W/8)-1:0] wstrb;
  reg [(DATA_W)-1:0] wdata;
  wire [(1)-1:0] rvalid;
  wire [(DATA_W)-1:0] rdata;

  integer i;

  localparam CLOCK_PERIOD = 10;

  initial clk = 0;
  always #(CLOCK_PERIOD/2) clk = ~clk;
  `define ADVANCE @(posedge clk) #(CLOCK_PERIOD/2);

  Reg #(
    .DELAY_W(DELAY_W),
    .DATA_W(DATA_W)
  ) uut (
    .in0(in0),
    .out0(out0),
    .running(running),
    .run(run),
    .done(done),
    .clk(clk),
    .rst(rst),
    .disabled(disabled),
    .currentValue(currentValue),
    .delay0(delay0),
    .valid(valid),
    .addr(addr),
    .wstrb(wstrb),
    .wdata(wdata),
    .rvalid(rvalid),
    .rdata(rdata)
  );


  task Write (input [2-1:0] addr_i,input [DATA_W-1:0] data_i);
  begin

    `ADVANCE;

    valid = 1;
    wstrb = ~0;
    addr = addr_i;
    wdata = data_i;

    `ADVANCE;

    valid = 0;
    wstrb = 0;
    addr = 0;
    wdata = 0;
  end
  endtask

  task Read (input [2-1:0] addr_i);
  begin

    `ADVANCE;

    valid = 1;
    wstrb = 0;
    addr = addr_i;

    `ADVANCE;

    while(~rvalid) begin
      `ADVANCE;
    end

    valid = 0;
  end
  endtask

  task RunAccelerator;
  begin
    run = 1;

    `ADVANCE;

    run = 0;
    running = 1;

    `ADVANCE;

    while(~done) begin

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
    running = 0;
    run = 0;
    clk = 0;
    rst = 0;
    disabled = 0;
    delay0 = 0;
    valid = 0;
    addr = 0;
    wstrb = 0;
    wdata = 0;

    `ADVANCE;

    rst = 1;

    disabled = 1'b1;

    `ADVANCE;

    rst = 0;
    disabled = 1'b0;

    `ADVANCE;

    in0 = 8'hFF;
    delay0 = 2'b11;
    run = 1'b1;

    while(~done) begin
        `ADVANCE
        run = 1'b0;
        running = 1'b1;
    end

    `ADVANCE

    running = 1'b0;

    `ADVANCE

    for(i=0;i<(2**DELAY_W);i=i+1) begin
        in0 = {DATA_W{i[0]}};
        delay0 = i[DELAY_W-1:0];
        Write(i[1:0],in0);
        RunAccelerator();
    end

    `ADVANCE

    // Read
    valid = 1'b1;
    addr = 2'b11;
    wstrb = 0;
    disabled = 1'b1;

    while (~rvalid) begin
        `ADVANCE
    end

    valid = 1'b0;

    `ADVANCE

    $finish();
  end

endmodule
// verilator coverage_on

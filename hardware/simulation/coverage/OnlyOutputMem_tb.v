`timescale 1ns / 1ps

// verilator coverage_off
module OnlyOutputMem_tb (

);
  localparam DATA_W = 8;
  localparam SIZE_W = 2;
  localparam DELAY_W = 2;
  localparam ADDR_W = 2;
  localparam PERIOD_W = 2;
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
  reg [(ADDR_W)-1:0] iterA;
  reg [(PERIOD_W)-1:0] perA;
  reg [(PERIOD_W)-1:0] dutyA;
  reg [(ADDR_W)-1:0] startA;
  reg [(ADDR_W)-1:0] shiftA;
  reg [(ADDR_W)-1:0] incrA;
  // Delays
  reg [(DELAY_W)-1:0] delay0;
  // MemoryMapped
  reg [(1)-1:0] valid;
  reg [(12)-1:0] addr;
  reg [(DATA_W/8)-1:0] wstrb;
  reg [(DATA_W)-1:0] wdata;
  wire [(1)-1:0] rvalid;
  wire [(DATA_W)-1:0] rdata;
  // ExternalMemory
  wire [(ADDR_W)-1:0] ext_2p_addr_out_0;
  wire [(ADDR_W)-1:0] ext_2p_addr_in_0;
  wire [(1)-1:0] ext_2p_write_0;
  wire [(1)-1:0] ext_2p_read_0;
  wire [(DATA_W)-1:0] ext_2p_data_in_0;
  wire [(DATA_W)-1:0] ext_2p_data_out_0;

  integer i;

  localparam CLOCK_PERIOD = 10;

  initial clk = 0;
  always #(CLOCK_PERIOD/2) clk = ~clk;
  `define ADVANCE @(posedge clk) #(CLOCK_PERIOD/2);

  OnlyOutputMem #(
    .DATA_W(DATA_W),
    .SIZE_W(SIZE_W),
    .DELAY_W(DELAY_W),
    .ADDR_W(ADDR_W),
    .PERIOD_W(PERIOD_W)
  ) uut (
    .out0(out0),
    .running(running),
    .run(run),
    .done(done),
    .clk(clk),
    .rst(rst),
    .disabled(disabled),
    .iterA(iterA),
    .perA(perA),
    .dutyA(dutyA),
    .startA(startA),
    .shiftA(shiftA),
    .incrA(incrA),
    .delay0(delay0),
    .valid(valid),
    .addr(addr),
    .wstrb(wstrb),
    .wdata(wdata),
    .rvalid(rvalid),
    .rdata(rdata),
    .ext_2p_addr_out_0(ext_2p_addr_out_0),
    .ext_2p_addr_in_0(ext_2p_addr_in_0),
    .ext_2p_write_0(ext_2p_write_0),
    .ext_2p_read_0(ext_2p_read_0),
    .ext_2p_data_in_0(ext_2p_data_in_0),
    .ext_2p_data_out_0(ext_2p_data_out_0)
  );


  task Write (input [ADDR_W-1:0] addr_i,input [DATA_W-1:0] data_i);
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

  my_2p_asym_ram #(
    .W_DATA_W(DATA_W),
    .R_DATA_W(DATA_W),
    .ADDR_W(ADDR_W)
  ) ext_2p_0 (
    .w_en_i(ext_2p_write_0),
    .w_addr_i(ext_2p_addr_out_0),
    .w_data_i(ext_2p_data_out_0),
    .r_en_i(ext_2p_read_0),
    .r_addr_i(ext_2p_addr_in_0),
    .r_data_o(ext_2p_data_in_0),
    .clk_i(clk)
  );

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
    running = 0;
    run = 0;
    clk = 0;
    rst = 0;
    disabled = 0;
    iterA = 0;
    perA = 0;
    dutyA = 0;
    startA = 0;
    shiftA = 0;
    incrA = 0;
    delay0 = 0;
    valid = 0;
    addr = 0;
    wstrb = 0;
    wdata = 0;

    `ADVANCE;

    rst = 1;

    `ADVANCE;

    rst = 0;
    disabled = 1;

    `ADVANCE;

    // write data to memory - multiple times 0 -> 1 -> 0 -> 1
    for(i=0;i<(2**(ADDR_W+2));i=i+1) begin
      Write(i[ADDR_W-1:0], {DATA_W{i[ADDR_W]}});
    end

    for(i=0;i<(2**ADDR_W);i=i+1) begin
      Write(i[ADDR_W-1:0], {DATA_W{i[0]}});
    end

    // max configuration
    disabled = 0;
    iterA = {ADDR_W{1'b1}};
    perA = {PERIOD_W{1'b1}};
    dutyA = {PERIOD_W{1'b1}};
    startA = {ADDR_W{1'b1}};
    shiftA = {ADDR_W{1'b1}};
    incrA = {ADDR_W{1'b1}};
    delay0 = {DELAY_W{1'b1}};

    run = 1;
    running = 1;
    `ADVANCE;
    run = 0;
    // run
    for(i=0;i<(2**(ADDR_W*PERIOD_W*DELAY_W));i=i+1) begin
      `ADVANCE;
    end

    running = 0;

    `ADVANCE;
    rst = 1;
    `ADVANCE;

    rst = 0;

    iterA = {ADDR_W{1'b0}};
    perA = {PERIOD_W{1'b0}};
    dutyA = {PERIOD_W{1'b0}};
    startA = {ADDR_W{1'b0}};
    shiftA = {ADDR_W{1'b0}};
    incrA = {ADDR_W{1'b0}};
    delay0 = {DELAY_W{1'b0}};

    `ADVANCE;
    rst = 1;
    `ADVANCE;
    rst = 0;
    run = 1;
    running = 1;
    `ADVANCE;
    run = 0;
    `ADVANCE;
    `ADVANCE;
    running = 0;

    $finish();
  end

endmodule
// verilator coverage_on

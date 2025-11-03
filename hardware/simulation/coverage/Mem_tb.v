`timescale 1ns / 1ps

// verilator coverage_off
module Mem_tb (

);
  localparam DATA_W = 8;
  localparam SIZE_W = 2;
  localparam DELAY_W = 2;
  localparam ADDR_W = 2;
  localparam PERIOD_W = 2;
  // Inputs
  reg [(DATA_W)-1:0] in0;
  reg [(DATA_W)-1:0] in1;
  // Outputs
  wire [(DATA_W)-1:0] out0;
  wire [(DATA_W)-1:0] out1;
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
  reg [(1)-1:0] reverseA;
  reg [(1)-1:0] extA;
  reg [(1)-1:0] in0_wr;
  reg [(ADDR_W)-1:0] iterB;
  reg [(PERIOD_W)-1:0] perB;
  reg [(PERIOD_W)-1:0] dutyB;
  reg [(ADDR_W)-1:0] startB;
  reg [(ADDR_W)-1:0] shiftB;
  reg [(ADDR_W)-1:0] incrB;
  reg [(1)-1:0] reverseB;
  reg [(1)-1:0] extB;
  reg [(1)-1:0] in1_wr;
  // Delays
  reg [(DELAY_W)-1:0] delay0;
  reg [(DELAY_W)-1:0] delay1;
  // MemoryMapped
  reg [(1)-1:0] valid;
  reg [(12)-1:0] addr;
  reg [(DATA_W/8)-1:0] wstrb;
  reg [(DATA_W)-1:0] wdata;
  wire [(1)-1:0] rvalid;
  wire [(DATA_W)-1:0] rdata;
  // ExternalMemory
  wire [(ADDR_W)-1:0] ext_dp_addr_0_port_0;
  wire [(DATA_W)-1:0] ext_dp_out_0_port_0;
  wire [(DATA_W)-1:0] ext_dp_in_0_port_0;
  wire [(1)-1:0] ext_dp_enable_0_port_0;
  wire [(1)-1:0] ext_dp_write_0_port_0;
  wire [(ADDR_W)-1:0] ext_dp_addr_0_port_1;
  wire [(DATA_W)-1:0] ext_dp_out_0_port_1;
  wire [(DATA_W)-1:0] ext_dp_in_0_port_1;
  wire [(1)-1:0] ext_dp_enable_0_port_1;
  wire [(1)-1:0] ext_dp_write_0_port_1;

  integer i;

  localparam CLOCK_PERIOD = 10;

  initial clk = 0;
  always #(CLOCK_PERIOD/2) clk = ~clk;
  `define ADVANCE @(posedge clk) #(CLOCK_PERIOD/2);

  Mem #(
    .DATA_W(DATA_W),
    .SIZE_W(SIZE_W),
    .DELAY_W(DELAY_W),
    .ADDR_W(ADDR_W),
    .PERIOD_W(PERIOD_W)
  ) uut (
    .in0(in0),
    .in1(in1),
    .out0(out0),
    .out1(out1),
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
    .reverseA(reverseA),
    .extA(extA),
    .in0_wr(in0_wr),
    .iterB(iterB),
    .perB(perB),
    .dutyB(dutyB),
    .startB(startB),
    .shiftB(shiftB),
    .incrB(incrB),
    .reverseB(reverseB),
    .extB(extB),
    .in1_wr(in1_wr),
    .delay0(delay0),
    .delay1(delay1),
    .valid(valid),
    .addr(addr),
    .wstrb(wstrb),
    .wdata(wdata),
    .rvalid(rvalid),
    .rdata(rdata),
    .ext_dp_addr_0_port_0(ext_dp_addr_0_port_0),
    .ext_dp_out_0_port_0(ext_dp_out_0_port_0),
    .ext_dp_in_0_port_0(ext_dp_in_0_port_0),
    .ext_dp_enable_0_port_0(ext_dp_enable_0_port_0),
    .ext_dp_write_0_port_0(ext_dp_write_0_port_0),
    .ext_dp_addr_0_port_1(ext_dp_addr_0_port_1),
    .ext_dp_out_0_port_1(ext_dp_out_0_port_1),
    .ext_dp_in_0_port_1(ext_dp_in_0_port_1),
    .ext_dp_enable_0_port_1(ext_dp_enable_0_port_1),
    .ext_dp_write_0_port_1(ext_dp_write_0_port_1)
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

  task Read (input [ADDR_W-1:0] addr_i);
  begin

    `ADVANCE;

    valid = 1;
    wstrb = 0;
    addr = addr_i;

    while(~rvalid) begin
        `ADVANCE;
    end

    valid = 0;
    wstrb = 0;
    addr = 0;

  end
  endtask

  my_dp_asym_ram #(
    .A_DATA_W(DATA_W),
    .B_DATA_W(DATA_W),
    .ADDR_W(ADDR_W)
  ) ext_dp_0 (
    .dinA_i(ext_dp_out_0_port_0),
    .addrA_i(ext_dp_addr_0_port_0),
    .enA_i(ext_dp_enable_0_port_0),
    .weA_i(ext_dp_write_0_port_0),
    .doutA_o(ext_dp_in_0_port_0),
    .dinB_i(ext_dp_out_0_port_1),
    .addrB_i(ext_dp_addr_0_port_1),
    .enB_i(ext_dp_enable_0_port_1),
    .weB_i(ext_dp_write_0_port_1),
    .doutB_o(ext_dp_in_0_port_1),
    .clk_i(clk)
  );

  initial begin
    `ifdef VCD;
    $dumpfile("uut.vcd");
    $dumpvars();
    `endif // VCD;
    in0 = 0;
    in1 = 0;
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
    reverseA = 0;
    extA = 0;
    in0_wr = 0;
    iterB = 0;
    perB = 0;
    dutyB = 0;
    startB = 0;
    shiftB = 0;
    incrB = 0;
    reverseB = 0;
    extB = 0;
    in1_wr = 0;
    delay0 = 0;
    delay1 = 0;
    valid = 0;
    addr = 0;
    wstrb = 0;
    wdata = 0;

    `ADVANCE;

    rst = 1;
    disabled = 1;

    `ADVANCE;

    rst = 0;
    disabled = 1;

    `ADVANCE;
    disabled = 1;

    // write data to memory - multiple times 0 -> 1 -> 0 -> 1
    for(i=0;i<(2**(ADDR_W+2));i=i+1) begin
      Write(i[ADDR_W-1:0], {DATA_W{i[ADDR_W]}});
    end

    for(i=0;i<(2**ADDR_W);i=i+1) begin
      Write(i[ADDR_W-1:0], {DATA_W{i[0]}});
    end

    // Read data from memory
    for(i=0;i<(2**ADDR_W);i=i+1) begin
      Read(i[ADDR_W-1:0]);
    end


    disabled = 0;
    // Max configuration
    iterA = {ADDR_W{1'b1}};
    perA = {PERIOD_W{1'b1}};
    dutyA = {PERIOD_W{1'b1}};
    startA = {ADDR_W{1'b1}};
    shiftA = {ADDR_W{1'b1}};
    incrA = {ADDR_W{1'b1}};

    reverseA = 1'b1;
    extA = 1'b1;
    in0_wr = 1'b1;

    iterB = {ADDR_W{1'b1}};
    perB = {PERIOD_W{1'b1}};
    dutyB = {PERIOD_W{1'b1}};
    startB = {ADDR_W{1'b1}};
    shiftB = {ADDR_W{1'b1}};
    incrB = {ADDR_W{1'b1}};

    reverseB = 1'b1;
    extB = 1'b1;
    in1_wr = 1'b1;

    delay0 = {DELAY_W{1'b1}};
    delay1 = {DELAY_W{1'b1}};

    `ADVANCE;

    // 0 configuration
    iterA = {ADDR_W{1'b0}};
    perA = {PERIOD_W{1'b0}};
    dutyA = {PERIOD_W{1'b0}};
    startA = {ADDR_W{1'b0}};
    shiftA = {ADDR_W{1'b0}};
    incrA = {ADDR_W{1'b0}};

    reverseA = 1'b0;
    extA = 1'b0;
    in0_wr = 1'b0;

    iterB = {ADDR_W{1'b0}};
    perB = {PERIOD_W{1'b0}};
    dutyB = {PERIOD_W{1'b0}};
    startB = {ADDR_W{1'b0}};
    shiftB = {ADDR_W{1'b0}};
    incrB = {ADDR_W{1'b0}};

    reverseB = 1'b0;
    extB = 1'b0;
    in1_wr = 1'b0;

    delay0 = {DELAY_W{1'b0}};
    delay1 = {DELAY_W{1'b0}};

    `ADVANCE;

    run = 1;
    running = 1;

    `ADVANCE;

    run = 0;

    for(i=0;i<(2**(ADDR_W+PERIOD_W+DELAY_W));i=i+1) begin
      `ADVANCE;
    end
    running = 0;

    `ADVANCE;

    rst = 1;

    `ADVANCE;

    rst = 0;


    // Max configuration
    iterA = {ADDR_W{1'b1}};
    perA = {PERIOD_W{1'b1}};
    dutyA = {PERIOD_W{1'b1}};
    startA = {ADDR_W{1'b1}};
    shiftA = {ADDR_W{1'b1}};
    incrA = {ADDR_W{1'b1}};

    reverseA = 1'b1;
    extA = 1'b1;
    in0_wr = 1'b1;

    iterB = {ADDR_W{1'b1}};
    perB = {PERIOD_W{1'b1}};
    dutyB = {PERIOD_W{1'b1}};
    startB = {ADDR_W{1'b1}};
    shiftB = {ADDR_W{1'b1}};
    incrB = {ADDR_W{1'b1}};

    reverseB = 1'b1;
    extB = 1'b1;
    in1_wr = 1'b1;

    delay0 = {DELAY_W{1'b1}};
    delay1 = {DELAY_W{1'b1}};

    run = 1;
    running = 1;

    `ADVANCE;

    run = 0;

    for(i=0;i<(2**(ADDR_W+PERIOD_W+DELAY_W));i=i+1) begin
      `ADVANCE;
    end

    running = 0;
    rst = 1;

    `ADVANCE;
    rst = 0;

    // in0,in1 as data
    extA = 0;
    extB = 0;
    in0_wr = 1;
    in1_wr = 1;

    run = 1;
    running = 1;

    `ADVANCE;

    for(i=0;i<(2**(ADDR_W+PERIOD_W+DELAY_W));i=i+1) begin
      in0 = {DATA_W{i[0]}};
      in1 = {DATA_W{i[0]}};
      run = 0;
      `ADVANCE;
    end
    running = 0;

    `ADVANCE;

    $finish();
  end

endmodule
// verilator coverage_on

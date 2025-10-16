`timescale 1ns / 1ps

// verilator coverage_off
module VWrite_tb (

);
  localparam DATA_W = 8;
  localparam ADDR_W = 4;
  localparam PERIOD_W = 2;
  localparam AXI_ADDR_W = 4;
  localparam AXI_DATA_W = 8;
  localparam DELAY_W = 2;
  localparam LEN_W = 4;
  // Inputs
  reg [(DATA_W)-1:0] in0;
  // Control
  reg [(1)-1:0] running;
  reg [(1)-1:0] run;
  wire [(1)-1:0] done;
  reg [(1)-1:0] clk;
  reg [(1)-1:0] rst;
  // Config
  reg [(AXI_ADDR_W)-1:0] ext_addr;
  reg [(ADDR_W)-1:0] amount_minus_one;
  reg [(LEN_W)-1:0] length;
  reg [(1)-1:0] enabled;
  reg [(AXI_ADDR_W)-1:0] addr_shift;
  reg [(1)-1:0] pingPong;
  reg [(ADDR_W)-1:0] iter;
  reg [(PERIOD_W)-1:0] per;
  reg [(PERIOD_W)-1:0] duty;
  reg [(ADDR_W)-1:0] start;
  reg [(ADDR_W)-1:0] shift;
  reg [(ADDR_W)-1:0] incr;
  reg [(ADDR_W)-1:0] iter2;
  reg [(PERIOD_W)-1:0] per2;
  reg [(ADDR_W)-1:0] shift2;
  reg [(ADDR_W)-1:0] incr2;
  reg [(ADDR_W)-1:0] iter3;
  reg [(PERIOD_W)-1:0] per3;
  reg [(ADDR_W)-1:0] shift3;
  reg [(ADDR_W)-1:0] incr3;
  reg [(1)-1:0] ignore_first;
  reg [(20)-1:0] extra_delay;
  // Delays
  reg [(DELAY_W)-1:0] delay0;
  // Databus
  wire [(1)-1:0] databus_ready_0;
  wire [(1)-1:0] databus_valid_0;
  wire [(AXI_ADDR_W)-1:0] databus_addr_0;
  wire [(AXI_DATA_W)-1:0] databus_rdata_0;
  wire [(AXI_DATA_W)-1:0] databus_wdata_0;
  wire [(AXI_DATA_W/8)-1:0] databus_wstrb_0;
  wire [(LEN_W)-1:0] databus_len_0;
  wire [(1)-1:0] databus_last_0;
  // ExternalMemory
  wire [(ADDR_W)-1:0] ext_2p_addr_out_0;
  wire [(ADDR_W)-1:0] ext_2p_addr_in_0;
  wire [(1)-1:0] ext_2p_write_0;
  wire [(1)-1:0] ext_2p_read_0;
  wire [(AXI_DATA_W)-1:0] ext_2p_data_in_0;
  wire [(DATA_W)-1:0] ext_2p_data_out_0;

  integer i;

  localparam CLOCK_PERIOD = 10;

  initial clk = 0;
  always #(CLOCK_PERIOD/2) clk = ~clk;
  `define ADVANCE @(posedge clk) #(CLOCK_PERIOD/2);

  VWrite #(
    .DATA_W(DATA_W),
    .ADDR_W(ADDR_W),
    .PERIOD_W(PERIOD_W),
    .AXI_ADDR_W(AXI_ADDR_W),
    .AXI_DATA_W(AXI_DATA_W),
    .DELAY_W(DELAY_W),
    .LEN_W(LEN_W)
  ) uut (
    .in0(in0),
    .running(running),
    .run(run),
    .done(done),
    .clk(clk),
    .rst(rst),
    .ext_addr(ext_addr),
    .amount_minus_one(amount_minus_one),
    .length(length),
    .enabled(enabled),
    .addr_shift(addr_shift),
    .pingPong(pingPong),
    .iter(iter),
    .per(per),
    .duty(duty),
    .start(start),
    .shift(shift),
    .incr(incr),
    .iter2(iter2),
    .per2(per2),
    .shift2(shift2),
    .incr2(incr2),
    .iter3(iter3),
    .per3(per3),
    .shift3(shift3),
    .incr3(incr3),
    .ignore_first(ignore_first),
    .extra_delay(extra_delay),
    .delay0(delay0),
    .databus_ready_0(databus_ready_0),
    .databus_valid_0(databus_valid_0),
    .databus_addr_0(databus_addr_0),
    .databus_rdata_0(databus_rdata_0),
    .databus_wdata_0(databus_wdata_0),
    .databus_wstrb_0(databus_wstrb_0),
    .databus_len_0(databus_len_0),
    .databus_last_0(databus_last_0),
    .ext_2p_addr_out_0(ext_2p_addr_out_0),
    .ext_2p_addr_in_0(ext_2p_addr_in_0),
    .ext_2p_write_0(ext_2p_write_0),
    .ext_2p_read_0(ext_2p_read_0),
    .ext_2p_data_in_0(ext_2p_data_in_0),
    .ext_2p_data_out_0(ext_2p_data_out_0)
  );


  reg [1-1:0] insertValue;
  reg [AXI_ADDR_W-1:0] addrToInsert;
  reg [AXI_DATA_W-1:0] valueToInsert;
  SimHelper_DatabusMem #(
    .DATA_W(AXI_DATA_W),
    .ADDR_W(AXI_ADDR_W),
    .LEN_W(LEN_W)
  ) DatabusMem (
    .databus_ready(databus_ready_0),
    .databus_valid(databus_valid_0),
    .databus_addr(databus_addr_0),
    .databus_rdata(databus_rdata_0),
    .databus_wdata(databus_wdata_0),
    .databus_wstrb(databus_wstrb_0),
    .databus_len(databus_len_0),
    .databus_last(databus_last_0),
    .insertValue(insertValue),
    .addrToInsert(addrToInsert),
    .valueToInsert(valueToInsert),
    .clk(clk),
    .rst(rst)
  );


  task WriteMemory (input [AXI_ADDR_W-1:0] addr_i,input [AXI_DATA_W-1:0] data_i);
  begin
    insertValue = 1;
    addrToInsert = addr_i;
    valueToInsert = data_i;

    `ADVANCE;

    insertValue = 0;
    addrToInsert = 0;
    valueToInsert = 0;

    `ADVANCE;

  end
  endtask

  my_2p_asym_ram #(
    .W_DATA_W(DATA_W),
    .R_DATA_W(AXI_DATA_W),
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
    in0 = 0;
    running = 0;
    run = 0;
    clk = 0;
    rst = 0;
    ext_addr = 0;
    amount_minus_one = 0;
    length = 0;
    enabled = 0;
    addr_shift = 0;
    pingPong = 0;
    iter = 0;
    per = 0;
    duty = 0;
    start = 0;
    shift = 0;
    incr = 0;
    iter2 = 0;
    per2 = 0;
    shift2 = 0;
    incr2 = 0;
    iter3 = 0;
    per3 = 0;
    shift3 = 0;
    incr3 = 0;
    ignore_first = 0;
    extra_delay = 0;
    delay0 = 0;
    insertValue = 0;
    addrToInsert = 0;
    valueToInsert = 0;

    `ADVANCE;

    rst = 1;

    `ADVANCE;

    rst = 0;

    `ADVANCE;

    // Max configuration
    in0 = {DATA_W{1'b1}};
    ext_addr = {AXI_ADDR_W{1'b1}};
    amount_minus_one = {ADDR_W{1'b1}};
    length = {LEN_W{1'b1}};
    enabled = 1;
    addr_shift = {AXI_ADDR_W{1'b1}};
    pingPong = 1;
    iter = {ADDR_W{1'b1}};
    per = {PERIOD_W{1'b1}};
    duty = {PERIOD_W{1'b1}};
    start = {ADDR_W{1'b1}};
    shift = {ADDR_W{1'b1}};
    incr = {ADDR_W{1'b1}};
    iter2 = {ADDR_W{1'b1}};
    per2 = {PERIOD_W{1'b1}};
    shift2 = {ADDR_W{1'b1}};
    incr2 = {ADDR_W{1'b1}};
    iter3 = {ADDR_W{1'b1}};
    per3 = {PERIOD_W{1'b1}};
    shift3 = {ADDR_W{1'b1}};
    incr3 = {ADDR_W{1'b1}};
    ignore_first = 1;
    extra_delay = {20{1'b1}};
    delay0 = {DELAY_W{1'b1}};

    `ADVANCE;

    // Zero configuration
    in0 = {DATA_W{1'b0}};
    ext_addr = {AXI_ADDR_W{1'b0}};
    amount_minus_one = {ADDR_W{1'b0}};
    length = {LEN_W{1'b0}};
    enabled = 0;
    addr_shift = {AXI_ADDR_W{1'b0}};
    pingPong = 0;
    iter = {ADDR_W{1'b0}};
    per = {PERIOD_W{1'b0}};
    duty = {PERIOD_W{1'b0}};
    start = {ADDR_W{1'b0}};
    shift = {ADDR_W{1'b0}};
    incr = {ADDR_W{1'b0}};
    iter2 = {ADDR_W{1'b0}};
    per2 = {PERIOD_W{1'b0}};
    shift2 = {ADDR_W{1'b0}};
    incr2 = {ADDR_W{1'b0}};
    iter3 = {ADDR_W{1'b0}};
    per3 = {PERIOD_W{1'b0}};
    shift3 = {ADDR_W{1'b0}};
    incr3 = {ADDR_W{1'b0}};
    ignore_first = 0;
    extra_delay = {20{1'b0}};
    delay0 = {DELAY_W{1'b0}};

    `ADVANCE;

    // Write data to ext 2p memory
    in0 = {DATA_W{1'b1}};
    ext_addr = {AXI_ADDR_W{1'b0}};
    amount_minus_one = {ADDR_W{1'b1}} - 1'b1;
    length = {LEN_W{1'b1}};
    enabled = 1;
    addr_shift = {AXI_ADDR_W{1'b1}};
    pingPong = 1;
    iter = {ADDR_W{1'b1}};
    per = {PERIOD_W{1'b1}};
    duty = {PERIOD_W{1'b0}};
    start = {ADDR_W{1'b1}};
    shift = {ADDR_W{1'b1}};
    incr = 1;
    iter2 = {ADDR_W{1'b0}};
    per2 = {PERIOD_W{1'b0}};
    shift2 = {ADDR_W{1'b0}};
    incr2 = {ADDR_W{1'b0}};
    iter3 = {ADDR_W{1'b0}};
    per3 = {PERIOD_W{1'b0}};
    shift3 = {ADDR_W{1'b0}};
    incr3 = {ADDR_W{1'b0}};
    ignore_first = 0;
    extra_delay = {20{1'b0}};
    delay0 = {DELAY_W{1'b0}};

    RunAccelerator();

    `ADVANCE;

    rst = 1;

    `ADVANCE;

    rst = 0;

    `ADVANCE;

    $finish();
  end

endmodule
// verilator coverage_on

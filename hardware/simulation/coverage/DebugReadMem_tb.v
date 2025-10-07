`timescale 1ns / 1ps

// verilator coverage_off
module DebugReadMem_tb (

);
  localparam AXI_ADDR_W = 2;
  localparam AXI_DATA_W = 2;
  localparam LEN_W = 8;
  // Control
  reg [(1)-1:0] running;
  reg [(1)-1:0] run;
  wire [(1)-1:0] done;
  reg [(1)-1:0] clk;
  reg [(1)-1:0] rst;
  // Config
  reg [(AXI_ADDR_W)-1:0] address;
  // State
  wire [(AXI_DATA_W)-1:0] lastRead;
  // Databus
  wire [(1)-1:0] databus_ready_0;
  wire [(1)-1:0] databus_valid_0;
  wire [(AXI_ADDR_W)-1:0] databus_addr_0;
  wire [(AXI_DATA_W)-1:0] databus_rdata_0;
  wire [(AXI_DATA_W)-1:0] databus_wdata_0;
  wire [(AXI_DATA_W/8)-1:0] databus_wstrb_0;
  wire [(LEN_W)-1:0] databus_len_0;
  wire [(1)-1:0] databus_last_0;

  integer i;

  localparam CLOCK_PERIOD = 10;

  initial clk = 0;
  always #(CLOCK_PERIOD/2) clk = ~clk;
  `define ADVANCE @(posedge clk) #(CLOCK_PERIOD/2);

  DebugReadMem #(
    .AXI_ADDR_W(AXI_ADDR_W),
    .AXI_DATA_W(AXI_DATA_W),
    .LEN_W(LEN_W)
  ) uut (
    .running(running),
    .run(run),
    .done(done),
    .clk(clk),
    .rst(rst),
    .address(address),
    .lastRead(lastRead),
    .databus_ready_0(databus_ready_0),
    .databus_valid_0(databus_valid_0),
    .databus_addr_0(databus_addr_0),
    .databus_rdata_0(databus_rdata_0),
    .databus_wdata_0(databus_wdata_0),
    .databus_wstrb_0(databus_wstrb_0),
    .databus_len_0(databus_len_0),
    .databus_last_0(databus_last_0)
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
    address = 0;
    insertValue = 0;
    addrToInsert = 0;
    valueToInsert = 0;

    `ADVANCE;

    rst = 1;

    `ADVANCE;

    rst = 0;

    `ADVANCE;

    // fill memory with data
    for(i=0;i<(2**AXI_ADDR_W);i=i+1) begin
        WriteMemory(i[AXI_ADDR_W-1:0],i[AXI_DATA_W-1:0]);
    end

    `ADVANCE;

    // read memory
    for(i=0;i<(2**AXI_ADDR_W);i=i+1) begin
        address = i[AXI_ADDR_W-1:0];
        RunAccelerator();
    end

    $finish();
  end

endmodule
// verilator coverage_on

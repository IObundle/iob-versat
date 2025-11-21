`timescale 1ns / 1ps

// verilator coverage_on
module LookupTableRead_tb (

);
  localparam DATA_W = 8;
  localparam ADDR_W = 2;
  localparam AXI_ADDR_W = 2;
  localparam AXI_DATA_W = 8;
  localparam DELAY_W = 2;
  localparam LEN_W = 2;
  // Inputs
  reg [(ADDR_W-2)-1:0] in0;
  // Outputs
  wire [(DATA_W)-1:0] out0;
  // Control
  reg [(1)-1:0] running;
  reg [(1)-1:0] run;
  wire [(1)-1:0] done;
  reg [(1)-1:0] clk;
  reg [(1)-1:0] rst;
  // Config
  reg [(AXI_ADDR_W)-1:0] ext_addr;
  reg [(ADDR_W)-1:0] iterA;
  reg [(ADDR_W)-1:0] perA;
  reg [(ADDR_W)-1:0] dutyA;
  reg [(ADDR_W)-1:0] shiftA;
  reg [(ADDR_W)-1:0] incrA;
  reg [(LEN_W)-1:0] length;
  reg [(1)-1:0] pingPong;
  reg [(1)-1:0] disabled;
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
  wire [(ADDR_W)-1:0] ext_dp_addr_0_port_0;
  wire [(AXI_DATA_W)-1:0] ext_dp_out_0_port_0;
  wire [(AXI_DATA_W)-1:0] ext_dp_in_0_port_0;
  wire [(1)-1:0] ext_dp_enable_0_port_0;
  wire [(1)-1:0] ext_dp_write_0_port_0;
  wire [(ADDR_W)-1:0] ext_dp_addr_0_port_1;
  wire [(AXI_DATA_W)-1:0] ext_dp_out_0_port_1;
  wire [(AXI_DATA_W)-1:0] ext_dp_in_0_port_1;
  wire [(1)-1:0] ext_dp_enable_0_port_1;
  wire [(1)-1:0] ext_dp_write_0_port_1;

  integer i;

  localparam CLOCK_PERIOD = 10;

  initial clk = 0;
  always #(CLOCK_PERIOD/2) clk = ~clk;
  `define ADVANCE @(posedge clk) #(CLOCK_PERIOD/2);

  LookupTableRead #(
    .DATA_W(DATA_W),
    .ADDR_W(ADDR_W),
    .AXI_ADDR_W(AXI_ADDR_W),
    .AXI_DATA_W(AXI_DATA_W),
    .DELAY_W(DELAY_W),
    .LEN_W(LEN_W)
  ) uut (
    .in0(in0),
    .out0(out0),
    .running(running),
    .run(run),
    .done(done),
    .clk(clk),
    .rst(rst),
    .ext_addr(ext_addr),
    .iterA(iterA),
    .perA(perA),
    .dutyA(dutyA),
    .shiftA(shiftA),
    .incrA(incrA),
    .length(length),
    .pingPong(pingPong),
    .disabled(disabled),
    .databus_ready_0(databus_ready_0),
    .databus_valid_0(databus_valid_0),
    .databus_addr_0(databus_addr_0),
    .databus_rdata_0(databus_rdata_0),
    .databus_wdata_0(databus_wdata_0),
    .databus_wstrb_0(databus_wstrb_0),
    .databus_len_0(databus_len_0),
    .databus_last_0(databus_last_0),
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

  my_dp_asym_ram #(
    .A_DATA_W(AXI_DATA_W),
    .B_DATA_W(AXI_DATA_W),
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
    iterA = 0;
    perA = 0;
    dutyA = 0;
    shiftA = 0;
    incrA = 0;
    length = 0;
    pingPong = 0;
    disabled = 0;
    insertValue = 0;
    addrToInsert = 0;
    valueToInsert = 0;

    `ADVANCE;

    rst = 1;

    `ADVANCE;

    rst = 0;

    in0 = {ADDR_W{1'b1}};

    ext_addr = {AXI_ADDR_W{1'b1}};
    iterA = {ADDR_W{1'b1}};
    perA = {ADDR_W{1'b1}};
    dutyA = {ADDR_W{1'b1}};
    shiftA = {ADDR_W{1'b1}};
    incrA = {ADDR_W{1'b1}};

    length = {LEN_W{1'b1}};
    pingPong = 1;
    disabled = 1;

    `ADVANCE;

    in0 = {ADDR_W{1'b0}};

    ext_addr = {AXI_ADDR_W{1'b0}};
    iterA = {ADDR_W{1'b0}};
    perA = {ADDR_W{1'b0}};
    dutyA = {ADDR_W{1'b0}};
    shiftA = {ADDR_W{1'b0}};
    incrA = {ADDR_W{1'b0}};

    length = {LEN_W{1'b0}};
    pingPong = 0;
    disabled = 0;

    // Write data to SimHelper_DatabusMem
    for(i=0;i<(2**AXI_ADDR_W);i=i+1) begin
      WriteMemory(i[AXI_ADDR_W-1:0],{AXI_DATA_W{i[0]}});
    end

    `ADVANCE;

    // Max AddressGen configuration
    in0 = {ADDR_W{1'b1}};

    ext_addr = {AXI_ADDR_W{1'b1}};
    iterA = {ADDR_W{1'b1}};
    perA = {ADDR_W{1'b1}};
    dutyA = {ADDR_W{1'b1}};
    shiftA = {ADDR_W{1'b1}};
    incrA = {ADDR_W{1'b1}};

    length = {LEN_W{1'b1}};
    pingPong = 1;
    disabled = 0;

    run = 1;
    running = 1;

    `ADVANCE;

    // run AddressGen with max configuration
    for(i=0;i<(2**(3*ADDR_W));i=i+1) begin
      if (i == 0) begin
          disabled = 1;
          pingPong = 0;
      end else begin
          disabled = 0;
          pingPong = 1;
      end
      in0 = i[ADDR_W-1:0];
      ext_addr = i[AXI_ADDR_W-1:0];
      `ADVANCE;
    end

    running = 0;
    run = 0;

    `ADVANCE;

    rst = 1;

    `ADVANCE;

    rst = 0;

    `ADVANCE;

    run = 1;

    `ADVANCE;

    `ADVANCE;

    run = 0;
    running = 1;

    // run AddressGen with max configuration
    for(i=0;i<(2**(3*ADDR_W));i=i+1) begin
      if (i == 0) begin
          disabled = 1;
          pingPong = 0;
      end else begin
          disabled = 0;
          pingPong = 1;
      end
      in0 = i[ADDR_W-1:0];
      ext_addr = i[AXI_ADDR_W-1:0];
      `ADVANCE;
    end

    running = 0;

    `ADVANCE;

    rst = 1;

    `ADVANCE;

    rst = 0;

    run = 1;
    running = 1;

    `ADVANCE;

    run = 0;

    // run AddressGen with max configuration
    for(i=0;i<(2**(3*ADDR_W));i=i+1) begin
      in0 = i[ADDR_W-1:0];
      ext_addr = i[AXI_ADDR_W-1:0];
      `ADVANCE;
    end

    running = 0;
    run = 0;

    `ADVANCE;
    rst = 1;

    `ADVANCE;

    rst = 0;

    $finish();
  end


endmodule
// verilator coverage_off

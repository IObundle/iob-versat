`timescale 1ns / 1ps

// verilator coverage_off
module LookupTable_tb (

);
  localparam DATA_W = 8;
  localparam SIZE_W = 8;
  localparam ADDR_W = 2;
  // Inputs
  reg [(DATA_W)-1:0] in0;
  reg [(DATA_W)-1:0] in1;
  // Outputs
  wire [(DATA_W)-1:0] out0;
  wire [(DATA_W)-1:0] out1;
  // Control
  reg [(1)-1:0] running;
  reg [(1)-1:0] clk;
  reg [(1)-1:0] rst;
  // MemoryMapped
  reg [(1)-1:0] valid;
  reg [(8)-1:0] addr;
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

  LookupTable #(
    .DATA_W(DATA_W),
    .SIZE_W(SIZE_W),
    .ADDR_W(ADDR_W)
  ) uut (
    .in0(in0),
    .in1(in1),
    .out0(out0),
    .out1(out1),
    .running(running),
    .clk(clk),
    .rst(rst),
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
    clk = 0;
    rst = 0;
    valid = 0;
    addr = 0;
    wstrb = 0;
    wdata = 0;

    `ADVANCE;

    rst = 1;

    `ADVANCE;

    rst = 0;

    `ADVANCE;

    for(i=0;i<(2**ADDR_W);i=i+1) begin
        Write(i[ADDR_W-1:0],{DATA_W{i[0]}});
    end

    `ADVANCE;

    wstrb = 0;
    addr = 0;
    valid = 1;

    while(rvalid == 0) begin
        `ADVANCE;
    end

    `ADVANCE;

    running = 1;

    for(i=0;i<(2**ADDR_W);i=i+1) begin
        in0 = {DATA_W{i[0]}};
        in1 = {DATA_W{i[0]}};
        `ADVANCE;
    end

    `ADVANCE;
    `ADVANCE;
    `ADVANCE;

    running = 0;
    rst = 1;

    `ADVANCE;

    $finish();
  end

endmodule
// verilator coverage_on

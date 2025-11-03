`timescale 1ns / 1ps

// verilator coverage_off
module RegFile_tb (

);
  localparam DELAY_W = 2;
  localparam ADDR_W = 6;
  localparam DATA_W = 8;
  localparam AXI_DATA_W = 8;
  // Inputs
  reg [(DATA_W)-1:0] in0;
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
  reg [(4)-1:0] selectedInput;
  reg [(4)-1:0] selectedOutput0;
  reg [(4)-1:0] selectedOutput1;
  reg [(1)-1:0] disabled;
  // Delays
  reg [(DELAY_W)-1:0] delay0;
  // MemoryMapped
  reg [(1)-1:0] valid;
  reg [(6)-1:0] addr;
  reg [(DATA_W/8)-1:0] wstrb;
  reg [(DATA_W)-1:0] wdata;
  wire [(1)-1:0] rvalid;
  wire [(DATA_W)-1:0] rdata;
  // ExternalMemory
  wire [(ADDR_W)-1:0] ext_2p_addr_out_0;
  wire [(ADDR_W)-1:0] ext_2p_addr_in_0;
  wire [(1)-1:0] ext_2p_write_0;
  wire [(1)-1:0] ext_2p_read_0;
  wire [(AXI_DATA_W)-1:0] ext_2p_data_in_0;
  wire [(AXI_DATA_W)-1:0] ext_2p_data_out_0;
  wire [(ADDR_W)-1:0] ext_2p_addr_out_1;
  wire [(ADDR_W)-1:0] ext_2p_addr_in_1;
  wire [(1)-1:0] ext_2p_write_1;
  wire [(1)-1:0] ext_2p_read_1;
  wire [(AXI_DATA_W)-1:0] ext_2p_data_in_1;
  wire [(AXI_DATA_W)-1:0] ext_2p_data_out_1;

  integer i;

  localparam CLOCK_PERIOD = 10;

  initial clk = 0;
  always #(CLOCK_PERIOD/2) clk = ~clk;
  `define ADVANCE @(posedge clk) #(CLOCK_PERIOD/2);

  RegFile #(
    .DELAY_W(DELAY_W),
    .ADDR_W(ADDR_W),
    .DATA_W(DATA_W),
    .AXI_DATA_W(AXI_DATA_W)
  ) uut (
    .in0(in0),
    .out0(out0),
    .out1(out1),
    .running(running),
    .run(run),
    .done(done),
    .clk(clk),
    .rst(rst),
    .selectedInput(selectedInput),
    .selectedOutput0(selectedOutput0),
    .selectedOutput1(selectedOutput1),
    .disabled(disabled),
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
    .ext_2p_data_out_0(ext_2p_data_out_0),
    .ext_2p_addr_out_1(ext_2p_addr_out_1),
    .ext_2p_addr_in_1(ext_2p_addr_in_1),
    .ext_2p_write_1(ext_2p_write_1),
    .ext_2p_read_1(ext_2p_read_1),
    .ext_2p_data_in_1(ext_2p_data_in_1),
    .ext_2p_data_out_1(ext_2p_data_out_1)
  );


  task Write (input [6-1:0] addr_i,input [DATA_W-1:0] data_i);
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
    .W_DATA_W(AXI_DATA_W),
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

  my_2p_asym_ram #(
    .W_DATA_W(AXI_DATA_W),
    .R_DATA_W(AXI_DATA_W),
    .ADDR_W(ADDR_W)
  ) ext_2p_1 (
    .w_en_i(ext_2p_write_1),
    .w_addr_i(ext_2p_addr_out_1),
    .w_data_i(ext_2p_data_out_1),
    .r_en_i(ext_2p_read_1),
    .r_addr_i(ext_2p_addr_in_1),
    .r_data_o(ext_2p_data_in_1),
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
    selectedInput = 0;
    selectedOutput0 = 0;
    selectedOutput1 = 0;
    disabled = 0;
    delay0 = 0;
    valid = 0;
    addr = 0;
    wstrb = 0;
    wdata = 0;

    `ADVANCE;

    rst = 1;

    `ADVANCE;

    disabled = 1'b1;
    rst = 0;

    `ADVANCE;

    disabled = 1'b0;

    `ADVANCE;

    // Write data to memories
    for(i=0;i<(2**ADDR_W);i=i+1) begin
        Write(i[ADDR_W-1:0],{DATA_W{i[0]}});
    end

    // Read back data
    for(i=0;i<(2**ADDR_W);i=i+1) begin

        `ADVANCE;

        valid = 1;
        wstrb = 0;
        addr = i[ADDR_W-1:0];

        `ADVANCE;

        while(~rvalid) begin
          `ADVANCE;
        end

        valid = 0;
    end

    // Run accelerator
    disabled = 1'b0;
    for(i=0;i<(2**DELAY_W);i=i+1) begin
        delay0 = i[DELAY_W-1:0];
    end

    for(i=0;i<(2**(4+1));i=i+1) begin
        in0 = {DATA_W{i[0]}};
        delay0 = i[DELAY_W-1:0];
        selectedInput = i[3:0];
        selectedOutput0 = i[3:0];
        selectedOutput1 = i[3:0];
        RunAccelerator();
    end

    $finish();
  end

endmodule
// verilator coverage_on

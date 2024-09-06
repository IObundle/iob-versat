`timescale 1ns / 1ps

module VRead #(
   parameter SIZE_W     = 32,
   parameter DATA_W     = 32,
   parameter ADDR_W     = 14,
   parameter PERIOD_W   = 12, // Must be 2 less than ADDR_W (boundary of 4) (for 32 bit DATA_W)
   parameter AXI_ADDR_W = 32,
   parameter AXI_DATA_W = 32,
   parameter DELAY_W    = 7,
   parameter LEN_W      = 8
) (
   input clk,
   input rst,

   input  running,
   input  run,
   output done,

   // Databus interface
   input                         databus_ready_0,
   output                        databus_valid_0,
   output reg [  AXI_ADDR_W-1:0] databus_addr_0,
   input      [  AXI_DATA_W-1:0] databus_rdata_0,
   output     [  AXI_DATA_W-1:0] databus_wdata_0,
   output     [AXI_DATA_W/8-1:0] databus_wstrb_0,
   output     [       LEN_W-1:0] databus_len_0,
   input                         databus_last_0,

   // input / output data
   (* versat_latency = 1 *) output [DATA_W-1:0] out0,

   // External memory
   output [    ADDR_W-1:0] ext_2p_addr_out_0,
   output [    ADDR_W-1:0] ext_2p_addr_in_0,
   output                  ext_2p_write_0,
   output                  ext_2p_read_0,
   input  [AXI_DATA_W-1:0] ext_2p_data_in_0,
   output [AXI_DATA_W-1:0] ext_2p_data_out_0,

   input [AXI_ADDR_W-1:0] ext_addr,
   input [  PERIOD_W-1:0] perA,
   input [    ADDR_W-1:0] incrA,
   input [     LEN_W-1:0] length,
   input                  pingPong,

   // configurations
`ifdef COMPLEX_INTERFACE
   input [PERIOD_W-1:0] dutyA,
   input [  ADDR_W-1:0] int_addr,
   input [  ADDR_W-1:0] iterA,
   input [  ADDR_W-1:0] shiftA,
`endif

   input [  ADDR_W-1:0] iterB,
   input [PERIOD_W-1:0] perB,
   input [PERIOD_W-1:0] dutyB,
   input [  ADDR_W-1:0] startB,
   input [  ADDR_W-1:0] shiftB,
   input [  ADDR_W-1:0] incrB,
   input [  ADDR_W-1:0] iter2B,
   input [PERIOD_W-1:0] per2B,
   input [  ADDR_W-1:0] shift2B,
   input [  ADDR_W-1:0] incr2B,

   input enableRead,

   input [DELAY_W-1:0] delay0
);

   assign databus_wdata_0 = 0;
   assign databus_wstrb_0 = 0;
   assign databus_len_0   = length;

   // output databus

   reg               doneRead;
   reg               doneOutput;
   wire              doneOutput_int;

   assign done = (doneRead  & doneOutput);

   always @(posedge clk, posedge rst) begin
      if (rst) begin
         doneRead <= 1'b1;
         doneOutput <= 1'b1;
      end else if (run) begin
         doneRead <= !enableRead;
         doneOutput <= 1'b0;
      end else begin
         if (databus_valid_0 && databus_ready_0 && databus_last_0) doneRead <= 1'b1;
         if (doneOutput_int) doneOutput <= 1'b1;
      end
   end

   // Ping pong and related logic for the initial address
   reg pingPongState;

   wire [ADDR_W-1:0] startA = {pingPong && !pingPongState , {(ADDR_W-1){1'b0}}};

   // port addresses and enables
   wire [ADDR_W-1:0] addrB;
   wire [ADDR_W-1:0] startB_inst = {pingPong && pingPongState, startB[ADDR_W-2:0]};

   // Ping pong 
   always @(posedge clk, posedge rst) begin
      if (rst) pingPongState <= 0;
      else if (run) pingPongState <= pingPong ? (!pingPongState) : 1'b0;
   end

   wire gen_valid, gen_ready;
   wire [ADDR_W-1:0] gen_addr;

   always @(posedge clk, posedge rst) begin
      if (rst) databus_addr_0 <= 0;
      else if (run && enableRead) databus_addr_0 <= ext_addr;
   end

   SimpleAddressGen #(
      .ADDR_W(ADDR_W),
      .DATA_W(SIZE_W),
      .PERIOD_W(PERIOD_W)
   ) addrgenRead (
      .clk_i(clk),
      .rst_i(rst),
      .run_i(run && enableRead),

      //configurations 
      .period_i(perA),
      .delay_i (0),
      .start_i (startA),
      .incr_i  (incrA),

`ifdef COMPLEX_INTERFACE
      .iterations_i(iterA),
      .duty_i      (dutyA),
      .shift_i     (shiftA),
`endif

      //outputs 
      .valid_o(gen_valid),
      .ready_i(gen_ready),
      .addr_o (gen_addr),
      .done_o ()
   );

   // mem enables output by addr gen
   wire enB;

   SimpleAddressGen #(
      .ADDR_W(ADDR_W),
      .DATA_W(DATA_W),
      .PERIOD_W(PERIOD_W)
   ) addrgenOutput (
      .clk_i(clk),
      .rst_i(rst),
      .run_i(run),

      //configurations 
      .period_i(perB),
      .delay_i (delay0),
      .start_i (startB_inst),
      .incr_i  (incrB),

`ifdef COMPLEX_INTERFACE
      .iterations_i(iterB),
      .duty_i      (dutyB),
      .shift_i     (shiftB),
`endif

      //outputs 
      .valid_o(enB),
      .ready_i(1'b1),
      .addr_o (addrB),
      .done_o (doneOutput_int)
   );

   wire                  write_en;
   wire [    ADDR_W-1:0] write_addr;
   wire [AXI_DATA_W-1:0] write_data;

   wire                  data_ready;

   MemoryWriter #(
      .ADDR_W(ADDR_W),
      .DATA_W(AXI_DATA_W)
   ) writer (
      .gen_valid_i(gen_valid),
      .gen_ready_o(gen_ready),
      .gen_addr_i (gen_addr),

      // Slave connected to data source
      .data_valid_i(databus_ready_0),
      .data_ready_o(data_ready),
      .data_in_i   (databus_rdata_0),

      // Connect to memory
      .mem_enable_o(write_en),
      .mem_addr_o  (write_addr),
      .mem_data_o  (write_data),

      .clk_i(clk),
      .rst_i(rst)
   );

   assign databus_valid_0 = (data_ready && !doneRead);

   localparam DIFF = AXI_DATA_W / DATA_W;
   localparam DECISION_BIT_W = $clog2(DIFF);

   function [ADDR_W-DECISION_BIT_W-1:0] symbolSpaceConvert(input [ADDR_W-1:0] in);
      reg [ADDR_W-1:0] noPingPong;
      reg [ADDR_W-1:0] shiftRes;
      begin
         noPingPong           = in;
         noPingPong[ADDR_W-1] = 1'b0;
         shiftRes             = noPingPong >> DECISION_BIT_W;
         symbolSpaceConvert   = shiftRes[ADDR_W-DECISION_BIT_W-1:0];
      end
   endfunction

   reg [ADDR_W-1:0] addr_out;

   generate
      if (AXI_DATA_W > DATA_W) begin
         always @* begin
            addr_out                            = 0;
            addr_out[ADDR_W-DECISION_BIT_W-1:0] = symbolSpaceConvert(addrB[ADDR_W-1:0]);
            addr_out[ADDR_W-1]                  = pingPong && addrB[ADDR_W-1];
         end

         reg [DECISION_BIT_W-1:0] sel_0;  // Matches addr_0_port_0
         always @(posedge clk, posedge rst) begin
            if (rst) begin
               sel_0 <= 0;
            end else begin
               sel_0 <= addrB[DECISION_BIT_W-1:0];
            end
         end

         WideAdapter #(
            .INPUT_W (AXI_DATA_W),
            .OUTPUT_W(DATA_W),
            .SIZE_W  (SIZE_W)
         ) adapter (
            .sel_i(sel_0),
            .in_i (ext_2p_data_in_0),
            .out_o(out0)
         );
      end else begin
         always @* begin
            addr_out = addrB;
         end
         assign out0 = ext_2p_data_in_0;
      end  // if(AXI_DATA_W > DATA_W)
   endgenerate

   assign ext_2p_write_0    = write_en;
   assign ext_2p_addr_out_0 = write_addr;
   assign ext_2p_data_out_0 = write_data;

   assign ext_2p_read_0     = enB;
   assign ext_2p_addr_in_0  = addr_out;

endmodule

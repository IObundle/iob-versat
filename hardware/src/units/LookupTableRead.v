`timescale 1ns / 1ps

module LookupTableRead #(
   parameter DATA_W     = 32,
   parameter SIZE_W     = 32,
   parameter ADDR_W     = 16,
   parameter AXI_ADDR_W = 32,
   parameter AXI_DATA_W = 32,
   parameter LEN_W      = 8
) (
   //databus interface
   input                         databus_ready_0,
   output                        databus_valid_0,
   output reg [  AXI_ADDR_W-1:0] databus_addr_0,
   input      [  AXI_DATA_W-1:0] databus_rdata_0,
   output     [  AXI_DATA_W-1:0] databus_wdata_0,
   output     [AXI_DATA_W/8-1:0] databus_wstrb_0,
   output     [       LEN_W-1:0] databus_len_0,
   input                         databus_last_0,

   output reg done,

   input [DATA_W-1:0] in0,

   (* versat_latency = 2 *) output [DATA_W-1:0] out0,

   // Used by lookup table
   output reg [    ADDR_W-1:0] ext_dp_addr_0_port_0,
   output     [AXI_DATA_W-1:0] ext_dp_out_0_port_0,
   input      [AXI_DATA_W-1:0] ext_dp_in_0_port_0,
   output                      ext_dp_enable_0_port_0,
   output                      ext_dp_write_0_port_0,

   // Used to write data into lookup table
   output [    ADDR_W-1:0] ext_dp_addr_0_port_1,
   output [AXI_DATA_W-1:0] ext_dp_out_0_port_1,
   input  [AXI_DATA_W-1:0] ext_dp_in_0_port_1,
   output                  ext_dp_enable_0_port_1,
   output                  ext_dp_write_0_port_1,

   // configurations
   input [AXI_ADDR_W-1:0] ext_addr,
   input [    ADDR_W-1:0] int_addr,
   input [    ADDR_W-1:0] iterA,
   input [    ADDR_W-1:0] perA,
   input [    ADDR_W-1:0] dutyA,
   input [    ADDR_W-1:0] shiftA,
   input [    ADDR_W-1:0] incrA,
   input [     LEN_W-1:0] length,
   input                  pingPong,

   input disabled,

   input [31:0] delay0,

   input running,
   input clk,
   input rst,
   input run
);

   assign ext_dp_out_0_port_0 = 0;

   localparam DIFF = AXI_DATA_W / DATA_W;
   localparam DECISION_BIT_W = $clog2(DIFF);

   /*
   function [ADDR_W-DECISION_BIT_W-1:0] symbolSpaceConvert(input [ADDR_W-1:0] in);
      reg [ADDR_W-1:0] shiftRes;
      begin
         shiftRes = in >> DECISION_BIT_W;
         symbolSpaceConvert = shiftRes[ADDR_W-DECISION_BIT_W-1:0];
      end
   endfunction
   */

   reg              pingPongState;

   reg [ADDR_W-1:0] addr_0;
   always @* begin
      addr_0 = 0;
      addr_0[ADDR_W-1:0] = {
         in0[ADDR_W-3:0], 2'b00
      };  // Increments by 4 because SIZE_W == 32 //symbolSpaceConvert(in0[ADDR_W-1:0]);
      addr_0[ADDR_W-1] = pingPong && !pingPongState;
   end

   always @(posedge clk, posedge rst) begin
      if (rst) ext_dp_addr_0_port_0 <= 0;
      else ext_dp_addr_0_port_0 <= addr_0;
   end

   generate
      if (AXI_DATA_W > DATA_W) begin
         // Byte selector needs to be saved to later indicate which lane to output (for 64)
         reg [DECISION_BIT_W-1:0] sel_0;  // Matches addr_0_port_0
         reg [DECISION_BIT_W-1:0] sel_1;  // Matches rdata_0_port_0
         always @(posedge clk, posedge rst) begin
            if (rst) begin
               sel_0 <= 0;
               sel_1 <= 0;
            end else begin
               sel_0 <= in0[DECISION_BIT_W-1:0];
               sel_1 <= sel_0;
            end
         end

         WideAdapter #(
            .INPUT_W (AXI_DATA_W),
            .OUTPUT_W(DATA_W)
         ) adapter (
            .sel_i(sel_1),
            .in_i (ext_dp_in_0_port_0),
            .out_o(out0)
         );
      end else begin
         assign out0 = ext_dp_in_0_port_0;
      end
   endgenerate

   assign ext_dp_enable_0_port_0 = 1'b1;
   assign ext_dp_write_0_port_0  = 1'b0;

   assign databus_wdata_0        = 0;
   assign databus_wstrb_0        = 0;
   assign databus_len_0          = length;

   always @(posedge clk, posedge rst) begin
      if (rst) begin
         done <= 1'b1;
      end else if (run && !disabled) begin
         done <= 1'b0;
      end else begin
         if (databus_valid_0 && databus_ready_0 && databus_last_0) done <= 1'b1;
      end
   end

   // Ping pong 
   always @(posedge clk, posedge rst) begin
      if (rst) pingPongState <= 0;
      else if (run)  // && !disabled
         pingPongState <= pingPong ? (!pingPongState) : 1'b0;
   end

   always @(posedge clk, posedge rst) begin
      if (rst) databus_addr_0 <= 0;
      else if (run && !disabled) databus_addr_0 <= ext_addr;
   end

   wire [ADDR_W-1:0] startA = 0;
   wire [      31:0] delayA = 0;
   wire [ADDR_W-1:0] addrA, addrA_int, addrA_int2;

   wire              next;
   wire              gen_ready;
   wire [ADDR_W-1:0] gen_addr;
   wire              gen_done;

   MyAddressGen #(
      .ADDR_W  (ADDR_W),
      .DATA_W  (AXI_DATA_W),
      .PERIOD_W(ADDR_W)
   ) addrgenA (
      .clk_i(clk),
      .rst_i(rst),
      .run_i(run && !disabled),

      //configurations 
      .iterations_i(iterA),
      .period_i    (perA),
      .duty_i      (dutyA),
      .delay_i     (delayA),
      .start_i     (startA),
      .shift_i     (shiftA),
      .incr_i      (incrA),

      //outputs 
      .valid_o(),
      .ready_i(gen_ready),
      .addr_o (gen_addr),
      .done_o (gen_done)
   );

   wire                  write_en;
   wire [    ADDR_W-1:0] write_addr;
   wire [AXI_DATA_W-1:0] write_data;

   reg                   gen_valid;
   always @(posedge clk, posedge rst) begin
      if (rst) begin
         gen_valid <= 1'b0;
      end else if (run) begin
         gen_valid <= !disabled;
      end else if (running) begin
         if (databus_valid_0 && databus_ready_0 && databus_last_0) begin
            gen_valid <= 1'b0;
         end
      end
   end

   MemoryWriter #(
      .ADDR_W(ADDR_W),
      .DATA_W(AXI_DATA_W)
   ) writer (
      .gen_valid_i(gen_valid),
      .gen_ready_o(gen_ready),
      .gen_addr_i (gen_addr),

      // Slave connected to data source
      .data_valid_i(databus_ready_0),
      .data_ready_o(databus_valid_0),
      .data_in_i   (databus_rdata_0),

      // Connect to memory
      .mem_enable_o(write_en),
      .mem_addr_o  (write_addr),
      .mem_data_o  (write_data),

      .clk_i(clk),
      .rst_i(rst)
   );

   assign ext_dp_addr_0_port_1 = pingPong ? {(pingPongState) ^ write_addr[ADDR_W-1],write_addr[ADDR_W-2:0]} : write_addr;
   assign ext_dp_enable_0_port_1 = write_en;
   assign ext_dp_write_0_port_1 = 1'b1;
   assign ext_dp_out_0_port_1 = write_data;


endmodule

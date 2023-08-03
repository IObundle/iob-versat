`timescale 1ns / 1ps

`default_nettype none
module VRead #(
   parameter DATA_W = 32,
   parameter ADDR_W = 12,
   parameter PERIOD_W = 10,
   parameter AXI_ADDR_W = 32,
   parameter AXI_DATA_W = 32,
   parameter LEN_W = 8
   )
   (
   input                  clk,
   input                  rst,

   input                  running,
   input                  run,
   output                 done,

   // Databus interface
   input                       databus_ready_0,
   output                      databus_valid_0,
   output reg [AXI_ADDR_W-1:0] databus_addr_0,
   input [AXI_DATA_W-1:0]      databus_rdata_0,
   output [AXI_DATA_W-1:0]     databus_wdata_0,
   output [AXI_DATA_W/8-1:0]   databus_wstrb_0,
   output [LEN_W-1:0]          databus_len_0,
   input                       databus_last_0,

   // input / output data
   (* versat_latency = 1 *) output [DATA_W-1:0]    out0,

   // External memory
   output [ADDR_W-1:0]     ext_2p_addr_out_0,
   output [ADDR_W-1:0]     ext_2p_addr_in_0,
   output                  ext_2p_write_0,
   output                  ext_2p_read_0,
   input  [AXI_DATA_W-1:0] ext_2p_data_in_0,
   output [AXI_DATA_W-1:0] ext_2p_data_out_0,

   // configurations
   input [AXI_ADDR_W-1:0] ext_addr,
   input [ADDR_W-1:0] int_addr,
   input [10:0]  size,
   input [ADDR_W-1:0] iterA,
   input [PERIOD_W-1:0]   perA,
   input [PERIOD_W-1:0]   dutyA,
   input [ADDR_W-1:0] shiftA,
   input [ADDR_W-1:0] incrA,
   input [LEN_W-1:0]      length,
   input                  pingPong,

   input [ADDR_W-1:0] iterB,
   input [PERIOD_W-1:0]   perB,
   input [PERIOD_W-1:0]   dutyB,
   input [ADDR_W-1:0] startB,
   input [ADDR_W-1:0] shiftB,
   input [ADDR_W-1:0] incrB,
   input                  reverseB,
   input                  extB,
   input [ADDR_W-1:0] iter2B,
   input [PERIOD_W-1:0]   per2B,
   input [ADDR_W-1:0] shift2B,
   input [ADDR_W-1:0] incr2B,

   input                  disabled,

   input [31:0]            delay0
   );

   assign databus_wdata_0 = 0;
   assign databus_wstrb_0 = 0;
   assign databus_len_0 = length;
   
   // output databus
   wire [DATA_W-1:0] outB;
   
   wire gen_done;
   reg doneA;
   reg doneB;
   wire doneB_int;
   assign out0 = outB;
   assign done = doneA & doneB;

   always @(posedge clk,posedge rst)
   begin
      if(rst) begin
         doneA <= 1'b1;
         doneB <= 1'b1;
      end else if(run && !disabled) begin
         doneA <= 1'b0;
         doneB <= 1'b0;
      end else begin 
         if(databus_valid_0 && databus_ready_0 && databus_last_0)
            doneA <= 1'b1;
         if(doneB_int)
            doneB <= 1'b1;
      end
   end

   function [ADDR_W-1:0] reverseBits;
      input [ADDR_W-1:0]   word;
      integer                   i;

      begin
        for (i=0; i < ADDR_W; i=i+1)
          reverseBits[i] = word[ADDR_W-1 - i];
      end
   endfunction

   wire [1:0]             direction = 2'b01;
   reg [ADDR_W-1:0] startA;

   reg                    pingPongState;

   always @*
   begin
      startA = 0;
      startA[ADDR_W-1] = !pingPongState;
   end

   wire [31:0]   delayA    = 0;

   // port addresses and enables
   wire [ADDR_W-1:0] addrA, addrA_int, addrA_int2;
   wire [ADDR_W-1:0] addrB, addrB_int, addrB_int2;

   // data inputs
   wire                   req;
   wire                   rnw;

   wire [ADDR_W-1:0]      startB_inst = pingPong ? {pingPongState,startB[ADDR_W-2:0]} : startB;

   // mem enables output by addr gen
   wire enB;

   // write enables

   // Ping pong 
   always @(posedge clk,posedge rst)
   begin
      if(rst)
         pingPongState <= 0;
      else if(run && !disabled)
         pingPongState <= pingPong ? (!pingPongState) : 1'b0;
   end

   wire next;
   wire gen_valid,gen_ready;
   wire [ADDR_W-1:0] gen_addr;

   always @(posedge clk,posedge rst)
   begin
      if(rst)
         databus_addr_0 <= 0;
      else if(run && !disabled)
         databus_addr_0 <= ext_addr;
   end

   MyAddressGen #(.ADDR_W(ADDR_W)) addrgenA(
      .clk(clk),
      .rst(rst),
      .run(run && !disabled),

      //configurations 
      .iterations(iterA),
      .period(perA),
      .duty(dutyA),
      .delay(delayA),
      .start(startA),
      .shift(shiftA),
      .incr(incrA),

      //outputs 
      .valid(gen_valid),
      .ready(gen_ready),
      .addr(gen_addr),
      .done(gen_done)
      );

    xaddrgen2 #(.MEM_ADDR_W(ADDR_W)) addrgen2B (
                       .clk(clk),
                       .rst(rst),
                       .run(run && !disabled),
                       .iterations(iterB),
                       .period(perB),
                       .duty(dutyB),
                       .start(startB_inst),
                       .shift(shiftB),
                       .incr(incrB),
                       .delay(delay0[9:0]),
                       .iterations2(iter2B),
                       .period2(per2B),
                       .shift2(shift2B),
                       .incr2(incr2B),
                       .addr(addrB_int),
                       .mem_en(enB),
                       .done(doneB_int)
                       );

   assign addrA = addrA_int2;
   assign addrB = addrB_int2;

   assign addrA_int2 = addrA_int;
   assign addrB_int2 = reverseB ? reverseBits(addrB_int) : addrB_int;
   
   wire write_en;
   wire [ADDR_W-1:0] write_addr;
   wire [AXI_DATA_W-1:0] write_data;
   
   MemoryWriter #(.ADDR_W(ADDR_W),.DATA_W(AXI_DATA_W)) 
   writer(
      .gen_valid(gen_valid),
      .gen_ready(gen_ready),
      .gen_addr(gen_addr),

      // Slave connected to data source
      .data_valid(databus_ready_0),
      .data_ready(databus_valid_0),
      .data_in(databus_rdata_0),

      // Connect to memory
      .mem_enable(write_en),
      .mem_addr(write_addr),
      .mem_data(write_data),

      .clk(clk),
      .rst(rst)
      );

   localparam DIFF = AXI_DATA_W/DATA_W;
   localparam DECISION_BIT_W = $clog2(DIFF);

   function [ADDR_W-DECISION_BIT_W-1:0] symbolSpaceConvert(input [ADDR_W-1:0] in);
      reg [ADDR_W-1:0] shiftRes;
      begin
         shiftRes = in >> DECISION_BIT_W;
         symbolSpaceConvert = shiftRes[ADDR_W-DECISION_BIT_W-1:0];
      end
   endfunction

   reg [ADDR_W-1:0] addr_out;

   generate 
   if(AXI_DATA_W > DATA_W) begin
      always @* begin
         addr_out = 0;
         addr_out[ADDR_W-DECISION_BIT_W-1:0] = symbolSpaceConvert(addrB[ADDR_W-1:0]);
         addr_out[ADDR_W-1] = pingPong && addrB[ADDR_W-1];
      end

      reg[DECISION_BIT_W-1:0] sel_0; // Matches addr_0_port_0
      reg[DECISION_BIT_W-1:0] sel_1; // Matches rdata_0_port_0
      always @(posedge clk,posedge rst) begin
         if(rst) begin
            sel_0 <= 0;
            sel_1 <= 0;
         end else begin
            sel_0 <= addrB[DECISION_BIT_W-1];
            sel_1 <= sel_0;
         end
      end

      WideAdapter #(
      .INPUT_W(AXI_DATA_W),
      .OUTPUT_W(DATA_W)
      )
      adapter
      (
         .sel(sel_1),
         .in(ext_2p_data_in_0),
         .out(outB)
      );
   end else begin
      always @* begin
         addr_out = addrB;
      end
      assign outB = ext_2p_data_in_0;
   end // if(AXI_DATA_W > DATA_W)
   endgenerate

   assign ext_2p_write_0 = write_en;
   assign ext_2p_addr_out_0 = write_addr;
   assign ext_2p_data_out_0 = write_data;

   assign ext_2p_read_0 = enB;
   assign ext_2p_addr_in_0 = addr_out;

endmodule
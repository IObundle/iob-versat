`timescale 1ns / 1ps

module LookupTableRead #(
       parameter DATA_W = 32,
       parameter ADDR_W = 12,
       parameter AXI_ADDR_W = 64
   )
   (
      //databus interface
      input                 databus_ready_0,
      output                databus_valid_0,
      output reg [AXI_ADDR_W-1:0]     databus_addr_0,
      input [DATA_W-1:0]    databus_rdata_0,
      output [DATA_W-1:0]   databus_wdata_0,
      output [DATA_W/8-1:0] databus_wstrb_0,
      output [7:0]          databus_len_0,
      input                 databus_last_0,

      output reg done,

      input [DATA_W-1:0] in0,

      (* versat_latency = 2 *) output reg [DATA_W-1:0] out0,

      // Used by lookup table
      output reg [ADDR_W-1:0] ext_dp_addr_0_port_0,
      output [DATA_W-1:0]   ext_dp_out_0_port_0,
      input  [DATA_W-1:0]   ext_dp_in_0_port_0,
      output                ext_dp_enable_0_port_0,
      output                ext_dp_write_0_port_0,

      // Used by read
      output [ADDR_W-1:0]   ext_dp_addr_0_port_1,
      output [DATA_W-1:0]   ext_dp_out_0_port_1,
      input  [DATA_W-1:0]   ext_dp_in_0_port_1,
      output                ext_dp_enable_0_port_1,
      output                ext_dp_write_0_port_1,

       // configurations
      input [AXI_ADDR_W-1:0] ext_addr,
      input [ADDR_W-1:0] int_addr,
      input [31:0]       size,
      input [ADDR_W-1:0] iterA,
      input [9:0]        perA,
      input [9:0]        dutyA,
      input [ADDR_W-1:0] shiftA,
      input [ADDR_W-1:0] incrA,
      input [7:0]        length,
      input              pingPong,

      input [9:0] iterB,
      input [9:0] perB,
      input [9:0] dutyB,
      input [9:0] startB,
      input [9:0] shiftB,
      input [9:0] incrB,
      input       reverseB,
      input       extB,
      input [9:0] iter2B,
      input [9:0] per2B,
      input [9:0] shift2B,
      input [9:0] incr2B,

      input [31:0] delay0,

      input running,
      input clk,
      input rst,
      input run
   );

   reg pingPongState;

   always @(posedge clk,posedge rst) begin
      if(rst)
         ext_dp_addr_0_port_0 <= 0;
      else
         ext_dp_addr_0_port_0 <= {pingPongState ^ in0[ADDR_W-1],in0[ADDR_W-2:0]};
   end

   assign ext_dp_enable_0_port_0 = 1'b1;
   assign ext_dp_write_0_port_0 = 1'b0;
   assign out0 = ext_dp_in_0_port_0;

   assign databus_wdata_0 = 0;
   assign databus_wstrb_0 = 4'b0000;
   assign databus_len_0 = length;

   always @(posedge clk,posedge rst)
   begin
      if(rst) begin
         done <= 1'b1;
      end else if(run) begin
         done <= 1'b0;
      end else begin 
         if(databus_valid_0 && databus_ready_0 && databus_last_0)
            done <= 1'b1;
      end
   end

   // Ping pong 
   always @(posedge clk,posedge rst)
   begin
      if(rst)
         pingPongState <= 0;
      else if(run)
         pingPongState <= pingPong ? (!pingPongState) : 1'b0;
   end
   
   always @(posedge clk,posedge rst)
   begin
      if(rst)
         databus_addr_0 <= 0;
      else if(run)
         databus_addr_0 <= ext_addr;
   end

   wire [ADDR_W-1:0] startA = 0;
   wire [31:0]   delayA    = 0;
   wire [ADDR_W-1:0] addrA, addrA_int, addrA_int2;

   wire next;
   wire gen_valid,gen_ready;
   wire [ADDR_W-1:0] gen_addr;
   wire gen_done;

   MyAddressGen #(.ADDR_W(ADDR_W)) addrgenA(
      .clk(clk),
      .rst(rst),
      .run(run),

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

   wire write_en;
   wire [ADDR_W-1:0] write_addr;
   wire [DATA_W-1:0] write_data;
   
   MemoryWriter #(.ADDR_W(ADDR_W)) 
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

   assign ext_dp_addr_0_port_1 = {(!pingPongState) ^ write_addr[ADDR_W-1],write_addr[ADDR_W-2:0]};
   assign ext_dp_enable_0_port_1 = write_en;
   assign ext_dp_write_0_port_1 = 1'b1;
   assign ext_dp_out_0_port_1 = write_data;


endmodule

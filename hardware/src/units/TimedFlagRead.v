`timescale 1ns / 1ps

module TimedFlagRead #(
   parameter DATA_W = 32,
   parameter SIZE_W = 16,
   parameter ADDR_W = 16,
   parameter AXI_ADDR_W = 32,
   parameter AXI_DATA_W = 32,
   parameter LEN_W = 8
   )(
   input                  clk,
   input                  rst,

   input                  running,
   input                  run,
   output reg             done,

   //databus interface
   input                       databus_ready_0,
   output                      databus_valid_0,
   output reg [AXI_ADDR_W-1:0] databus_addr_0,
   input [AXI_DATA_W-1:0]      databus_rdata_0,
   output [AXI_DATA_W-1:0]     databus_wdata_0,
   output [AXI_DATA_W/8-1:0]   databus_wstrb_0,
   output [LEN_W-1:0]          databus_len_0,
   input                       databus_last_0,

   // read port 
   output [ADDR_W-1:0]   ext_dp_addr_0_port_0, // ADDR set to SIZE_W == 16 and AXI_ADDR_W == 32.
   output [SIZE_W-1:0]   ext_dp_out_0_port_0,
   input  [SIZE_W-1:0]   ext_dp_in_0_port_0,
   output                ext_dp_enable_0_port_0,
   output                ext_dp_write_0_port_0,

   // connect directly to databus
   output [ADDR_W-1:0]   ext_dp_addr_0_port_1,
   output [AXI_DATA_W-1:0]   ext_dp_out_0_port_1,
   input  [AXI_DATA_W-1:0]   ext_dp_in_0_port_1,
   output                ext_dp_enable_0_port_1,
   output                ext_dp_write_0_port_1,

   // configurations
   input [AXI_ADDR_W-1:0] ext_addr,
   input [ADDR_W-1:0]     int_addr,
   input [ADDR_W-1:0]     iterA,
   input [ADDR_W-1:0]     perA,
   input [ADDR_W-1:0]     dutyA,
   input [ADDR_W-1:0]     shiftA,
   input [ADDR_W-1:0]     incrA,
   input [LEN_W-1:0]      length,

   input [31:0]           maximum,

   input                  disabled,

   input [31:0]          in0,

   (* versat_latency = 1 *) output reg [31:0] out0, // Signal indicating change

   input [31:0] delay0
);

assign ext_dp_out_0_port_0 = 0;
assign ext_dp_write_0_port_0 = 1'b0;

//reg [31:0] cycle;
wire [SIZE_W-1:0] currentValue;
reg pingPongState;

wire loadNext;
reg nextValue;

wire [ADDR_W-1:0] scannerAddress;

MemoryScanner #(.DATA_W(SIZE_W),.ADDR_W(ADDR_W)) scanner(
   .addr(scannerAddress),
   .dataIn(ext_dp_in_0_port_0),
   .enable(ext_dp_enable_0_port_0),
   
   .nextValue(nextValue),
   .currentValue(currentValue),

   .reset(run),

   .clk(clk),
   .rst(rst)
   );

assign ext_dp_addr_0_port_0 = {!pingPongState,scannerAddress[ADDR_W-2:0]};

// Ping pong 
always @(posedge clk,posedge rst)
begin
   if(rst)
      pingPongState <= 0;
   else if(run) // && !disabled
      pingPongState <= !pingPongState;
end

reg stillValid;
assign done = (!running || disabled || !stillValid) && !databus_valid_0 ;

always @(posedge clk,posedge rst) begin
   if(rst) begin
      stillValid <= 1'b0;
   end else if(run) begin
      stillValid <= 1'b1; 
   end else if(running) begin
      if(in0 + 1 >= maximum) begin
         stillValid <= 1'b0;
      end
   end
end

assign nextValue = (running && in0[SIZE_W-1:0] == currentValue);

reg [31:0] delay;
assign out0 = ((delay == 0 && !disabled && stillValid) ? {32{(in0[SIZE_W-1:0] == currentValue)}} : 0);

   always @(posedge clk,posedge rst) begin
       if(rst) begin
            delay <= 0;
       end else if(run) begin
            delay <= delay0;
       end else if(|delay) begin
            delay <= delay - 1;
       end
   end

   wire [ADDR_W-1:0] startA = 0;
   wire [31:0]       delayA = 0;
   wire [ADDR_W-1:0] addrA, addrA_int, addrA_int2;

   always @(posedge clk,posedge rst)
   begin
      if(rst)
         databus_addr_0 <= 0;
      else if(run)
         databus_addr_0 <= ext_addr;
   end

   assign databus_wdata_0 = 0;
   assign databus_wstrb_0 = 0;
   assign databus_len_0 = length;

   wire next;
   wire gen_ready;
   wire [ADDR_W-1:0] gen_addr;
   wire gen_done;

   MyAddressGen #(.ADDR_W(ADDR_W),.DATA_W(AXI_DATA_W),.PERIOD_W(ADDR_W)) addrgenA(
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
      .valid(),
      .ready(gen_ready),
      .addr(gen_addr),
      .done(gen_done)
      );

   wire write_en;
   wire [ADDR_W-1:0] write_addr;
   wire [AXI_DATA_W-1:0] write_data;
   
   reg gen_valid;
   always @(posedge clk,posedge rst) begin
      if(rst) begin
         gen_valid <= 1'b0;
      end else if(run) begin
         gen_valid <= 1'b1;
      end else if(gen_valid) begin //if(running) begin
         if(databus_valid_0 && databus_ready_0 && databus_last_0) begin
            gen_valid <= 1'b0;
         end
      end
   end

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

   assign ext_dp_addr_0_port_1 = {pingPongState,write_addr[ADDR_W-2:0]}; // This should increament by 4 more.
   assign ext_dp_enable_0_port_1 = write_en;
   assign ext_dp_write_0_port_1 = 1'b1;
   assign ext_dp_out_0_port_1 = write_data;

endmodule

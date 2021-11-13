`timescale 1ns / 1ps
`include "axi.vh"
`include "xversat.vh"
`include "xdefs.vh"

`include "versat_defs.vh"

module versat_instance #(
      parameter ADDR_W = `ADDR_W,
      parameter DATA_W = `DATA_W,
      parameter AXI_ADDR_W = `AXI_ADDR_W
   )
   (
   // Databus master interface
`ifdef IO
   input [`nIO-1:0]                m_databus_ready,
   output [`nIO-1:0]               m_databus_valid,
   output [`nIO*AXI_ADDR_W-1:0]   m_databus_addr,
   input [`nIO*`DATAPATH_W-1:0]    m_databus_rdata,
   output [`nIO*`DATAPATH_W-1:0]   m_databus_wdata,
   output [`nIO*`DATAPATH_W/8-1:0] m_databus_wstrb,
`endif
   // data/control interface
   input                           valid,
   input [ADDR_W-1:0]              addr,
   input [DATA_W/8-1:0]            wstrb,
   input [DATA_W-1:0]              wdata,
   output                          ready,
   output reg [DATA_W-1:0]         rdata,

   input                           clk,
   input                           rst
   );

wire wor_ready;

wire done;
reg run;
wire [31:0] unitRdataFinal;
reg [31:0] stateRead;

wire we = (|wstrb);
wire memoryMappedAddr = addr[`MAPPED_BIT];

// Versat registers and memory access
reg versat_ready;
reg [31:0] versat_rdata;

reg soft_reset;

wire rst_int = (rst | soft_reset);

// Interface does not use soft_rest
always @(posedge clk,posedge rst) // Care, rst because writing to soft reset register
   if(rst) begin
      versat_rdata <= 32'h0;
      versat_ready <= 1'b0;
      soft_reset <= 0;
   end else begin
      versat_ready <= 1'b0;

      if(valid) begin 
         // Config/State register access
         if(!memoryMappedAddr) begin
            versat_ready <= 1'b1;
            versat_rdata <= stateRead;
         end

         // Versat specific registers
         if(addr == 0) begin
            versat_ready <= 1'b1;
            if(we)
               soft_reset <= wdata[1];
            else
               versat_rdata <= {31'h0,done}; 
         end
      end
   end

always @(posedge clk,posedge rst_int)
begin
   if(rst_int) begin
      run <= 1'b0;
   end else begin
      run <= 1'b0;

      if(valid && we && addr == 0)
         run <= wdata[0];
   end
end

assign rdata = (versat_ready ? versat_rdata : unitRdataFinal);

assign ready = versat_ready | wor_ready;

reg [`CONFIG_W - 1:0] configdata;
wire [`STATE_W - 1:0] statedata;

wire [`NUMBER_UNITS - 1:0] unitDone;
reg [`MAPPED_UNITS - 1:0] memoryMappedEnable;
wire[`MAPPED_UNITS - 1:0] unitReady;
wire [31:0] unitRData[`MAPPED_UNITS - 1:0];

assign wor_ready = (|unitReady);
assign done = &unitDone;

`ifdef SHADOW_REGISTERS
reg [`CONFIG_W - 1:0] configdata_shadow;

always @(posedge clk,posedge rst_int)
begin
   if(rst_int)
      configdata <= {`CONFIG_W{1'b0}};
   else if(valid && we && addr == 0 && wdata[0])
      configdata <= configdata_shadow;
end
`endif


`timescale 1ns / 1ps
`include "xversat.vh"
`include "xdefs.vh"

`include "versat_defs.vh"

module versat_instance #(
      parameter ADDR_W = `ADDR_W,
      parameter DATA_W = `DATA_W
   )
   (
   // Databus master interface
`ifdef IO
   input [`nIO-1:0]                m_databus_ready,
   output [`nIO-1:0]               m_databus_valid,
   output [`nIO*`IO_ADDR_W-1:0]    m_databus_addr,
   input [`nIO*`DATAPATH_W-1:0]    m_databus_rdata,
   output [`nIO*`DATAPATH_W-1:0]   m_databus_wdata,
   output [`nIO*`DATAPATH_W/8-1:0] m_databus_wstrb,
`endif
   // data/control interface
   input                           valid,
   input [ADDR_W-1:0]              addr,
   input                           we,
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
         if(!addr[14]) begin
            versat_ready <= 1'b1;
            versat_rdata <= stateRead;
         end

         // Versat specific registers
         if(addr[15:0] == 16'hffff) begin
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

      if(valid && we && addr[15:0] == 16'hffff)
         run <= wdata[0];
   end
end

assign rdata = (versat_ready ? versat_rdata : unitRdataFinal);

assign ready = versat_ready | wor_ready;

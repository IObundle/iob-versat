`timescale 1ns / 1ps

`include "versat_defs.vh"

module versat_instance #(
      parameter ADDR_W = 32,
      parameter DATA_W = 32,
      parameter AXI_ADDR_W = 32,
      parameter AXI_DATA_W = @{databusDataSize},
      parameter LEN_W = 20,
      parameter DELAY_W = 10
   )
   (
   // Databus master interface
`ifdef VERSAT_IO
   input  [`nIO-1:0]                m_databus_ready,
   output [`nIO-1:0]                m_databus_valid,
   output [`nIO*AXI_ADDR_W-1:0]     m_databus_addr,
   input  [AXI_DATA_W-1:0]          m_databus_rdata,
   output [`nIO*AXI_DATA_W-1:0]     m_databus_wdata,
   output [`nIO*(AXI_DATA_W/8)-1:0] m_databus_wstrb,
   output [`nIO*LEN_W-1:0]          m_databus_len,
   input  [`nIO-1:0]                m_databus_last,
`endif

// data/control interface
   input                           csr_valid,
   input [ADDR_W-1:0]              csr_addr,
   input [DATA_W/8-1:0]            csr_wstrb,
   input [DATA_W-1:0]              csr_wdata,
   output                          csr_rvalid,
   output reg [DATA_W-1:0]         csr_rdata,

@{externalMemPorts}

@{inAndOuts}

   input                           clk,
   input                           rst
   );

@{wireDecls}

// This interface is the result of joining the csr or the dma interface
// Any logic that can interact with the DMA must use this interface.
wire data_valid;
wire [ADDR_W-1:0] data_address;
wire [DATA_W-1:0] data_data;
wire [(DATA_W/8)-1:0] data_wstrb;

reg running;

reg [DATA_W-1:0] stateRead;

wire we = (|csr_wstrb);
wire memoryMappedAddr = @{memoryConfigDecisionExpr};

// Versat registers and memory access
reg versat_rvalid;
reg [DATA_W-1:0] versat_rdata;

reg soft_reset,signal_loop; // Self resetting 

wire done = &unitDone;
wire canRun = done;
wire rst_int = (rst | soft_reset);

reg startRunPulse;
reg run;

wire pre_run_pulse = canRun && startRunPulse;

always @(posedge clk,posedge rst)
begin
   if(rst) begin
       run <= 0;
   end else if(pre_run_pulse) begin
       run <= 1'b1;
   end else begin
       run <= 1'b0;
   end
end

wire dma_running; // TODO: Emit this only if we have DMA enabled.

@{emitIO}

always @(posedge clk,posedge rst)
begin
   if(rst) begin
      running <= 0;
   end else if(run) begin
      running <= 1;
   end else if(running && done) begin
      running <= 0;
   end
end

@{dmaInstantiation}

@{profilingStuff}

// Control interface write portion
@{controlWriteInterface}

// Control interface read portion
@{controlReadInterface}

@{unitsMappedDecl}

@{connections}

@{configurationDecl}

@{stateDecl}

@{combOperations}

@{instanciateUnits}

@{assignOuts}

endmodule

`include "versat_undefs.vh"

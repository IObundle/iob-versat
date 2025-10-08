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
   input [ADDR_W-1:0]              csr_addr, // Use address in the code below, it uses byte addresses
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

wire wor_rvalid;

wire data_valid;
wire [ADDR_W-1:0] address;
wire [DATA_W-1:0] data_data;
wire [(DATA_W/8)-1:0] data_wstrb;

reg running;

wire done;
reg [30:0] runCounter;
reg [31:0] stateRead;

wire we = (|csr_wstrb);

wire memoryMappedAddr;

// Versat registers and memory access
reg versat_rvalid;
reg [31:0] versat_rdata;

reg soft_reset,signal_loop; // Self resetting 

wire canRun = |runCounter && (&unitDone);
reg canRun1;

wire rst_int = (rst | soft_reset);

always @(posedge clk,posedge rst)
begin
   if(rst) begin
       canRun1 <= 0;
   end else begin
       canRun1 <= canRun;
   end
end

wire run = (canRun && canRun1);
wire pre_run_pulse = (canRun && !canRun1); // One cycle before run is asserted

assign done = (!(|runCounter) && (&unitDone));

wire dma_running;

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

always @(posedge clk,posedge rst_int)
begin
   if(rst_int) begin
      runCounter <= 0;
   end else begin
      if(run)
         runCounter <= runCounter - 1;

      if(csr_valid && we) begin
         if(csr_addr == 0)
            runCounter <= runCounter + csr_wdata[15:0];
      end
   end
end

assign memoryMappedAddr = @{memoryConfigDecisionExpr};

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

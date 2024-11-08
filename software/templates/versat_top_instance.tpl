`timescale 1ns / 1ps

#{include "versat_common.tpl"}

#{set nDones #{call CountDones instances}}
#{set nOperations #{call CountOperations instances}}
#{set nCombOperations #{call CountCombOperations instances}}
#{set nSeqOperations  #{call CountSeqOperations instances}}

#{set databusCounter 0}

`include "versat_defs.vh"

module versat_instance #(
      parameter ADDR_W = 32,
      parameter DATA_W = 32,
      parameter AXI_ADDR_W = 32,
      parameter AXI_DATA_W = @{arch.databusDataSize},
      parameter LEN_W = 20
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
   input                           valid,
   input [ADDR_W-1:0]              addr, // Use address in the code below, it uses byte addresses
   input [DATA_W/8-1:0]            wstrb,
   input [DATA_W-1:0]              wdata,
   output                          rvalid,
   output reg [DATA_W-1:0]         rdata,

   #{for ext external}
      #{set i index}
      #{if ext.type}
   // DP
      #{for it 2}
	  #{set dp ext.dp[it]}
output [@{dp.bitSize}-1:0]  ext_dp_addr_@{i}_port_@{index}_o,
   output [@{dp.dataSizeOut}-1:0] ext_dp_out_@{i}_port_@{index}_o,
   input  [@{dp.dataSizeIn}-1:0] ext_dp_in_@{i}_port_@{index}_i,
   output                       ext_dp_enable_@{i}_port_@{index}_o,
   output                       ext_dp_write_@{i}_port_@{index}_o,
      #{end}
      #{else}
   // 2P
   output [@{ext.tp.bitSizeOut}-1:0]  ext_2p_addr_out_@{i}_o,
   output [@{ext.tp.bitSizeIn}-1:0]  ext_2p_addr_in_@{i}_o,
   output                       ext_2p_write_@{i}_o,
   output                       ext_2p_read_@{i}_o,
   input  [@{ext.tp.dataSizeIn}-1:0] ext_2p_data_in_@{i}_i,
   output [@{ext.tp.dataSizeOut}-1:0] ext_2p_data_out_@{i}_o,
      #{end}
   #{end}

   #{for i nInputs}
   input [DATA_W-1:0]              in@{i},
   #{end}

   #{for i nOutputs}
   output [DATA_W-1:0]             out@{i},
   #{end}

   input                           clk,
   input                           rst
   );

wire wor_rvalid;

wire done;
reg [30:0] runCounter;
reg [31:0] stateRead;
#{if unitsMapped}
wire [31:0] unitRdataFinal;
#{end}

wire we = (|wstrb);

wire memoryMappedAddr;

// Versat registers and memory access
reg versat_rvalid;
reg [31:0] versat_rdata;

reg soft_reset,signal_loop; // Self resetting 

wire rst_int = (rst | soft_reset);

#{if useDMA}
reg [1:0] dma_state;
#{end}

// Interface does not use soft_rest
always @(posedge clk,posedge rst) // Care, rst because writing to soft reset register
   if(rst) begin
      versat_rdata <= 32'h0;
      versat_rvalid <= 1'b0;
      signal_loop <= 1'b0;
      soft_reset <= 0;
   end else begin
      versat_rvalid <= 1'b0;
      soft_reset <= 1'b0;
      signal_loop <= 1'b0;

      if(valid) begin 
         // Config/State register access
         if(!memoryMappedAddr) begin
            if(wstrb == 0) versat_rvalid <= 1'b1;
            versat_rdata <= stateRead;
         end

         // Versat specific registers
         if(addr == 0) begin
            if(wstrb == 0) versat_rvalid <= 1'b1;
            if(we) begin
               soft_reset <= wdata[31];
               signal_loop <= wdata[30];    
            end else begin
               versat_rdata <= {31'h0,done}; 
            end
         end
#{if useDMA}
         if(addr == 4) begin
            if(wstrb == 0) versat_rvalid <= 1'b1;
            versat_rdata <= {31'h0,dma_state == 0};
         end
#{end}
      end
   end

#{if nIO}
wire databus_ready[@{nIO}];
wire databus_valid[@{nIO}];
wire [AXI_ADDR_W-1:0]   databus_addr[@{nIO}];
wire [AXI_DATA_W-1:0]   databus_rdata[@{nIO}];
wire [AXI_DATA_W-1:0]   databus_wdata[@{nIO}];
wire [AXI_DATA_W/8-1:0] databus_wstrb[@{nIO}];
wire [LEN_W-1:0]        databus_len[@{nIO}];
wire databus_last[@{nIO}];

#{set numberIOs nIO}
#{if useDMA}
#{set numberIOs (nIO - 1)}
#{end}
#{for i numberIOs}
assign databus_ready[@{databusCounter}] = m_databus_ready[@{i}];
assign m_databus_valid[@{databusCounter}] = databus_valid[@{i}];
assign m_databus_addr[(@{(databusCounter)} * AXI_ADDR_W) +: AXI_ADDR_W] = databus_addr[@{i}];
assign databus_rdata[@{databusCounter}] = m_databus_rdata;
assign m_databus_wdata[@{(databusCounter)} * AXI_DATA_W +: AXI_DATA_W] = databus_wdata[@{i}];
assign m_databus_wstrb[@{(databusCounter)} * (AXI_DATA_W/8) +: (AXI_DATA_W/8)] = databus_wstrb[@{i}];
assign m_databus_len[@{(databusCounter)} * LEN_W +: LEN_W] = databus_len[@{i}];
assign databus_last[@{databusCounter}] = m_databus_last[@{i}];
#{inc databusCounter}
#{end}
#{end}

#{if nDones}
wire [@{nDones - 1}:0] unitDone;
#{else}
wire unitDone = 1'b1;
#{end}

wire canRun = |runCounter && (&unitDone);
reg canRun1;

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

reg lastCycleDone;

always @(posedge clk)
begin
    lastCycleDone <= done;
end

reg running;

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

#{if useDMA}
reg [LEN_W-1:0] dma_length;
reg [AXI_ADDR_W-1:0] dma_addr_in;
reg [AXI_ADDR_W-1:0] dma_addr_read;

reg dma_valid;
reg [AXI_ADDR_W-1:0] dma_addr;
reg [DATA_W-1:0] dma_wdata;
reg [(DATA_W/8)-1:0] dma_wstrb;
reg [LEN_W-1:0] dma_len;

wire dma_ready;
wire [AXI_DATA_W-1:0] dma_rdata;
wire dma_last;

always @(posedge clk,posedge rst_int)
begin
   if(rst_int) begin
      dma_length <= 0;
      dma_addr_in <= 0;
      dma_addr_read <= 0;
      dma_state <= 0;
      runCounter <= 0;
   end else begin
      if(run)
         runCounter <= runCounter - 1;

      case(dma_state)
      2'b00: ;
      2'b01: begin
         if(dma_ready && dma_last) begin
            dma_state <= 0;
         end
         if(dma_ready) begin
            dma_addr_in <= dma_addr_in + 4;
         end
      end
      2'b10: begin
      end
      2'b11: begin
      end
      endcase

      if(valid && we) begin
         if(addr == 0)
            runCounter <= runCounter + wdata[15:0];
         if(addr == 4)
            dma_addr_in <= wdata;
         if(addr == 8)
            dma_addr_read <= wdata;
         if(addr == 12)
            dma_length <= wdata[LEN_W-1:0];
         if(addr == 16)
            dma_state <= 2'b01;
      end
   end
end

assign dma_ready = m_databus_ready[@{databusCounter}];
assign m_databus_valid[@{databusCounter}] = dma_valid;
assign m_databus_addr[@{databusCounter} * AXI_ADDR_W +: AXI_ADDR_W] = dma_addr;
assign dma_rdata = m_databus_rdata[AXI_DATA_W-1:0];
assign m_databus_wdata[@{databusCounter} * AXI_DATA_W +: AXI_DATA_W] = dma_wdata;
assign m_databus_wstrb[@{databusCounter} * (AXI_DATA_W/8) +: (AXI_DATA_W/8)] = dma_wstrb;
assign m_databus_len[@{databusCounter} * LEN_W +: LEN_W] = dma_len;
assign dma_last = m_databus_last[@{databusCounter}];
#{inc databusCounter}

always @*
begin
   dma_valid = 1'b0;
   dma_addr = dma_addr_read;
   dma_wdata = 0;
   dma_wstrb = 0; // Only read, for now
   dma_len = dma_length;

   case(dma_state)
   2'b00: ;
   2'b01: begin
      dma_valid = 1'b1;
   end
   2'b10: begin
   end
   2'b11: begin
   end
   endcase
end
#{else}
// No DMA
always @(posedge clk,posedge rst_int)
begin
   if(rst_int) begin
      runCounter <= 0;
   end else begin
      if(run)
         runCounter <= runCounter - 1;

      if(valid && we) begin
         if(addr == 0)
            runCounter <= runCounter + wdata[15:0];
      end
   end
end


#{end}

#{if useDMA}
wire data_valid = (dma_ready || valid);
wire data_write = (dma_ready || (valid && we && addr >= 20)); // Dma ready is always a write, dma cannot be used to read, for now
wire [ADDR_W-1:0] address = dma_ready ? dma_addr_in : addr;
wire [DATA_W-1:0] data_data = dma_ready ? dma_rdata : wdata;
wire [(DATA_W/8)-1:0] data_wstrb = dma_ready ? ~0 : wstrb;
#{else}
wire data_valid = valid;
wire data_write = valid && we;
wire [ADDR_W-1:0] address = addr;
wire [DATA_W-1:0] data_data = wdata;
wire [(DATA_W/8)-1:0] data_wstrb = wstrb;
#{end}

#{if memoryMappedBytes == 0}
assign memoryMappedAddr = 1'b0;
#{else}
assign memoryMappedAddr = address[@{memoryConfigDecisionBit}];
#{end}

reg [31:0] latency_counter;
reg enableCounter;
always @(posedge clk,posedge rst)
begin
   if(rst) begin
      latency_counter <= 0;
      enableCounter <= 0;
   end else if(run) begin
      latency_counter <= 0;
      enableCounter <= 1;
   end else if(done) begin
      enableCounter <= 0;
   end else if(enableCounter) begin
      latency_counter <= latency_counter + 1;
   end
end

#{if unitsMapped}
assign rdata = (versat_rvalid ? versat_rdata : unitRdataFinal);
#{else}
assign rdata = versat_rdata;
#{end}

#{if unitsMapped}
assign rvalid = versat_rvalid | wor_rvalid;
#{else}
assign rvalid = versat_rvalid;
#{end}

#{if unitsMapped}
reg [@{unitsMapped - 1}:0] memoryMappedEnable;
wire[@{unitsMapped - 1}:0] unitRValid;
assign wor_rvalid = (|unitRValid);
#{end}

#{if versatValues.numberConnections}
wire [31:0] #{join ", " node instances} #{set id index}
   #{join ", "  j node.outputs} #{if j} output_@{id}_@{index} #{end} #{end}
#{end};
#{end}

#{if unitsMapped}
wire [31:0] unitRData[@{unitsMapped - 1}:0];
assign unitRdataFinal = (#{join "|" i unitsMapped} unitRData[@{i}] #{end});
#{end}

// Memory mapped
#{if unitsMapped}
always @*
begin
   memoryMappedEnable = {@{unitsMapped}{1'b0}};
   if(data_valid & memoryMappedAddr)
   begin
   #{set counter 0}
   #{for node instances}
   #{set inst node}
   #{if inst.declaration.memoryMapBits}
      #{if memoryMasks[counter].memoryMaskSize}
      if(address[@{memoryAddressBits - 1}:@{memoryAddressBits - memoryMasks[counter].memoryMaskSize}] == @{memoryAddressBits - inst.declaration.memoryMapBits}'b@{memoryMasks[counter].memoryMask}) // @{memoryMappedBase * 4 + inst.memMapped * 4 |> Hex}
         memoryMappedEnable[@{counter}] = 1'b1;
      #{else}
      memoryMappedEnable[0] = 1'b1;
      #{end}
   #{inc counter}
   #{end}
   #{end}
   end
end
#{end}

#{if versatValues.configurationBits}
wire [@{versatValues.configurationBits-1}:0] configdata;

versat_configurations configs(
   .config_data_o(configdata),                 

   .memoryMappedAddr(memoryMappedAddr),
   .data_write(data_write),

   .address(address),
   .data_wstrb(data_wstrb),
   .data_data(data_data),

   .change_config_pulse(pre_run_pulse),

   .clk_i(clk),
   .rst_i(rst_int)
);
#{end}

#{if versatValues.stateBits} wire [@{versatValues.stateBits - 1}:0] statedata; #{end}

// State reading
always @*
begin
   stateRead = 32'h0;
   if(valid & !we & !memoryMappedAddr) begin // This if could be removed, stateRead could be updated every time, the if just adds unnecesary logic, but easier to debug, I guess
      #{set counter 0}
      #{set addr versatState}
      #{for node instances}
      #{set inst node}
      #{set decl inst.declaration}
      #{for wire decl.baseConfig.states}
      if(addr[@{versatValues.stateAddressBits + 1}:0] >= @{addr * 4} && addr[@{versatValues.stateAddressBits + 1}:0] < @{(addr + 1) * 4}) // @{addr * 4 |> Hex}
         stateRead = statedata[@{counter}+:@{wire.bitSize}];
      #{inc addr}
      #{set counter counter + wire.bitSize}
      #{end}
      #{end}
   end
end

#{if nCombOperations}
reg [31:0] #{join "," node instances} #{if node.declaration.isOperation and node.declaration.baseConfig.outputLatencies[0] == 0} comb_@{node |> Identify} #{end}#{end}; 

always @*
begin
#{for node ordered}
   #{set decl node.declaration}
   #{if decl.isOperation and decl.baseConfig.outputLatencies[0] == 0}
      #{set input1 node.inputs[0]}
      #{if decl.baseConfig.inputDelays.size == 1}
         #{format decl.operation "comb" @{node |> Identify} #{call retOutputName2 instances input1}};
      #{else}
         #{set input2 node.inputs[1]}
         #{format decl.operation "comb" @{node |> Identify} #{call retOutputName2 instances input1} #{call retOutputName2 instances input2}};
      #{end}
   #{end}
#{end}
end
#{end}

#{if nSeqOperations}
reg [31:0] #{join "," node instances} #{if node.declaration.isOperation and node.declaration.baseConfig.outputLatencies[0] != 0} seq_@{node |> Identify} #{end}#{end}; 

always @(posedge clk)
begin
#{for node instances}
   #{set decl node.declaration}
   #{if decl.isOperation and decl.baseConfig.outputLatencies[0] != 0 }
      #{set input1 node.inputs[0]}
      #{format decl.operation "seq" @{node |> Identify} #{call retOutputName2 instances input1}};
   #{end}
#{end}   
end
#{end}

#{set counter 0}
#{set configDataIndex 0}
#{set staticDataIndex staticStart}
#{set stateDataIndex 0}
#{set delaySeen 0}
#{set ioIndex 0}
#{set externalCounter 0}
#{set memoryMappedIndex 0}
#{set doneCounter 0}
#{for node instances}
#{set id index}
#{set inst node}
#{set decl inst.declaration}
   #{if (decl != inputDecl and decl != outputDecl and !decl.isOperation)}
      @{decl.name} @{parameters[index]} @{inst |> Identify}_@{counter} (
         #{for j node.outputs} #{if j}
            .out@{index}(output_@{id}_@{index}),
         #{else}
            .out@{index}(),
         #{end}
         #{end}

         #{for input node.inputs}
         #{if input.inst}
            .in@{index}(@{#{call retOutputName2 instances input}}),
         #{else}
            .in@{index}(0),
         #{end}
         #{end}

         #{for wire decl.baseConfig.configs}
         #{if decl.type}
            .@{wire.name}(configdata[@{configDataIndex}+:@{wire.bitSize}]),
         #{else}
            .@{wire.name}(configdata[@{configDataIndex}+:@{wire.bitSize}]),
         #{end}
         #{set configDataIndex configDataIndex + wire.bitSize}
         #{end}

         #{for unit decl.staticUnits}
         #{set id unit.first}
         #{for wire unit.second.configs}
            .@{id.parent.name}_@{id.name}_@{wire.name}(configdata[@{staticDataIndex}+:@{wire.bitSize}]),
         #{set staticDataIndex staticDataIndex + wire.bitSize}
         #{end}
         #{end}

         #{for i decl.baseConfig.delayOffsets.max}
            .delay@{i}(configdata[@{delayStart + (delaySeen * delaySize)}+:@{delaySize}]),
         #{inc delaySeen}
         #{end}

         #{for external decl.externalMemory}
            #{set i index}
            #{if external.type}
         // DP
            #{for port 2}
         .ext_dp_addr_@{i}_port_@{port}(ext_dp_addr_@{externalCounter}_port_@{port}_o),
         .ext_dp_out_@{i}_port_@{port}(ext_dp_out_@{externalCounter}_port_@{port}_o),
         .ext_dp_in_@{i}_port_@{port}(ext_dp_in_@{externalCounter}_port_@{port}_i),
         .ext_dp_enable_@{i}_port_@{port}(ext_dp_enable_@{externalCounter}_port_@{port}_o),
         .ext_dp_write_@{i}_port_@{port}(ext_dp_write_@{externalCounter}_port_@{port}_o),
            #{end}
            #{else}
         // 2P
         .ext_2p_addr_out_@{i}(ext_2p_addr_out_@{externalCounter}_o),
         .ext_2p_addr_in_@{i}(ext_2p_addr_in_@{externalCounter}_o),
         .ext_2p_write_@{i}(ext_2p_write_@{externalCounter}_o),
         .ext_2p_read_@{i}(ext_2p_read_@{externalCounter}_o),
         .ext_2p_data_in_@{i}(ext_2p_data_in_@{externalCounter}_i),
         .ext_2p_data_out_@{i}(ext_2p_data_out_@{externalCounter}_o),
            #{end}
         #{inc externalCounter}
         #{end}

         #{for wire decl.baseConfig.states}
         #{if decl.type}
            .@{wire.name}(statedata[@{stateDataIndex}+:@{wire.bitSize}]),
         #{else}
            .@{wire.name}(statedata[@{stateDataIndex}+:@{wire.bitSize}]),
         #{end}
         #{set stateDataIndex stateDataIndex + wire.bitSize}
         #{end}      

         #{if decl.memoryMapBits}
         .valid(memoryMappedEnable[@{memoryMappedIndex}]),
         .wstrb(data_wstrb),
         #{if decl.memoryMapBits}
         .addr(address[@{decl.memoryMapBits - 1}:0]),
         #{end}
         .rdata(unitRData[@{memoryMappedIndex}]),
         .rvalid(unitRValid[@{memoryMappedIndex}]),
         .wdata(data_data),
         #{inc memoryMappedIndex}
         #{end}

         #{for i decl.nIOs}
         .databus_ready_@{i}(databus_ready[@{ioIndex}]),
         .databus_valid_@{i}(databus_valid[@{ioIndex}]),
         .databus_addr_@{i}(databus_addr[@{ioIndex}]),
         .databus_rdata_@{i}(databus_rdata[@{ioIndex}]),
         .databus_wdata_@{i}(databus_wdata[@{ioIndex}]),
         .databus_wstrb_@{i}(databus_wstrb[@{ioIndex}]),
         .databus_len_@{i}(databus_len[@{ioIndex}]),
         .databus_last_@{i}(databus_last[@{ioIndex}]),
         #{set ioIndex ioIndex + 1}
         #{end}

         #{if decl.signalLoop}
         .signal_loop(signal_loop),
         #{end}

         .running(running),

         .run(run),
         #{if decl.implementsDone}
         .done(unitDone[@{doneCounter}]),
         #{inc doneCounter}
         #{end}
         .clk(clk),
         .rst(rst_int)
      );
   #{inc counter}
   #{end}
#{end}

#{for node instances}
#{set inst node}
#{set decl inst.declaration}
#{if decl == outputDecl}
   #{for input node.inputs}
   #{if input.inst}
   assign out@{index} = @{#{call retOutputName2 instances input}};
   #{end}
   #{end}
#{end}
#{end}

endmodule

`include "versat_undefs.vh"

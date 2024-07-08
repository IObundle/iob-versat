`timescale 1ns / 1ps

#{include "versat_common.tpl"}

#{set nDones #{call CountDones instances}}
#{set nOperations #{call CountOperations instances}}
#{set nCombOperations #{call CountCombOperations instances}}
#{set nSeqOperations  #{call CountSeqOperations instances}}

module @{accel.name} #(
      parameter ADDR_W = 32,
      parameter DATA_W = 32,
      parameter DELAY_W = @{delaySize},
      parameter AXI_ADDR_W = @{arch.addrSize},
      parameter AXI_DATA_W = @{arch.dataSize},
      parameter LEN_W = 20
   )
   (

   input run,
   input running,
   
   #{if nDones}
   output done,
   #{end}

   #{for i accel.baseConfig.inputDelays}
   input [DATA_W-1:0]              in@{index},
   #{end}

   #{for i accel.baseConfig.outputLatencies}
   output [DATA_W-1:0]             out@{index},
   #{end}

   #{for wire accel.baseConfig.configs}
   input [@{wire.bitSize-1}:0]     @{wire.name},
   #{end}

   #{for unit accel.staticUnits}
   #{set id unit.first}
   #{for wire unit.second.configs}
   input [@{wire.bitSize-1}:0]     @{id.parent.name}_@{id.name}_@{wire.name},
   #{end}
   #{end}

   #{for wire accel.baseConfig.states}
   output [@{wire.bitSize-1}:0]    @{wire.name},
   #{end}

   #{for i accel.baseConfig.delayOffsets.max}
   input  [DELAY_W-1:0]            delay@{i},
   #{end}

   #{for i accel.nIOs}
   // Databus master interface
   input                   databus_ready_@{i},
   output                  databus_valid_@{i},
   output [AXI_ADDR_W-1:0] databus_addr_@{i},
   input  [AXI_DATA_W-1:0] databus_rdata_@{i},
   output [AXI_DATA_W-1:0] databus_wdata_@{i},
   output [(AXI_DATA_W)/8 - 1:0] databus_wstrb_@{i},
   output [LEN_W-1:0]      databus_len_@{i},
   input                   databus_last_@{i},
   #{end}

   #{for ext accel.externalMemory}
#{set i index}
      #{if ext.type}
   // DP
      #{for dp ext.dp}
   output [@{dp.bitSize}-1:0]   ext_dp_addr_@{i}_port_@{index},
   output [@{dp.dataSizeOut}-1:0]  ext_dp_out_@{i}_port_@{index},
   input  [@{dp.dataSizeIn}-1:0]  ext_dp_in_@{i}_port_@{index},
   output                        ext_dp_enable_@{i}_port_@{index},
   output                        ext_dp_write_@{i}_port_@{index},
      #{end}
      #{else}
   // 2P
   output [@{ext.tp.bitSizeOut}-1:0]   ext_2p_addr_out_@{i},
   output [@{ext.tp.bitSizeIn}-1:0]   ext_2p_addr_in_@{i},
   output                        ext_2p_write_@{i},
   output                        ext_2p_read_@{i},
   input  [@{ext.tp.dataSizeIn}-1:0]  ext_2p_data_in_@{i},
   output [@{ext.tp.dataSizeOut}-1:0]  ext_2p_data_out_@{i},
      #{end}
   #{end}

   #{if accel.signalLoop}
   input signal_loop,
   #{end}

   #{if accel.memoryMapBits}
   // data/control interface
   input                           valid,
   input [@{accel.memoryMapBits - 1}:0] addr,
   input [DATA_W/8-1:0]            wstrb,
   input [DATA_W-1:0]              wdata,
   output                          rvalid,
   output [DATA_W-1:0]             rdata,
   #{end}

   input                           clk,
   input                           rst
   );

wire wor_rvalid;

wire [31:0] unitRdataFinal;
reg [31:0] stateRead;

#{if unitsMapped}
// Memory access
wire we = (|wstrb);
wire[@{unitsMapped - 1}:0] unitRValid;
reg [@{unitsMapped - 1}:0] memoryMappedEnable;
wire [31:0] unitRData[@{unitsMapped - 1}:0];

assign rdata = unitRdataFinal;
assign rvalid = wor_rvalid;
assign wor_rvalid = (|unitRValid);

wire [31:0] #{join ", " i unitsMapped} rdata_@{i} #{end};

assign unitRdataFinal = (#{join "|" i unitsMapped} unitRData[@{i}] #{end});
#{end}

#{if nDones > 0}
wire [@{nDones - 1}:0] unitDone;
assign done = &unitDone;
#{end}

#{if numberConnections}
wire [31:0] #{join ", " node instances} #{let id index} #{join ", " j node.outputs}#{if j} output_@{id}_@{index} #{end}#{end}
#{end};
#{end}

#{if unitsMapped}
// Memory mapped
always @*
begin
   memoryMappedEnable = {@{unitsMapped}{1'b0}};
   if(valid)
   begin
   #{set counter 0}
   #{for node instances}
   #{set inst node}
   #{if inst.declaration.memoryMapBits}
      #{if memoryMasks[counter].memoryMaskSize}
      if(addr[@{accel.memoryMapBits - 1}-:@{memoryMasks[counter].memoryMaskSize}] == @{memoryMasks[counter].memoryMaskSize}'b@{memoryMasks[counter].memoryMask})
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

#{if nCombOperations}
reg [31:0] #{join "," node instances}#{if node.declaration.isOperation and node.declaration.baseConfig.outputLatencies[0] == 0}comb_@{node |> Identify}#{end}#{end}; 

always @*
begin
#{for node instances}
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

#{let counter 0}
#{let ioIndex 0}
#{let memoryMappedIndex 0}
#{let externalCounter 0}
#{let delaySeen 0}
#{let statesSeen 0}
#{let doneCounter 0}
#{for node instances}
#{let id index}
#{let inst node}
#{let decl inst.declaration}
#{if (decl != inputDecl and decl != outputDecl and !decl.isOperation)}
      @{decl.name} @{inst.parameters} @{inst |> Identify}_@{counter} (
  #{for j node.outputs}
    #{if j}
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

  #{if inst.isStatic}
    #{for wire inst.declaration.baseConfig.configs}
         .@{wire.name}(@{accel.name}_@{inst |> Identify}_@{wire.name}),
    #{end}
  #{else}
    #{set configStart accel.baseConfig.configOffsets.offsets[id]}
    #{if configStart >= 0}
      #{for wire decl.baseConfig.configs}
         .@{wire.name}(@{accel.baseConfig.configs[configStart + index].name}), // @{configStart + index}
      #{end}
    #{end}
  #{end}

  #{for unit decl.staticUnits}         
    #{set id unit.first}
    #{for wire unit.second.configs}
         .@{id.parent.name}_@{id.name}_@{wire.name}(@{id.parent.name}_@{id.name}_@{wire.name}),
    #{end}
  #{end}

  #{for i decl.baseConfig.delayOffsets.max}
            .delay@{i}(delay@{delaySeen}),
    #{inc delaySeen}
  #{end}
   
  #{for external decl.externalMemory}
    #{set i index}
    #{if external.type}
         // DP
      #{for port 2}
         .ext_dp_addr_@{i}_port_@{port}(ext_dp_addr_@{externalCounter}_port_@{port}),
         .ext_dp_out_@{i}_port_@{port}(ext_dp_out_@{externalCounter}_port_@{port}),
         .ext_dp_in_@{i}_port_@{port}(ext_dp_in_@{externalCounter}_port_@{port}),
         .ext_dp_enable_@{i}_port_@{port}(ext_dp_enable_@{externalCounter}_port_@{port}),
         .ext_dp_write_@{i}_port_@{port}(ext_dp_write_@{externalCounter}_port_@{port}),
      #{end}
    #{else}
         // 2P
         .ext_2p_addr_out_@{i}(ext_2p_addr_out_@{externalCounter}),
         .ext_2p_addr_in_@{i}(ext_2p_addr_in_@{externalCounter}),
         .ext_2p_write_@{i}(ext_2p_write_@{externalCounter}),
         .ext_2p_read_@{i}(ext_2p_read_@{externalCounter}),
         .ext_2p_data_in_@{i}(ext_2p_data_in_@{externalCounter}),
         .ext_2p_data_out_@{i}(ext_2p_data_out_@{externalCounter}),
    #{end}
    #{inc externalCounter}
  #{end}

  #{for wire decl.baseConfig.states}
            .@{wire.name}(@{accel.baseConfig.states[statesSeen].name}),
    #{inc statesSeen}
  #{end}

  #{if decl.memoryMapBits}
         .valid(memoryMappedEnable[@{memoryMappedIndex}]),
         .wstrb(wstrb),
    #{if decl.memoryMapBits}
         .addr(addr[@{decl.memoryMapBits - 1}:0]),
    #{end}
         .rdata(unitRData[@{memoryMappedIndex}]),
         .rvalid(unitRValid[@{memoryMappedIndex}]),
         .wdata(wdata),
    #{inc memoryMappedIndex}
  #{end}

  #{for i decl.nIOs}
         .databus_ready_@{i}(databus_ready_@{ioIndex}),
         .databus_valid_@{i}(databus_valid_@{ioIndex}),
         .databus_addr_@{i}(databus_addr_@{ioIndex}),
         .databus_rdata_@{i}(databus_rdata_@{ioIndex}),
         .databus_wdata_@{i}(databus_wdata_@{ioIndex}),
         .databus_wstrb_@{i}(databus_wstrb_@{ioIndex}),
         .databus_len_@{i}(databus_len_@{ioIndex}),
         .databus_last_@{i}(databus_last_@{ioIndex}),
    #{inc ioIndex}
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
         .rst(rst)
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


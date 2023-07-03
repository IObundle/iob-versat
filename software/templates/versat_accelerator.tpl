`timescale 1ns / 1ps

#{include "versat_common.tpl"}

#{set nDones #{call CountDones instances}}
#{set nOperations #{call CountOperations instances}}
#{set nCombOperations #{call CountCombOperations instances}}
#{set nSeqOperations  #{call CountSeqOperations instances}}

module @{accel.name} #(
      parameter ADDR_W = 32,
      parameter DATA_W = 32,
      parameter AXI_ADDR_W = 32
   )
   (

   input run,
   input running,
   
   #{if nDones}
   output done,
   #{end}

   #{for i accel.inputDelays}
   input [DATA_W-1:0]              in@{index},
   #{end}

   #{for i accel.outputLatencies}
   output [DATA_W-1:0]             out@{index},
   #{end}

   #{for wire accel.configs}
   input [@{wire.bitsize-1}:0]     @{wire.name},
   #{end}

   #{for unit accel.staticUnits}
   #{set id unit.first}
   #{for wire unit.second.configs}
   input [@{wire.bitsize-1}:0]     @{id.parent.name}_@{id.name}_@{wire.name},
   #{end}
   #{end}

   #{for wire accel.states}
   output [@{wire.bitsize-1}:0]    @{wire.name},
   #{end}

   #{for i accel.delayOffsets.max}
   input  [31:0]                   delay@{i},
   #{end}

   #{for i accel.nIOs}
   // Databus master interface
   input                   databus_ready_@{i},
   output                  databus_valid_@{i},
   output [AXI_ADDR_W-1:0] databus_addr_@{i},
   input [31:0]            databus_rdata_@{i},
   output [31:0]           databus_wdata_@{i},
   output [32/8 - 1:0]     databus_wstrb_@{i},
   output [7:0]            databus_len_@{i},
   input                   databus_last_@{i},
   #{end}

   #{for ext accel.externalMemory}
      #{set i index}
      #{if ext.type}
   // DP
      #{for port 2}
   output [@{ext.bitsize}-1:0]   ext_dp_addr_@{i}_port_@{port},
   output [@{ext.datasize}-1:0]   ext_dp_out_@{i}_port_@{port},
   input  [@{ext.datasize}-1:0]   ext_dp_in_@{i}_port_@{port},
   output                ext_dp_enable_@{i}_port_@{port},
   output                ext_dp_write_@{i}_port_@{port},
      #{end}
      #{else}
   // 2P
   output [@{ext.bitsize}-1:0]   ext_2p_addr_out_@{i},
   output [@{ext.bitsize}-1:0]   ext_2p_addr_in_@{i},
   output                ext_2p_write_@{i},
   output                ext_2p_read_@{i},
   input  [@{ext.datasize}-1:0]   ext_2p_data_in_@{i},
   output [@{ext.datasize}-1:0]   ext_2p_data_out_@{i},
      #{end}
   #{end}

   #{if accel.signalLoop}
   input signal_loop,
   #{end}

   #{if accel.isMemoryMapped}
   // data/control interface
   input                           valid,
   #{if accel.memoryMapBits}
   input [@{accel.memoryMapBits - 1}:0] addr,
   #{end}

   input [DATA_W/8-1:0]            wstrb,
   input [DATA_W-1:0]              wdata,
   output                          ready,
   output [DATA_W-1:0]             rdata,
   #{end}

   input                           clk,
   input                           rst
   );

wire wor_ready;

wire [31:0] unitRdataFinal;
reg [31:0] stateRead;

#{if unitsMapped}
// Memory access
wire we = (|wstrb);
wire[@{unitsMapped - 1}:0] unitReady;
reg [@{unitsMapped - 1}:0] memoryMappedEnable;
wire [31:0] unitRData[@{unitsMapped - 1}:0];

assign rdata = unitRdataFinal;
assign ready = wor_ready;
assign wor_ready = (|unitReady);

wire [31:0] #{join ", " for i unitsMapped} rdata_@{i} #{end};

assign unitRdataFinal = (#{join "|" for i unitsMapped} unitRData[@{i}] #{end});
#{end}

#{if nDones > 0}
wire [@{nDones - 1}:0] unitDone;
assign done = &unitDone;
#{end}

#{if versatValues.numberConnections}
wire [31:0] #{join ", " for node instances}
   #{join ", " for j node.outputs} #{if j} output_@{node.inst.id}_@{index} #{end} #{end}
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
   #{set inst node.inst}
   #{if inst.declaration.isMemoryMapped}
      #{if versatData[counter].memoryMaskSize}
      if(addr[@{memoryAddressBits - 1}:@{memoryAddressBits - versatData[counter].memoryMaskSize}] == @{memoryAddressBits - inst.declaration.memoryMapBits}'b@{versatData[counter].memoryMask})
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
reg [31:0] #{join "," for node instances} #{if node.inst.declaration.isOperation and node.inst.declaration.outputLatencies[0] == 0} comb_@{node.inst.name |> Identify} #{end}#{end}; 

always @*
begin
#{for node ordered}
   #{set decl node.inst.declaration}
   #{if decl.isOperation and decl.outputLatencies[0] == 0}
      #{set input1 node.inputs[0]}
      #{if decl.inputDelays.size == 1}
         #{format decl.operation "comb" @{node.inst.name |> Identify} #{call retOutputName input1}};
      #{else}
         #{set input2 node.inputs[1]}
         #{format decl.operation "comb" @{node.inst.name |> Identify} #{call retOutputName input1} #{call retOutputName input2}};
      #{end}
   #{end}
#{end}
end
#{end}

#{if nSeqOperations}
reg [31:0] #{join "," for node instances} #{if node.inst.declaration.isOperation and node.inst.declaration.outputLatencies[0] != 0} seq_@{node.inst.name |> Identify} #{end}#{end}; 

always @(posedge clk)
begin
#{for node instances}
   #{set decl node.inst.declaration}
   #{if decl.isOperation and decl.outputLatencies[0] != 0 }
      #{set input1 node.inputs[0]}
      #{format decl.operation "seq" @{node.inst.name |> Identify} #{call retOutputName input1}};
   #{end}
#{end}   
end
#{end}

#{set counter 0}
#{set ioIndex 0}
#{set memoryMappedIndex 0}
#{set externalCounter 0}
#{set delaySeen 0}
#{set statesSeen 0}
#{set doneCounter 0}
#{for node instances}
#{set temp index}
#{set inst node.inst}
#{set decl inst.declaration}
   #{if (decl != inputDecl and decl != outputDecl and !decl.isOperation)}
      @{decl.name} @{inst.parameters} @{inst.name |> Identify}_@{counter} (
         #{for j node.outputs} #{if j}
            .out@{index}(output_@{inst.id}_@{index}),
         #{else}
            .out@{index}(),
         #{end}
         #{end}

         #{for input node.inputs}
            .in@{index}(#{call outputName input}),
         #{end}

         #{if inst.isStatic}
         #{for wire inst.declaration.configs}
         .@{wire.name}(@{accel.name}_@{inst.name |> Identify}_@{wire.name}),
         #{end}

         #{else}
         #{set configStart accel.configOffsets.offsets[temp]}
         #{for wire decl.configs}
         .@{wire.name}(@{accel.configs[configStart + index].name}), // @{configStart + index}
         #{end}
         #{for unit decl.staticUnits}         
         #{set id unit.first}
         #{for wire unit.second.configs}
         .@{id.parent.name}_@{id.name}_@{wire.name}(@{id.parent.name}_@{id.name}_@{wire.name}),
         #{end}
         #{end}
         #{end}

         #{for i decl.delayOffsets.max}
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

         #{for wire decl.states}
            .@{wire.name}(@{accel.states[statesSeen].name}),
         #{inc statesSeen}
         #{end}

         #{if decl.isMemoryMapped}
         .valid(memoryMappedEnable[@{memoryMappedIndex}]),
         .wstrb(wstrb),
         #{if decl.memoryMapBits}
         .addr(addr[@{decl.memoryMapBits - 1}:0]),
         #{end}
         .rdata(unitRData[@{memoryMappedIndex}]),
         .ready(unitReady[@{memoryMappedIndex}]),
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
         #{set ioIndex ioIndex + decl.nIOs}
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
#{set inst node.inst}
#{set decl inst.declaration}
#{if decl == outputDecl}
   #{for input node.inputs}
   #{if input.node}
   assign out@{index} = #{call outputName input};
   #{end}
   #{end}
#{end}
#{end}

endmodule


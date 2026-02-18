`timescale 1ns / 1ps

module @{accel.name} #(
      parameter DELAY_W = 32,
      parameter DATA_W = 32,
      parameter ADDR_W = 32,
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
   input [@{wire.bitSize-1}:0]     @{wire.name},
   #{end}

   #{for unit accel.staticUnits}
   #{set id unit.first}
   #{for wire unit.second.configs}
   (* versat_static *) input [@{wire.bitSize-1}:0]     @{id.parent.name}_@{id.name}_@{wire.name},
   #{end}
   #{end}

   #{for wire accel.states}
   output [@{wire.bitSize-1}:0]    @{wire.name},
   #{end}

   #{for i accel.numberDelays}
   input  [31:0]                   delay@{i},
   #{end}

   #{if accel.nIOs}
   // Databus master interface
   input [@{accel.nIOs - 1}:0]                databus_ready,
   output [@{accel.nIOs - 1}:0]               databus_valid,
   output [@{accel.nIOs} * AXI_ADDR_W-1:0]    databus_addr,
   input [`DATAPATH_W-1:0]                    databus_rdata,
   output [@{accel.nIOs} * `DATAPATH_W-1:0]   databus_wdata,
   output [@{accel.nIOs} * `DATAPATH_W/8-1:0] databus_wstrb,
   output [@{accel.nIOs} * 8-1:0]             databus_len,
   input  [@{accel.nIOs - 1}:0]               databus_last,
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

   #{if accel.memoryMapBits}
   // data/control interface
   input                           valid,
   #{if accel.memoryMapBits}
   input [@{accel.memoryMapBits - 1}:0] addr,
   #{end}

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

wire [31:0] #{join ", " node instances} #{let id index}
   #{join ", " j node.outputs} #{if j} output_@{id}_@{index} #{end} #{end}
#{end};

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
      if(addr[@{memoryAddressBits - 1}:@{memoryAddressBits - memoryMasks[counter].memoryMaskSize}] == @{memoryAddressBits - inst.declaration.memoryMapBits}'b@{memoryMasks[counter].memoryMask})
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
reg [31:0] #{join "," node instances} #{if node.declaration.isOperation and node.declaration.outputLatencies[0] == 0} comb_@{node |> Identify} #{end}#{end}; 

always @*
begin
#{for node ordered}
   #{set decl node.declaration}
   #{if decl.isOperation and decl.outputLatencies[0] == 0}
      #{set input1 node.inputs[0]}
      #{if decl.inputDelays.size == 1}
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
reg [31:0] #{join "," node instances} #{if node.declaration.isOperation and node.declaration.outputLatencies[0] != 0} seq_@{node |> Identify} #{end}#{end}; 

always @(posedge clk)
begin
#{for node instances}
   #{set decl node.declaration}
   #{if decl.isOperation and decl.outputLatencies[0] != 0 }
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

         #{if inst.name == "Merge0"}
         .stride(@{special}), // TODO: This is being hardcoded for now. 

         #{else}
         
         #{if inst.isStatic}
         #{for wire decl.configs}
         .@{wire.name}(@{accel.name}_@{inst |> Identify}_@{wire.name}),
         #{end}

         #{else}
         #{set configStart topLevel[id].configPos.val}
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

         #{end}

         #{for i decl.numberDelays}
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

         #{if decl.nIOs}
         .databus_ready(databus_ready[@{ioIndex} +: @{decl.nIOs}]),
         .databus_valid(databus_valid[@{ioIndex} +: @{decl.nIOs}]),
         .databus_addr(databus_addr[@{ioIndex * 32} +: @{32 * decl.nIOs}]),
         .databus_rdata(databus_rdata),
         .databus_wdata(databus_wdata[@{ioIndex * 32} +: @{32 * decl.nIOs}]),
         .databus_wstrb(databus_wstrb[@{ioIndex * 4} +: @{4 * decl.nIOs}]),
         .databus_len(databus_len[@{ioIndex * 8} +: @{8 * decl.nIOs}]),
         .databus_last(databus_last[@{ioIndex} +: @{decl.nIOs}]),
         #{set ioIndex ioIndex + decl.nIOs}
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

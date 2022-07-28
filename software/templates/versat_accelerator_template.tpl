`timescale 1ns / 1ps
`include "system.vh"
`include "axi.vh"
`include "xversat.vh"
`include "xdefs.vh"

#{include "versat_common.tpl"}

#{call CountDones instances}

module @{accel.name.str} #(
      parameter ADDR_W = `ADDR_W,
      parameter DATA_W = `DATA_W,
      parameter AXI_ADDR_W = 32
   )
   (

   input run,
   
   #{if nDones}
   output done,
   #{end}

   #{for i accel.nInputs}
   input [DATA_W-1:0]              in@{i},
   #{end}

   #{for i accel.nOutputs}
   output [DATA_W-1:0]             out@{i},
   #{end}

   #{for i accel.nConfigs}
   #{set wire accel.configWires[i]}
   input [@{wire.bitsize-1}:0]     @{wire.name},
   #{end}

   #{for unit accel.staticUnits}
   #{for i unit.nConfigs}
   #{set wire unit.wires[i]}
   input [@{wire.bitsize-1}:0]     @{unit.module.name.str}_@{unit.name}_@{wire.name},
   #{end}
   #{end}

   #{for i accel.nStates}
   #{set wire accel.stateWires[i]}
   output [@{wire.bitsize-1}:0]    @{wire.name},
   #{end}

   #{for i accel.nDelays}
   input  [31:0]                   delay@{i},
   #{end}

   #{if accel.nIOs}
   // Databus master interface
   input [@{accel.nIOs - 1}:0]                    databus_ready,
   output [@{accel.nIOs - 1}:0]                   databus_valid,
   output [@{accel.nIOs} * AXI_ADDR_W-1:0]    databus_addr,
   input [@{accel.nIOs} * `DATAPATH_W-1:0]    databus_rdata,
   output [@{accel.nIOs} * `DATAPATH_W-1:0]   databus_wdata,
   output [@{accel.nIOs} * `DATAPATH_W/8-1:0] databus_wstrb,
   #{end}

   #{if accel.isMemoryMapped}
   // data/control interface
   input                           valid,
   #{if accel.memoryMapBits}
   input [@{accel.memoryMapBits - 1}:0] addr,
   #{else}
   input                                addr, // Shouldnt need but otherwise verilator would complain
   #{end}

   input [DATA_W/8-1:0]            wstrb,
   input [DATA_W-1:0]              wdata,
   output                          ready,
   output reg [DATA_W-1:0]         rdata,
   #{end}

   input                           clk,
   input                           rst
   );

wire wor_ready;

wire [31:0] unitRdataFinal;
reg [31:0] stateRead;

// Memory access
#{if unitsMapped}
wire we = (|wstrb);
assign rdata = unitRdataFinal;
assign ready = wor_ready;
assign wor_ready = (|unitReady);
reg [@{unitsMapped - 1}:0] memoryMappedEnable;
wire[@{unitsMapped - 1}:0] unitReady;
wire [31:0] unitRData[@{unitsMapped - 1}:0];

wire [31:0] #{join ", " for i unitsMapped} rdata_@{i} #{end};

assign unitRdataFinal = (#{join "|" for i unitsMapped} unitRData[@{i}] #{end});
#{end}

#{if nDones > 0}
wire [@{nDones - 1}:0] unitDone;
assign done = &unitDone;
#{end}

wire [31:0] #{join ", " for inst instances}
   #{if inst.tempData.outputPortsUsed} 
      #{join ", " for j inst.tempData.outputPortsUsed} output_@{inst.id}_@{j} #{end}
   #{else}
      unused_@{inst.id} #{end}
#{end};

// Memory mapped
#{if unitsMapped}
always @*
begin
   memoryMappedEnable = {@{unitsMapped}{1'b0}};
   if(valid)
   begin
   #{set counter 0}
   #{for inst instances}
   #{if inst.declaration.isMemoryMapped}
      #{if inst.versatData.memoryMaskSize}
         if(addr[@{accel.memoryMapBits - 1}:@{accel.memoryMapBits - inst.versatData.memoryMaskSize}] == @{inst.versatData.memoryMaskSize}'b@{inst.versatData.memoryMask})
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

#{set counter 0}
reg [31:0] #{join "," for inst instances} #{if inst.declaration.latencies[0]} unused@{counter} #{inc counter} #{else} comb_@{inst.name.str} #{end}#{end}; 

always @*
begin
#{for inst instances}
#{set decl inst.declaration}
   #{if decl.isOperation}
      #{set input1 inst.tempData[0].inputs[0].inst}
      #{if decl.nInputs == 1}
         comb_@{inst.name.str} = @{decl.operation} #{call outputName input1};
      #{else}
         #{set input2 inst.tempData[0].inputs[1].inst}
         #{if decl.name.str == "RHR"}
            comb_@{inst.name.str} = (#{call outputName input1} >> #{call outputName input2}) | (#{call outputName input1} << (32 - #{call outputName input2}));
         #{else} 
            #{if decl.name.str == "RHL"}
               comb_@{inst.name.str} = (#{call outputName input1} << #{call outputName input2}) | (#{call outputName input1} >> (32 - #{call outputName input2}));
            #{else}
               comb_@{inst.name.str} = #{call outputName input1} @{decl.operation} #{call outputName input2};
            #{end}
         #{end}
      #{end}
   #{end}
#{end}
end

#{set counter 0}
#{set configDataIndex 0}
#{set stateDataIndex 0}
#{set ioIndex 0}
#{set memoryMappedIndex 0}
#{set inputSeen 0}
#{set configsSeen 0}
#{set delaySeen 0}
#{set statesSeen 0}
#{set doneCounter 0}
#{for inst instances}
#{set decl inst.declaration}
   #{if decl.name.str == "CircuitInput"}
   #{else}
   #{if decl.name.str == "CircuitOutput"}
   #{else}
      #{if !decl.isOperation}
      @{decl.name.str} @{decl.name.str}_@{counter} (
         #{for i inst.tempData.outputPortsUsed}
            .out@{i}(output_@{inst.id}_@{i}),
         #{end}

         #{for i inst.tempData.inputPortsUsed}
         #{if inst.tempData.inputs[i].inst.inst.declaration.type == 2}
            .in@{inst.tempData.inputs[i].port}(in@{inst.tempData.inputs[i].inst.inst.id}), // @{inst.tempData.inputs[i].inst.inst.name.str}
         #{else}
            .in@{inst.tempData.inputs[i].port}(#{call outputName inst.tempData.inputs[i].inst}),
         #{end}
         #{end}

         #{if inst.isStatic}
         #{for i inst.declaration.nConfigs}
         #{set wire inst.declaration.configWires[i]}
            .@{wire.name}(@{accel.name.str}_@{inst.name.str}_@{wire.name}), // Static
         #{end}

         #{else}
         
         #{for i decl.nConfigs}
         #{set wire decl.configWires[i]}
            .@{wire.name}(@{accel.configWires[configsSeen].name}), // Config
         #{inc configsSeen}
         #{end}
         #{for unit decl.staticUnits}
         #{for i unit.nConfigs}
         #{set wire unit.wires[i]}
            .@{unit.module.name.str}_@{unit.name}_@{wire.name}(@{unit.module.name.str}_@{unit.name}_@{wire.name}), // Static
         #{end}
         #{end}
         #{end}

         #{for i decl.nDelays}
            .delay@{i}(delay@{delaySeen}),
         #{inc delaySeen}
         #{end}

         #{for i decl.nStates}
         #{set wire decl.stateWires[i]}
            .@{wire.name}(@{accel.stateWires[statesSeen].name}),
         #{inc statesSeen}
         #{end}      

         #{if decl.isMemoryMapped}
         .valid(memoryMappedEnable[@{memoryMappedIndex}]),
         .wstrb(wstrb),
         #{if decl.memoryMapBits}
         .addr(addr[@{decl.memoryMapBits - 1}:0]),
         #{else}
         .addr(1'b0), // Shouldnt need but otherwise verilator would complain
         #{end}
         .rdata(unitRData[@{memoryMappedIndex}]),
         .ready(unitReady[@{memoryMappedIndex}]),
         .wdata(wdata),
         #{inc memoryMappedIndex}
         #{end}

         #{if decl.nIOs}
         .databus_ready(databus_ready[@{ioIndex} +: @{decl.nIOs}]),
         .databus_valid(databus_valid[@{ioIndex} +: @{decl.nIOs}]),
         .databus_addr(databus_addr[@{ioIndex * 32} +: @{32 * decl.nIOs}]),
         .databus_rdata(databus_rdata[@{ioIndex * 32} +: @{32 * decl.nIOs}]),
         .databus_wdata(databus_wdata[@{ioIndex * 32} +: @{32 * decl.nIOs}]),
         .databus_wstrb(databus_wstrb[@{ioIndex * 4} +: @{4 * decl.nIOs}]),
         #{set ioIndex ioIndex + decl.nIOs}
         #{end} 
         
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
   #{end}
#{end}

#{for inst instances}
#{set decl inst.declaration}
#{if decl.name.str == "CircuitOutput"}
   #{for i inst.tempData.numberInputs}
   #{set in inst.tempData.inputs[i].inst}
   assign out@{i} = #{call outputName in};
   #{end}
#{end}
#{end}

endmodule

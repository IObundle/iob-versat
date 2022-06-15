`timescale 1ns / 1ps
`include "system.vh"
`include "axi.vh"
`include "xversat.vh"
`include "xdefs.vh"

module @{accel.name.str} #(
      parameter ADDR_W = `ADDR_W,
      parameter DATA_W = `DATA_W,
      parameter AXI_ADDR_W = `AXI_ADDR_W
   )
   (

   input run,
   output done,

   #{for i accel.nInputs}
   (* latency=2 *) input [DATA_W-1:0]              in@{i},
   #{end}

   #{for i accel.nOutputs}
   (* latency=2 *) output [DATA_W-1:0]             out@{i},
   #{end}

   #{for i accel.nConfigs}
   #{set wire accel.configWires[i]}
   input [@{wire.bitsize-1}:0]     @{wire.name},
   #{end}

   #{for i accel.nStates}
   #{set wire accel.stateWires[i]}
   output [@{wire.bitsize-1}:0]    @{wire.name},
   #{end}

   #{for i accel.nDelays}
   input [31:0]                    delay@{i},
   #{end}

   #{if accel.nIOs}
   // Databus master interface
   input [@{accel.nIOs - 1}:0]                databus_ready,
   output [@{accel.nIOs - 1}:0]               databus_valid,
   output [@{accel.nIOs} * AXI_ADDR_W-1:0]    databus_addr,
   input [@{accel.nIOs} * `DATAPATH_W-1:0]    databus_rdata,
   output [@{accel.nIOs} * `DATAPATH_W-1:0]   databus_wdata,
   output [@{accel.nIOs} * `DATAPATH_W/8-1:0] databus_wstrb,
   #{end}

   #{if accel.memoryMapDWords}
   // data/control interface
   input                           valid,
   input [ADDR_W-1:0]              addr,
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

wire [@{numberUnits - 1}:0] unitDone;

assign done = &unitDone;

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
   #{for i unitsMapped}
      if(addr[@{memoryMappedUnitAddressRangeHigh}:@{memoryMappedUnitAddressRangeLow}] == @{i})
         memoryMappedEnable[@{i}] = 1'b1;
   #{end}
   end
end
#{end}

#{set counter 0}
#{set configDataIndex 0}
#{set stateDataIndex 0}
#{set ioIndex 0}
#{set memoryMappedIndex 0}
#{set inputSeen 0}
#{set delaySeen 0}
#{for inst instances}
#{set decl inst.declaration}
   #{if decl.name.str == "circuitInput"}
   #{else}
   #{if decl.name.str == "circuitOutput"}
   #{else}
   @{decl.name.str} @{decl.name.str}_@{counter} (
      #{for i inst.tempData.outputPortsUsed}
         .out@{i}(output_@{inst.id}_@{i}),
      #{end}

      #{for i inst.tempData.inputPortsUsed}
      #{if inst.tempData.inputs[i].inst.inst.declaration.type == 2}
         .in@{i}(in@{inst.tempData.inputs[i].inst.port}),
      #{else}
         .in@{i}(output_@{inst.tempData.inputs[i].inst.inst.id}_@{inst.tempData.inputs[i].inst.port}),
      #{end}
      #{end}

      #{for i decl.nConfigs}
      #{set wire decl.configWires[i]}
         .@{wire.name}(@{accel.configWires[configsSeen].name}),
      #{inc configsSeen}
      #{end}

      #{for i decl.nDelays}
         .delay@{i}(delay@{delaySeen}),
      #{inc delaySeen}
      #{end}

      #{for i decl.nStates}
      #{set wire decl.stateWires[i]}
         .@{wire.name}(@{accel.stateWires[statesSeen].name}),
      #{inc stateSeen}
      #{end}      

      #{if decl.memoryMapDWords}
      .valid(memoryMappedEnable[@{memoryMappedIndex}]),
      .wstrb(wstrb),
      .addr(addr[@{inst.versatData.addressTopBit}:0]),
      .rdata(unitRData[@{memoryMappedIndex}]),
      .ready(unitReady[@{memoryMappedIndex}]),
      .wdata(wdata),
      #{inc memoryMappedIndex}
      #{end}

      #{for i decl.nIOs}
      .databus_ready(databus_ready[@{ioIndex}]),
      .databus_valid(databus_valid[@{ioIndex}]),
      .databus_addr(databus_addr[@{ioIndex * 32}+:32]),
      .databus_rdata(databus_rdata[@{ioIndex * 32}+:32]),
      .databus_wdata(databus_wdata[@{ioIndex * 32}+:32]),
      .databus_wstrb(databus_wstrb[@{ioIndex * 4}+:4]),
      #{inc ioIndex}
      #{end} 
      
      .run(run),
      .done(unitDone[@{counter}]),
      .clk(clk),
      .rst(rst)
   );
      #{end}
   #{end}
#{set counter counter + 1}
#{end}

#{for inst instances}
#{set decl inst.declaration}
#{if decl.name.str == "circuitOutput"}
   #{for i inst.tempData.numberInputs}
   #{set in inst.tempData.inputs[i]}
   assign out@{i} = output_@{in.inst.inst.id}_@{in.inst.port};
   #{end}
#{end}
#{end}

endmodule

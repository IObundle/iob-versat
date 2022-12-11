`timescale 1ns / 1ps

#{include "versat_common.tpl"}

module @{base.name} #(
      parameter DELAY_W = 32,
      parameter DATA_W = 32,
      parameter ADDR_W = `ADDR_W,
      parameter AXI_ADDR_W = 32
   )(

      #{for i base.nInputs}
      input [DATA_W - 1:0] in@{i},
      #{end}

      #{for i base.nOutputs}
      (* versat_latency = @{base.latencies[0]} *) output [DATA_W - 1:0] out@{i},
      #{end}

      #{if firstComb.declaration.implementsDone}
      output done,
      #{end}

      #{for unit base.staticUnits}
      #{for i unit.nConfigs}
      #{set wire unit.wires[i]}
      input [@{wire.bitsize-1}:0]     @{unit.module.name}_@{unit.name}_@{wire.name},
      #{end}
      #{end}

      #{for i base.nConfigs}
      #{set wire base.configWires[i]}
      input [@{wire.bitsize-1}:0]     @{wire.name},
      #{end}

      #{for i base.nStates}
      #{set wire base.stateWires[i]}
      output [@{wire.bitsize-1}:0]    @{wire.name},
      #{end}

      input [DELAY_W - 1:0] delay0, // For now, iterative units always have one delay. No delay for sub unit, not yet figured out how to proceed in that regard 

      #{if base.nIOs}
      // Databus master interface
      input                      databus_ready,
      output                     databus_valid,
      output [AXI_ADDR_W-1:0]    databus_addr,
      input  [`DATAPATH_W-1:0]   databus_rdata,
      output [`DATAPATH_W-1:0]   databus_wdata,
      output [`DATAPATH_W/8-1:0] databus_wstrb,
      output [7:0]               databus_len,
      input                      databus_last,
      #{end}

      #{if base.isMemoryMapped}
      // data/control interface
      input                           valid,
      #{if base.memoryMapBits}
      input [@{base.memoryMapBits - 1}:0] addr,
      #{end}

      input [DATA_W/8-1:0]            wstrb,
      input [DATA_W-1:0]              wdata,
      output                          ready,
      output reg [DATA_W-1:0]         rdata,
      #{end}

      input clk,
      input rst,
      input run
   );

reg [31:0] delay;
reg running;
reg looping;

reg [31:0] data[@{base.dataSize-1}:0];
wire [31:0] unitOut[@{base.nOutputs-1}:0];

#{for i base.nInputs}
   #{set portInstance1 firstComb.graphData.inputs[i].instConnectedTo}
   #{set portInstance2 secondComb.graphData.inputs[i].instConnectedTo}
   wire [31:0] inputWire_@{i} = (looping ? #{call iterativeOutputName portInstance2 i} : #{call iterativeOutputName portInstance1 i}); // @{portInstance2.inst.name} : @{portInstance1.inst.name}
#{end}

always @(posedge clk)
begin
   if(rst) begin
      running <= 0;
   end else if(run) begin
      delay <= delay0;
      looping <= 0;
      running <= 1;
   end else if(running) begin
      if(!looping) begin // Delay phase (First graph, also responsible for providing the latency values?) 
         if(delay == 0) begin
            #{for i secondData.graphData.inputPortsUsed}
               #{set portInstance secondData.graphData.inputs[i].instConnectedTo}
               #{set port secondData.graphData.inputs[i].port}
               data[@{i}] <= #{call iterativeOutputName portInstance port};
            #{end}
            looping <= 1;
         end else begin
            delay <= delay - 1;
         end
      end else begin // Looping state
         // Update phase (Second graph)
         #{for i secondData.graphData.inputPortsUsed}
         #{set portInstance secondData.graphData.inputs[i].instConnectedTo}
         #{set port secondData.graphData.inputs[i].port}
            data[@{i}] <= #{call iterativeOutputName portInstance port};
         #{end}
      end
   end
end

// Second graph
#{for i base.nOutputs}
#{set portInstance1 firstOut.graphData.inputs[i].instConnectedTo}
#{set portInstance2 secondOut.graphData.inputs[i].instConnectedTo}
   assign out@{i} = (looping ? #{call iterativeOutputName portInstance2 i} : #{call iterativeOutputName portInstance1 i});
#{end}

#{if firstComb.declaration.implementsDone}
wire unitDone;
assign done = (unitDone & looping);
#{end}

#{set decl firstComb.declaration}

@{decl.name} @{firstComb.name}(
   #{for i comb.graphData.inputPortsUsed}
      .in@{i}(inputWire_@{i}),
   #{end}
   #{for i decl.nOutputs}
      .out@{i}(unitOut[@{i}]),
   #{end}

   #{for i decl.nConfigs}
   #{set wire decl.configWires[i]}
   .@{wire.name}(@{decl.configWires[i].name}),
   #{end}

   #{for unit decl.staticUnits}         
   #{for i unit.nConfigs}
   #{set wire unit.wires[i]}
   .@{unit.module.name}_@{unit.name}_@{wire.name}(@{unit.module.name}_@{unit.name}_@{wire.name}),
   #{end}
   #{end}

   #{for i decl.nStates}
   #{set wire decl.stateWires[i]}
      .@{wire.name}(@{decl.stateWires[i].name}),
   #{end}

   #{if decl.isMemoryMapped}
   .valid(valid),
   .wstrb(wstrb),
   #{if decl.memoryMapBits}
   .addr(addr),
   #{end}
   .rdata(rdata),
   .ready(ready),
   .wdata(wdata),
   #{end}

   #{if decl.nIOs}
   .databus_ready(databus_ready),
   .databus_valid(databus_valid),
   .databus_addr(databus_addr),
   .databus_rdata(databus_rdata),
   .databus_wdata(databus_wdata),
   .databus_wstrb(databus_wstrb),
   .databus_len(databus_len),
   .databus_last(databus_last),
   #{end} 

   #{if firstComb.declaration.implementsDone}
   .done(unitDone),
   #{end}

   .clk(clk),
   .rst(rst),
   .run(run)
);

endmodule

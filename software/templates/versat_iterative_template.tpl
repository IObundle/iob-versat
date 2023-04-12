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
      #{set id unit.first}
      #{for wire unit.second.configs}
      input [@{wire.bitsize-1}:0]     @{id.parent.name}_@{unit.id.name}_@{wire.name},
      #{end}
      #{end}

      #{for wire base.configs}
      input [@{wire.bitsize-1}:0]     @{wire.name},
      #{end}

      #{for wire base.states}
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
   #{set portInstance1 firstComb.graphData.singleInputs[i]}
   #{set portInstance2 secondComb.graphData.singleInputs[i]}
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
            #{for input secondData.graphData.singleInputs}
            #{if input.inst}
               data[@{index}] <= #{call iterativeOutputName input index};
            #{end}
            #{end}
            looping <= 1;
         end else begin
            delay <= delay - 1;
         end
      end else begin // Looping state
         // Update phase (Second graph)
         #{for input secondData.graphData.singleInputs}
         #{if input.inst}
            data[@{index}] <= #{call iterativeOutputName input index};
         #{end}
         #{end}
      end
   end
end

// Second graph
#{for i base.nOutputs}
#{set portInstance1 firstOut.graphData.allInputs[i].instConnectedTo}
#{set portInstance2 secondOut.graphData.allInputs[i].instConnectedTo}
   assign out@{i} = (looping ? #{call iterativeOutputName portInstance2 i} : #{call iterativeOutputName portInstance1 i});
#{end}

#{if firstComb.declaration.implementsDone}
wire unitDone;
assign done = (unitDone & looping);
#{end}

#{set decl firstComb.declaration}

@{decl.name} @{firstComb.name}(
   #{for input comb.graphData.singleInputs}
      .in@{index}(inputWire_@{index}),
   #{end}
   #{for i decl.nOutputs}
      .out@{i}(unitOut[@{i}]),
   #{end}

   #{for wire decl.configs}
   .@{wire.name}(@{wire.name}),
   #{end}

   #{for unit decl.staticUnits}         
   #{set id unit.first}
   #{for wire unit.second.configs}
   .@{id.parent.name}_@{id.name}_@{wire.name}(@{id.parent.name}_@{id.name}_@{wire.name}),
   #{end}
   #{end}

   #{for wire decl.states}
      .@{wire.name}(@{wire.name}),
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

`timescale 1ns / 1ps

#{include "versat_common.tpl"}

module @{base.name}(

      #{for i base.nInputs}
      input [31:0] in@{i},
      #{end}

      #{for i base.nOutputs}
      (* versat_latency = @{base.latencies[0]} *) output [31:0] out@{i},
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

      input [31:0] delay0,

      input clk,
      input rst,
      input run
   );

reg [31:0] delay;
reg running;
reg looping;

reg [31:0] state[@{base.stateSize-1}:0];
wire [31:0] unitOut[@{base.nOutputs-1}:0];

#{for i base.nInputs}
   #{set portInstance1 firstComb.tempData.inputs[i].instConnectedTo}
   #{set portInstance2 secondComb.tempData.inputs[i].instConnectedTo}
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
            #{for i secondState.tempData.inputPortsUsed}
               #{set portInstance secondState.tempData.inputs[i].instConnectedTo}
               #{set port secondState.tempData.inputs[i].port}
               state[@{i}] <= #{call iterativeOutputName portInstance port};
            #{end}
            looping <= 1;
         end else begin
            delay <= delay - 1;
         end
      end else begin // Looping state
         // Update phase (Second graph)
         #{for i secondState.tempData.inputPortsUsed}
         #{set portInstance secondState.tempData.inputs[i].instConnectedTo}
         #{set port secondState.tempData.inputs[i].port}
            state[@{i}] <= #{call iterativeOutputName portInstance port};
         #{end}
      end
   end
end

// Second graph
#{for i base.nOutputs}
#{set portInstance1 firstOut.tempData.inputs[i].instConnectedTo}
#{set portInstance2 secondOut.tempData.inputs[i].instConnectedTo}
   assign out@{i} = (looping ? #{call iterativeOutputName portInstance2 i} : #{call iterativeOutputName portInstance1 i});
#{end}

// Remember, should be combinatorial (no clk,no run, no nothing)
@{firstComb.declaration.name} @{firstComb.name}(
   #{for i comb.tempData.inputPortsUsed}
      .in@{i}(inputWire_@{i}),
   #{end}
   #{for i base.nOutputs}
      .out@{i}(unitOut[@{i}]),
   #{end}

   #{for i base.nConfigs}
   #{set wire base.configWires[i]}
   .@{wire.name}(@{accel.configWires[configsSeen].name}),
   #{inc configsSeen}
   #{end}
   #{for unit base.staticUnits}         
   #{for i unit.nConfigs}
   #{set wire unit.wires[i]}
   .@{unit.module.name}_@{unit.name}_@{wire.name}(@{unit.module.name}_@{unit.name}_@{wire.name}),
   #{end}
   #{end}

   .clk(clk),
   .rst(rst),
   .run(run)
);

endmodule

`timescale 1ns / 1ps

#{include "versat_common.tpl"}

module @{base.name}(

      #{for i base.nInputs}
      input [31:0] in@{i},
      #{end}

      #{for i base.nOutputs}
      (* versat_latency = @{base.latency} *) output [31:0] out@{i},
      #{end}

      input [31:0] delay0,

      input clk,
      input rst,
      input run
   );

reg [31:0] delay;
reg running;

reg [31:0] state[@{base.stateSize-1}:0];
wire [31:0] unitOut[@{base.nOutputs-1}:0];

always @(posedge clk)
begin
   if(run) begin
      delay <= delay0;
      running <= 0;
   end else if(!running) begin // Delay phase (First graph, also responsible for providing the latency values?) 
      if(delay == 0) begin
      #{for i firstOut.tempData.inputPortsUsed}
         state[@{i}] <= in@{firstOut.tempData.inputs[i].port}
      #{end}
         running <= 1;
      end else begin
         delay <= delay - 1;
      end
   end else begin
      // Update phase (Second graph)
      #{for i secondState.tempData.inputPortsUsed}
      #{set portInstance secondState.tempData.inputs[i].instConnectedTo}
      #{set port secondState.tempData.inputs[i].port}
      #{set printOut #{call iterativeOutputName portInstance port}}
         state[@{i}] <= @{printOut};
      #{end}
   end
end

// Second graph
#{for i secondOut.tempData.inputPortsUsed}
#{set portInstance secondOut.tempData.inputs[i].instConnectedTo}
#{set port secondOut.tempData.inputs[i].port}
#{set printOut #{call iterativeOutputName portInstance port}}
   assign out@{i} = @{printOut};
#{end}

// Remember, should be combinatorial (no clk,no run, no nothing)
@{base.baseDeclaration.name} @{unitName}(
   #{for i comb.tempData.inputPortsUsed}
   #{set portInstance comb.tempData.inputs[i].instConnectedTo}
   #{set port comb.tempData.inputs[i].port}
   #{set printOut #{call iterativeOutputName portInstance port}}
      .in@{i}(@{printOut}),
   #{end}
   #{for i base.nOutputs}
      .out@{i}(unitOut[@{i}]),
   #{end}
);

endmodule

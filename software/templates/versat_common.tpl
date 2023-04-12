#{define retIterativeOutputName portInstance port}
#{if portInstance.inst.declaration == versat.data}
   #{return "data[" # port # "]"}
#{else}
#{if portInstance.inst.declaration.type == 2}
   #{return "in" # port}
#{else}
   #{return "unitOut[" # port # "]"}
#{end}
#{end}
#{end}

#{define iterativeOutputName portInstance port}
#{set val #{call retIterativeOutputName portInstance port}}
@{val}#{end}

#{define retOutputName portNode}
#{if portNode.node}
#{set inst2 portNode.node.inst}
#{set decl2 inst2.declaration}
   #{if decl2.type == 2}
      #{return "in" # inst2.id}
   #{else}
      #{if decl2.isOperation}
         #{if decl2.outputLatencies[0] == 0}
            #{return "comb_" # inst2.name |> Identify}
         #{else}
            #{return "seq_" # inst2.name |> Identify}
         #{end} 
      #{else}
         #{return "output_" # inst2.id # "_" # portNode.port} 
      #{end}
   #{end}
#{else}
#{return ""}
#{end}
#{end}

#{define outputName portInstance}
#{set val #{call retOutputName portInstance}}
@{val}#{end}

#{define CountDones instances}
   #{set nDones 0}
   #{for node instances}
   #{if node.inst.declaration.implementsDone}
   #{inc nDones}
   #{end}
   #{end}
   #{return nDones}
#{end}

#{define CountOperations instances}
   #{set nOperations 0}
   #{set nCombOperations 0}
   #{set nSeqOperations 0}
   #{for node instances}
      #{if node.inst.declaration.isOperation}
         #{inc nOperations}
         #{if node.inst.declaration.outputLatencies[0] == 0}
            #{inc nCombOperations}
         #{else}
            #{inc nSeqOperations}
         #{end}
      #{end}
   #{end}
   #{return nOperations}
#{end}

